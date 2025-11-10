#include "pch.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Component/Mesh/Public/VertexDatas.h"
#include "Physics/Public/AABB.h"
#include "Texture/Public/Texture.h"
#include "Texture/Public/Material.h"
#include "Manager/Asset/Public/ObjManager.h"
#include "Manager/Asset/Public/FbxImporter.h"
#include "Manager/Path/Public/PathManager.h"
#include "Render/Renderer/Public/RenderResourceFactory.h"
#include "Runtime/CoreUObject/Public/WindowsBinReader.h"
#include "Runtime/CoreUObject/Public/WindowsBinWriter.h"

IMPLEMENT_SINGLETON_CLASS(UAssetManager, UObject)
UAssetManager::UAssetManager()
{
	TextureManager = new FTextureManager();
}

UAssetManager::~UAssetManager() = default;

void UAssetManager::Initialize()
{
	TextureManager->LoadAllTexturesFromDirectory(UPathManager::GetInstance().GetDataPath());
	// Data 폴더 속 모든 .obj 파일 로드 및 캐싱
	LoadAllObjStaticMesh();
	// Data/Fbx 폴더 속 모든 .fbx 파일 로드 및 캐싱 (Static/Skeletal 자동 판별)
	LoadAllFbxMeshes();

	VertexData.Emplace(EPrimitiveType::Torus, &VerticesTorus);
	VertexData.Emplace(EPrimitiveType::Arrow, &VerticesArrow);
	VertexData.Emplace(EPrimitiveType::CubeArrow, &VerticesCubeArrow);
	VertexData.Emplace(EPrimitiveType::Ring, &VerticesRing);
	VertexData.Emplace(EPrimitiveType::Line, &VerticesLine);
	VertexData.Emplace(EPrimitiveType::Sprite, &VerticesVerticalSquare);

	IndexData.Emplace(EPrimitiveType::Sprite, &IndicesVerticalSquare);
	IndexBuffers.Emplace(EPrimitiveType::Sprite,
		FRenderResourceFactory::CreateIndexBuffer(IndicesVerticalSquare.GetData(), IndicesVerticalSquare.Num() * sizeof(uint32)));

	NumIndices.Emplace(EPrimitiveType::Sprite, static_cast<uint32>(IndicesVerticalSquare.Num()));

	VertexBuffers.Emplace(EPrimitiveType::Torus, FRenderResourceFactory::CreateVertexBuffer(
		VerticesTorus.GetData(), static_cast<int>(VerticesTorus.Num() * sizeof(FNormalVertex))));
	VertexBuffers.Emplace(EPrimitiveType::Arrow, FRenderResourceFactory::CreateVertexBuffer(
		VerticesArrow.GetData(), static_cast<int>(VerticesArrow.Num() * sizeof(FNormalVertex))));
	VertexBuffers.Emplace(EPrimitiveType::CubeArrow, FRenderResourceFactory::CreateVertexBuffer(
		VerticesCubeArrow.GetData(), static_cast<int>(VerticesCubeArrow.Num() * sizeof(FNormalVertex))));
	VertexBuffers.Emplace(EPrimitiveType::Ring, FRenderResourceFactory::CreateVertexBuffer(
		VerticesRing.GetData(), static_cast<int>(VerticesRing.Num() * sizeof(FNormalVertex))));
	VertexBuffers.Emplace(EPrimitiveType::Line, FRenderResourceFactory::CreateVertexBuffer(
		VerticesLine.GetData(), static_cast<int>(VerticesLine.Num() * sizeof(FNormalVertex))));
	VertexBuffers.Emplace(EPrimitiveType::Sprite, FRenderResourceFactory::CreateVertexBuffer(
		VerticesVerticalSquare.GetData(), static_cast<int>(VerticesVerticalSquare.Num() * sizeof(FNormalVertex))));

	NumVertices.Emplace(EPrimitiveType::Torus, static_cast<uint32>(VerticesTorus.Num()));
	NumVertices.Emplace(EPrimitiveType::Arrow, static_cast<uint32>(VerticesArrow.Num()));
	NumVertices.Emplace(EPrimitiveType::CubeArrow, static_cast<uint32>(VerticesCubeArrow.Num()));
	NumVertices.Emplace(EPrimitiveType::Ring, static_cast<uint32>(VerticesRing.Num()));
	NumVertices.Emplace(EPrimitiveType::Line, static_cast<uint32>(VerticesLine.Num()));
	NumVertices.Emplace(EPrimitiveType::Sprite, static_cast<uint32>(VerticesVerticalSquare.Num()));

	// Calculate AABB for all primitive types (excluding StaticMesh)
	for (const auto& Pair : VertexData)
	{
		EPrimitiveType Type = Pair.first;
		const auto* Vertices = Pair.second;
		if (!Vertices || Vertices->IsEmpty())
		{
			continue;
		}

		AABBs[Type] = CalculateAABB(*Vertices);
	}

	// Calculate AABB for each StaticMesh
	for (const auto& MeshPair : StaticMeshCache)
	{
		const FName& ObjPath = MeshPair.first;
		const auto& Mesh = MeshPair.second;
		if (!Mesh || !Mesh->IsValid())
		{
			continue;
		}

		const auto& Vertices = Mesh->GetVertices();
		if (Vertices.IsEmpty())
		{
			continue;
		}

		StaticMeshAABBs[ObjPath] = CalculateAABB(Vertices);
	}
}

void UAssetManager::Release()
{
	// TMap.Value()
	for (auto& Pair : VertexBuffers)
	{
		SafeRelease(Pair.second);
	}
	for (auto& Pair : IndexBuffers)
	{
		SafeRelease(Pair.second);
	}

	for (auto& Pair : StaticMeshVertexBuffers)
	{
		SafeRelease(Pair.second);
	}
	for (auto& Pair : StaticMeshIndexBuffers)
	{
		SafeRelease(Pair.second);
	}

	StaticMeshCache.Empty();
	StaticMeshVertexBuffers.Empty();
	StaticMeshIndexBuffers.Empty();

	// TMap.Empty()
	VertexBuffers.Empty();
	IndexBuffers.Empty();

	SafeDelete(TextureManager);
}

/**
 * @brief Data/ 경로 하위에 모든 .obj 파일을 로드 후 캐싱한다
 */
void UAssetManager::LoadAllObjStaticMesh()
{
	TArray<FName> ObjList;
	const FString DataDirectory = "Data/"; // 검색할 기본 디렉토리
	// 디렉토리가 실제로 존재하는지 먼저 확인합니다.
	if (exists(DataDirectory) && std::filesystem::is_directory(DataDirectory))
	{
		// recursive_directory_iterator를 사용하여 디렉토리와 모든 하위 디렉토리를 순회합니다.
		for (const auto& Entry : std::filesystem::recursive_directory_iterator(DataDirectory))
		{
			// 현재 항목이 일반 파일이고, 확장자가 ".obj"인지 확인합니다.
			if (Entry.is_regular_file() && Entry.path().extension() == ".obj")
			{
				// .generic_string()을 사용하여 OS에 상관없이 '/' 구분자를 사용하는 경로를 바로 얻습니다.
				FString PathString = Entry.path().generic_string();

				// 찾은 파일 경로를 FName으로 변환하여 ObjList에 추가합니다.
				ObjList.Emplace(FName(PathString));
			}
			// FBX 파일은 LoadAllFbxMeshes()에서 처리하므로 여기서는 제외
		}
	}

	// CW 와인딩
	FObjImporter::Configuration Config;
	Config.bFlipWindingOrder = true;
	Config.bIsBinaryEnabled = true;
	Config.bPositionToUEBasis = true;
	Config.bNormalToUEBasis = true;
	Config.bUVToUEBasis = true;

	// 범위 기반 for문을 사용하여 배열의 모든 요소를 순회합니다.
	for (const FName& ObjPath : ObjList)
	{
		// FObjManager가 UStaticMesh 포인터를 반환한다고 가정합니다.
		UStaticMesh* LoadedMesh = FObjManager::LoadObjStaticMesh(ObjPath, Config);

		// 로드에 성공했는지 확인합니다.
		if (LoadedMesh)
		{
			StaticMeshCache.Emplace(ObjPath, LoadedMesh);

			StaticMeshVertexBuffers.Emplace(ObjPath, this->CreateVertexBuffer(LoadedMesh->GetVertices()));
			StaticMeshIndexBuffers.Emplace(ObjPath, this->CreateIndexBuffer(LoadedMesh->GetIndices()));
		}
	}
}

ID3D11Buffer* UAssetManager::GetVertexBuffer(FName InObjPath)
{
	return StaticMeshVertexBuffers.FindRef(InObjPath);
}

ID3D11Buffer* UAssetManager::GetIndexBuffer(FName InObjPath)
{
	return StaticMeshIndexBuffers.FindRef(InObjPath);
}

ID3D11Buffer* UAssetManager::CreateVertexBuffer(TArray<FNormalVertex> InVertices)
{
	return FRenderResourceFactory::CreateVertexBuffer(
		InVertices.GetData(), InVertices.Num() * sizeof(FNormalVertex)
	);
}

ID3D11Buffer* UAssetManager::CreateIndexBuffer(TArray<uint32> InIndices)
{
	return FRenderResourceFactory::CreateIndexBuffer(
		InIndices.GetData(), InIndices.Num() * sizeof(uint32)
	);
}

TArray<FNormalVertex>* UAssetManager::GetVertexData(EPrimitiveType InType)
{
	return VertexData[InType];
}

ID3D11Buffer* UAssetManager::GetVertexBuffer(EPrimitiveType InType)
{
	return VertexBuffers[InType];
}

uint32 UAssetManager::GetNumVertices(EPrimitiveType InType)
{
	return NumVertices[InType];
}

TArray<uint32>* UAssetManager::GetIndexData(EPrimitiveType InType)
{
	return IndexData[InType];
}

ID3D11Buffer* UAssetManager::GetIndexBuffer(EPrimitiveType InType)
{
	return IndexBuffers[InType];
}

uint32 UAssetManager::GetNumIndices(EPrimitiveType InType)
{
	return NumIndices[InType];
}

FAABB& UAssetManager::GetAABB(EPrimitiveType InType)
{
	return AABBs[InType];
}

FAABB& UAssetManager::GetStaticMeshAABB(FName InName)
{
	return StaticMeshAABBs[InName];
}

// StaticMesh Cache Accessors
UStaticMesh* UAssetManager::GetStaticMeshFromCache(const FName& InObjPath)
{
	if (auto* FoundPtr = StaticMeshCache.Find(InObjPath))
	{
		return *FoundPtr;
	}
	return nullptr;
}

void UAssetManager::AddStaticMeshToCache(const FName& InObjPath, UStaticMesh* InStaticMesh)
{
	if (!InStaticMesh)
	{
		return;
	}

	if (!StaticMeshCache.Contains(InObjPath))
	{
		StaticMeshCache.Add(InObjPath, InStaticMesh);
	}
}

void UAssetManager::AddVertexBufferToCache(const FName& InObjPath, ID3D11Buffer* InBuffer)
{
	if (!InBuffer)
	{
		return;
	}

	if (!StaticMeshVertexBuffers.Contains(InObjPath))
	{
		StaticMeshVertexBuffers.Add(InObjPath, InBuffer);
	}
}

void UAssetManager::AddIndexBufferToCache(const FName& InObjPath, ID3D11Buffer* InBuffer)
{
	if (!InBuffer)
	{
		return;
	}

	if (!StaticMeshIndexBuffers.Contains(InObjPath))
	{
		StaticMeshIndexBuffers.Add(InObjPath, InBuffer);
	}
}

void UAssetManager::AddStaticMeshAABB(const FName& InObjPath, const FAABB& InAABB)
{
	if (!StaticMeshAABBs.Contains(InObjPath))
	{
		StaticMeshAABBs.Add(InObjPath, InAABB);
	}
}

void UAssetManager::AddSkeletalMeshToCache(const FName& InFbxPath, USkeletalMesh* InMesh)
{
	if (!InMesh)
	{
		return;
	}

	if (!SkeletalMeshCache.Contains(InFbxPath))
	{
		SkeletalMeshCache.Add(InFbxPath, InMesh);
	}
}

USkeletalMesh* UAssetManager::GetSkeletalMeshFromCache(const FName& InFbxPath)
{
	if (SkeletalMeshCache.Contains(InFbxPath))
	{
		return SkeletalMeshCache[InFbxPath];
	}
	return nullptr;
}

USkeletalMesh* UAssetManager::LoadSkeletalMesh(const FName& InFbxPath)
{
	// 캐시에 이미 있으면 반환
	USkeletalMesh* CachedMesh = GetSkeletalMeshFromCache(InFbxPath);
	if (CachedMesh)
	{
		return CachedMesh;
	}

	// 바이너리 파일 경로 생성 (Cooked 폴더에 .fbxbin)
	path CookedPath = UPathManager::GetInstance().GetCookedPath();
	path FbxPath(InFbxPath.ToString());
	path BinFilePath = CookedPath / (FbxPath.stem().wstring() + L".fbxbin");

	FSkeletalMesh* SkeletalMeshData = nullptr;
	bool bLoadedFromBinary = false;

	FString PathString = InFbxPath.ToString();

	// 바이너리 캐시 확인
	if (exists(BinFilePath) && IsBinaryUpToDate(InFbxPath, FName(BinFilePath.generic_string())))
	{
		auto StartTime = std::chrono::high_resolution_clock::now();
		SkeletalMeshData = LoadSkeletalMeshBinary(FName(BinFilePath.generic_string()));
		auto EndTime = std::chrono::high_resolution_clock::now();
		auto Duration = std::chrono::duration_cast<std::chrono::milliseconds>(EndTime - StartTime);

		if (SkeletalMeshData && SkeletalMeshData->IsValid())
		{
			bLoadedFromBinary = true;
			UE_LOG_SUCCESS("SkeletalMeshCache: Loaded from fbxbin in %lld ms: '%s'", Duration.count(), PathString.c_str());
		}
		else
		{
			delete SkeletalMeshData;
			SkeletalMeshData = nullptr;
			UE_LOG_INFO("SkeletalMeshCache: fbxbin outdated, reloading from FBX '%s'", PathString.c_str());
		}
	}

	// 바이너리에서 로드 실패하면 FBX 파싱
	if (!SkeletalMeshData)
	{
		auto StartTime = std::chrono::high_resolution_clock::now();

		FFbxImporter& Parser = FFbxImporter::GetInstance();
		SkeletalMeshData = new FSkeletalMesh();

		if (!Parser.LoadSkeletalMesh(PathString, *SkeletalMeshData))
		{
			delete SkeletalMeshData;
			return nullptr;
		}

		auto EndTime = std::chrono::high_resolution_clock::now();
		auto Duration = std::chrono::duration_cast<std::chrono::milliseconds>(EndTime - StartTime);
		UE_LOG_INFO("SkeletalMeshCache: Parsed FBX in %lld ms: '%s'", Duration.count(), PathString.c_str());
	}

	if (SkeletalMeshData && SkeletalMeshData->IsValid())
	{
		USkeletalMesh* LoadedMesh = new USkeletalMesh();
		LoadedMesh->SetSkeletalMeshAsset(SkeletalMeshData);
		LoadedMesh->SetAssetPath(InFbxPath);

		// FBX 파일이 있는 디렉토리 경로
		path FbxPath(PathString);
		path FbxDirectory = FbxPath.parent_path();

		// MaterialInfo에서 UMaterial 생성 및 텍스처 로드
		for (int32 MaterialIndex = 0; MaterialIndex < SkeletalMeshData->MaterialInfo.Num(); ++MaterialIndex)
		{
			const FMaterial& MaterialInfo = SkeletalMeshData->MaterialInfo[MaterialIndex];
			UMaterial* Material = NewObject<UMaterial>();
			Material->SetName(MaterialInfo.Name);
			Material->SetMaterialData(MaterialInfo);

			// Diffuse 텍스처 로드
			if (!MaterialInfo.DiffuseTexturePath.IsEmpty())
			{
				FString TexturePathStr = (FbxDirectory / MaterialInfo.DiffuseTexturePath).generic_string();
				if (exists(TexturePathStr))
				{
					UTexture* DiffuseTexture = LoadTexture(TexturePathStr);
					if (DiffuseTexture)
					{
						Material->SetDiffuseTexture(DiffuseTexture);
					}
				}
			}

			// Normal 텍스처 로드
			if (!MaterialInfo.NormalTexturePath.IsEmpty())
			{
				FString TexturePathStr = (FbxDirectory / MaterialInfo.NormalTexturePath).generic_string();
				if (exists(TexturePathStr))
				{
					UTexture* NormalTexture = LoadTexture(TexturePathStr);
					if (NormalTexture)
					{
						Material->SetNormalTexture(NormalTexture);
					}
				}
			}

			// Specular 텍스처 로드
			if (!MaterialInfo.SpecularTexturePath.IsEmpty())
			{
				FString TexturePathStr = (FbxDirectory / MaterialInfo.SpecularTexturePath).generic_string();
				if (exists(TexturePathStr))
				{
					UTexture* SpecularTexture = LoadTexture(TexturePathStr);
					if (SpecularTexture)
					{
						Material->SetSpecularTexture(SpecularTexture);
					}
				}
			}

			LoadedMesh->SetMaterial(MaterialIndex, Material);
		}

		AddSkeletalMeshToCache(InFbxPath, LoadedMesh);

		// GPU 버퍼 생성
		FSkeletalMeshBuffers Buffers;
		for (const auto& Section : SkeletalMeshData->Sections)
		{
			ID3D11Buffer* VertexBuffer = CreateSkeletalVertexBuffer(Section.Vertices);
			ID3D11Buffer* IndexBuffer = CreateIndexBuffer(Section.Indices);
			Buffers.SectionVertexBuffers.Add(VertexBuffer);
			Buffers.SectionIndexBuffers.Add(IndexBuffer);
		}
		SkeletalMeshBuffers.Emplace(InFbxPath, Buffers);

		// 바이너리 캐시 저장 (FBX에서 로드한 경우에만)
		if (!bLoadedFromBinary)
		{
			SaveSkeletalMeshBinary(InFbxPath, SkeletalMeshData);
		}

		return LoadedMesh;
	}

	return nullptr;
}

FSkeletalMeshBuffers* UAssetManager::GetSkeletalMeshBuffers(const FName& InFbxPath)
{
	if (SkeletalMeshBuffers.Contains(InFbxPath))
	{
		return &SkeletalMeshBuffers[InFbxPath];
	}
	return nullptr;
}

/**
 * @brief Vertex 배열로부터 AABB(Axis-Aligned Bounding Box)를 계산하는 헬퍼 함수
 * @param Vertices 정점 데이터 배열
 * @return 계산된 FAABB 객체
 */
FAABB UAssetManager::CalculateAABB(const TArray<FNormalVertex>& Vertices)
{
	FVector MinPoint(+FLT_MAX, +FLT_MAX, +FLT_MAX);
	FVector MaxPoint(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (const auto& Vertex : Vertices)
	{
		MinPoint.X = std::min(MinPoint.X, Vertex.Position.X);
		MinPoint.Y = std::min(MinPoint.Y, Vertex.Position.Y);
		MinPoint.Z = std::min(MinPoint.Z, Vertex.Position.Z);

		MaxPoint.X = std::max(MaxPoint.X, Vertex.Position.X);
		MaxPoint.Y = std::max(MaxPoint.Y, Vertex.Position.Y);
		MaxPoint.Z = std::max(MaxPoint.Z, Vertex.Position.Z);
	}

	return {MinPoint, MaxPoint};
}

/**
 * @brief 넘겨준 경로로 캐싱된 UTexture 포인터를 반환해주는 함수
 * @param 로드할 텍스처 경로
 * @return 캐싱된 UTexture 포인터
 */
UTexture* UAssetManager::LoadTexture(const FName& InFilePath)
{
	return TextureManager->LoadTexture(InFilePath);
}

/**
 * @brief 지금까지 캐싱된 UTexture 포인터 목록 반환해주는 함수
 * @return {경로, 캐싱된 UTexture 포인터}
 */
const TMap<FName, UTexture*>& UAssetManager::GetTextureCache() const
{
	return TextureManager->GetTextureCache();
}

/**
 * @brief Data/Fbx 경로 하위의 모든 .fbx 파일을 로드하여 캐싱 (Static/Skeletal 자동 판별)
 */
void UAssetManager::LoadAllFbxMeshes()
{
	TArray<FName> FbxList;
	path FbxDirectory = UPathManager::GetInstance().GetDataPath() / "FBX";

	// 디렉토리 존재 확인
	if (!exists(FbxDirectory) || !std::filesystem::is_directory(FbxDirectory))
	{
		return; // 폴더가 없으면 조용히 반환
	}

	// .fbx 파일 찾기 (대소문자 구분 없이)
	for (const auto& Entry : std::filesystem::recursive_directory_iterator(FbxDirectory))
	{
		if (Entry.is_regular_file())
		{
			FString Extension = Entry.path().extension().string();
			// 대소문자 구분 없이 .fbx 확인
			if (Extension == ".fbx" || Extension == ".FBX" || Extension == ".Fbx")
			{
				FString PathString = Entry.path().generic_string();
				FbxList.Emplace(FName(PathString));
			}
		}
	}

	// 각 FBX 파일을 Static/Skeletal 판별하여 로딩
	for (const FName& FbxPath : FbxList)
	{
		FString PathString = FbxPath.ToString();

		// Skeletal Mesh 여부 판별
		bool bIsSkeletal = FFbxImporter::IsSkeletalMesh(PathString);

		if (bIsSkeletal)
		{
			// LoadSkeletalMesh 함수 호출 (Material 로딩 포함)
			USkeletalMesh* LoadedMesh = LoadSkeletalMesh(FbxPath);
			if (LoadedMesh)
			{
				UE_LOG("AssetManager: Loaded Skeletal Mesh: %s (Materials: %d)",
					PathString.data(), LoadedMesh->GetNumMaterials());
			}
		}
		else
		{
			// Static Mesh 로딩
			FFbxImporter& Parser = FFbxImporter::GetInstance();
			FStaticMesh* StaticMeshData = new FStaticMesh();
			if (Parser.LoadStaticMesh(PathString, *StaticMeshData))
			{
				UStaticMesh* LoadedMesh = new UStaticMesh();
				LoadedMesh->SetStaticMeshAsset(StaticMeshData);

				// Cache에 추가 (Add 함수 사용)
				AddStaticMeshToCache(FbxPath, LoadedMesh);

				// GPU 버퍼 생성 (Add 함수 사용)
				ID3D11Buffer* VertexBuffer = CreateVertexBuffer(StaticMeshData->Vertices);
				ID3D11Buffer* IndexBuffer = CreateIndexBuffer(StaticMeshData->Indices);
				AddVertexBufferToCache(FbxPath, VertexBuffer);
				AddIndexBufferToCache(FbxPath, IndexBuffer);

				// AABB 계산 (Add 함수 사용)
				FAABB MeshAABB = CalculateAABB(StaticMeshData->Vertices);
				AddStaticMeshAABB(FbxPath, MeshAABB);

				UE_LOG("AssetManager: Loaded Static Mesh: %s (Vertices: %d)", PathString.c_str(), StaticMeshData->Vertices.Num());
			}
			else
			{
				delete StaticMeshData;
			}
		}
	}
}

ID3D11Buffer* UAssetManager::CreateSkeletalVertexBuffer(const TArray<FSkeletalVertex>& InVertices)
{
	// FSkeletalVertex를 FNormalVertex*로 reinterpret_cast (구조체 레이아웃 호환)
	return FRenderResourceFactory::CreateVertexBuffer(
		reinterpret_cast<FNormalVertex*>(const_cast<FSkeletalVertex*>(InVertices.GetData())),
		static_cast<int>(InVertices.Num() * sizeof(FSkeletalVertex))
	);
}

/**
 * @brief FBX 파일에서 로드한 SkeletalMesh를 바이너리로 저장
 * @param InFbxPath 원본 FBX 파일 경로
 * @param InMesh 저장할 FSkeletalMesh 포인터
 * @return 저장 성공 여부
 */
bool UAssetManager::SaveSkeletalMeshBinary(const FName& InFbxPath, const FSkeletalMesh* InMesh)
{
	if (!InMesh || !InMesh->IsValid())
	{
		return false;
	}

	path CookedPath = UPathManager::GetInstance().GetCookedPath();
	path FbxPath(InFbxPath.ToString());
	path BinFilePath = CookedPath / (FbxPath.stem().wstring() + L".fbxbin");

	FWindowsBinWriter BinWriter(BinFilePath);
	FSkeletalMesh* MeshData = const_cast<FSkeletalMesh*>(InMesh);
	BinWriter << (*MeshData);

	UE_LOG_SUCCESS("SkeletalMeshCache: Saved fbxbin '%ls'", BinFilePath.c_str());
	return true;
}

/**
 * @brief 바이너리 파일에서 SkeletalMesh 로드
 * @param InBinPath 바이너리 파일 경로
 * @return 로드된 FSkeletalMesh 포인터 (실패 시 nullptr)
 */
FSkeletalMesh* UAssetManager::LoadSkeletalMeshBinary(const FName& InBinPath)
{
	FString BinPathString = InBinPath.ToString();
	if (!exists(BinPathString))
	{
		return nullptr;
	}

	FSkeletalMesh* SkeletalMeshData = new FSkeletalMesh();
	FWindowsBinReader BinReader(BinPathString);
	BinReader << (*SkeletalMeshData);

	UE_LOG_SUCCESS("SkeletalMeshCache: Loaded cached fbxbin '%s'", BinPathString.data());
	return SkeletalMeshData;
}

/**
 * @brief 바이너리 파일이 FBX 파일보다 최신인지 확인
 * @param InFbxPath 원본 FBX 파일 경로
 * @param InBinPath 바이너리 파일 경로
 * @return 바이너리 파일이 최신이면 true
 */
bool UAssetManager::IsBinaryUpToDate(const FName& InFbxPath, const FName& InBinPath)
{
	if (!exists(InFbxPath.ToString()) || !exists(InBinPath.ToString()))
	{
		return false;
	}

	auto FbxTime = std::filesystem::last_write_time(InFbxPath.ToString());
	auto BinTime = std::filesystem::last_write_time(InBinPath.ToString());

	return BinTime >= FbxTime;
}
