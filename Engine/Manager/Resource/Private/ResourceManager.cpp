#include "pch.h"
#include "Manager/Resource/Public/ResourceManager.h"

#include "Mesh/Public/VertexDatas.h"
#include "Render/Renderer/Public/Renderer.h"
#include "DirectXTK/WICTextureLoader.h"
#include "DirectXTK/DDSTextureLoader.h"

IMPLEMENT_SINGLETON_CLASS_BASE(UResourceManager)

UResourceManager::UResourceManager() = default;

UResourceManager::~UResourceManager() = default;

void UResourceManager::Initialize()
{
	URenderer& Renderer = URenderer::GetInstance();

	// TMap.Add()
	VertexDatas.emplace(EPrimitiveType::Cube, &VerticesCube);
	VertexDatas.emplace(EPrimitiveType::Sphere, &VerticesSphere);
	VertexDatas.emplace(EPrimitiveType::Triangle, &VerticesTriangle);
	VertexDatas.emplace(EPrimitiveType::Square, &VerticesSquare);
	VertexDatas.emplace(EPrimitiveType::Torus, &VerticesTorus);
	VertexDatas.emplace(EPrimitiveType::Arrow, &VerticesArrow);
	VertexDatas.emplace(EPrimitiveType::CubeArrow, &VerticesCubeArrow);
	VertexDatas.emplace(EPrimitiveType::Ring, &VerticesRing);
	VertexDatas.emplace(EPrimitiveType::Line, &VerticesLine);

	// TArray.GetData(), TArray.Num()*sizeof(FVertexSimple), TArray.GetTypeSize()
	Vertexbuffers.emplace(EPrimitiveType::Cube, Renderer.CreateVertexBuffer(
		                      VerticesCube.data(), static_cast<int>(VerticesCube.size()) * sizeof(FVertex)));
	Vertexbuffers.emplace(EPrimitiveType::Sphere, Renderer.CreateVertexBuffer(
		                      VerticesSphere.data(), static_cast<int>(VerticesSphere.size() * sizeof(FVertex))));
	Vertexbuffers.emplace(EPrimitiveType::Triangle, Renderer.CreateVertexBuffer(
		                      VerticesTriangle.data(), static_cast<int>(VerticesTriangle.size() * sizeof(FVertex))));
	Vertexbuffers.emplace(EPrimitiveType::Square, Renderer.CreateVertexBuffer(
		                      VerticesSquare.data(), static_cast<int>(VerticesSquare.size() * sizeof(FVertex))));
	Vertexbuffers.emplace(EPrimitiveType::Torus, Renderer.CreateVertexBuffer(
		                      VerticesTorus.data(), static_cast<int>(VerticesTorus.size() * sizeof(FVertex))));
	Vertexbuffers.emplace(EPrimitiveType::Arrow, Renderer.CreateVertexBuffer(
		                      VerticesArrow.data(), static_cast<int>(VerticesArrow.size() * sizeof(FVertex))));
	Vertexbuffers.emplace(EPrimitiveType::CubeArrow, Renderer.CreateVertexBuffer(
		                      VerticesCubeArrow.data(), static_cast<int>(VerticesCubeArrow.size() * sizeof(FVertex))));
	Vertexbuffers.emplace(EPrimitiveType::Ring, Renderer.CreateVertexBuffer(
		                      VerticesRing.data(), static_cast<int>(VerticesRing.size() * sizeof(FVertex))));
	Vertexbuffers.emplace(EPrimitiveType::Line, Renderer.CreateVertexBuffer(
		                      VerticesLine.data(), static_cast<int>(VerticesLine.size() * sizeof(FVertex))));

	NumVertices.emplace(EPrimitiveType::Cube, static_cast<uint32>(VerticesCube.size()));
	NumVertices.emplace(EPrimitiveType::Sphere, static_cast<uint32>(VerticesSphere.size()));
	NumVertices.emplace(EPrimitiveType::Triangle, static_cast<uint32>(VerticesTriangle.size()));
	NumVertices.emplace(EPrimitiveType::Square, static_cast<uint32>(VerticesSquare.size()));
	NumVertices.emplace(EPrimitiveType::Torus, static_cast<uint32>(VerticesTorus.size()));
	NumVertices.emplace(EPrimitiveType::Arrow, static_cast<uint32>(VerticesArrow.size()));
	NumVertices.emplace(EPrimitiveType::CubeArrow, static_cast<uint32>(VerticesCubeArrow.size()));
	NumVertices.emplace(EPrimitiveType::Ring, static_cast<uint32>(VerticesRing.size()));
	NumVertices.emplace(EPrimitiveType::Line, static_cast<uint32>(VerticesLine.size()));

	// Calculate Cube AABB
	FVector MinPoint(+FLT_MAX, +FLT_MAX, +FLT_MAX);
	FVector MaxPoint(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	for (const auto& Vertex : VerticesCube)
	{
		MinPoint.X = min(MinPoint.X, Vertex.Position.X);
		MinPoint.Y = min(MinPoint.Y, Vertex.Position.Y);
		MinPoint.Z = min(MinPoint.Z, Vertex.Position.Z);
		MaxPoint.X = max(MaxPoint.X, Vertex.Position.X);
		MaxPoint.Y = max(MaxPoint.Y, Vertex.Position.Y);
		MaxPoint.Z = max(MaxPoint.Z, Vertex.Position.Z);
	}
	CubeAABB = FAABB(MinPoint, MaxPoint);

	// Calculate Sphere AABB
	MinPoint = FVector(+FLT_MAX, +FLT_MAX, +FLT_MAX);
	MaxPoint = FVector(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	for (const auto& Vertex : VerticesSphere)
	{
		MinPoint.X = min(MinPoint.X, Vertex.Position.X);
		MinPoint.Y = min(MinPoint.Y, Vertex.Position.Y);
		MinPoint.Z = min(MinPoint.Z, Vertex.Position.Z);
		MaxPoint.X = max(MaxPoint.X, Vertex.Position.X);
		MaxPoint.Y = max(MaxPoint.Y, Vertex.Position.Y);
		MaxPoint.Z = max(MaxPoint.Z, Vertex.Position.Z);
	}
	SphereAABB = FAABB(MinPoint, MaxPoint);
}

void UResourceManager::Release()
{
	// TMap.Value()
	for (auto& Pair : Vertexbuffers)
	{
		URenderer::GetInstance().ReleaseVertexBuffer(Pair.second);
	}

	// TMap.Empty()
	Vertexbuffers.clear();

	// Texture Resource 해제
	ReleaseAllTextures();
}

TArray<FVertex>* UResourceManager::GetVertexData(EPrimitiveType InType)
{
	return VertexDatas[InType];
}

ID3D11Buffer* UResourceManager::GetVertexbuffer(EPrimitiveType InType)
{
	return Vertexbuffers[InType];
}

uint32 UResourceManager::GetNumVertices(EPrimitiveType InType)
{
	return NumVertices[InType];
}

const FAABB& UResourceManager::GetCubeAABB() const
{
	return CubeAABB;
}

const FAABB& UResourceManager::GetSphereAABB() const
{
	return SphereAABB;
}

/**
 * @brief 파일에서 텍스처를 로드하고 캐시에 저장하는 함수
 * 중복 로딩을 방지하기 위해 이미 로드된 텍스처는 캐시에서 반환
 * @param InFilePath 로드할 텍스처 파일의 경로
 * @return 성공시 ID3D11ShaderResourceView 포인터, 실패시 nullptr
 */
ID3D11ShaderResourceView* UResourceManager::LoadTexture(const FString& InFilePath)
{
	// 이미 로드된 텍스처가 있는지 확인
	auto Iter = TextureCache.find(InFilePath);
	if (Iter != TextureCache.end())
	{
		return Iter->second;
	}

	// 새로운 텍스처 로드
	ID3D11ShaderResourceView* TextureSRV = CreateTextureFromFile(InFilePath);
	if (TextureSRV)
	{
		TextureCache[InFilePath] = TextureSRV;
	}

	return TextureSRV;
}

/**
 * @brief 캐시된 텍스처를 가져오는 함수
 * 이미 로드된 텍스처만 반환하고 새로 로드하지는 않음
 * @param InFilePath 가져올 텍스처 파일의 경로
 * @return 캐시에 있으면 ID3D11ShaderResourceView 포인터, 없으면 nullptr
 */
ID3D11ShaderResourceView* UResourceManager::GetTexture(const FString& InFilePath)
{
	auto Iter = TextureCache.find(InFilePath);
	if (Iter != TextureCache.end())
	{
		return Iter->second;
	}

	return nullptr;
}

/**
 * @brief 특정 텍스처를 캐시에서 해제하는 함수
 * DirectX 리소스를 해제하고 캐시에서 제거
 * @param InFilePath 해제할 텍스처 파일의 경로
 */
void UResourceManager::ReleaseTexture(const FString& InFilePath)
{
	auto Iter = TextureCache.find(InFilePath);
	if (Iter != TextureCache.end())
	{
		if (Iter->second)
		{
			Iter->second->Release();
		}

		TextureCache.erase(Iter);
	}
}

/**
 * @brief 특정 텍스처가 캐시에 있는지 확인하는 함수
 * @param InFilePath 확인할 텍스처 파일의 경로
 * @return 캐시에 있으면 true, 없으면 false
 */
bool UResourceManager::HasTexture(const FString& InFilePath) const
{
	return TextureCache.find(InFilePath) != TextureCache.end();
}

/**
 * @brief 모든 텍스처 리소스를 해제하는 함수
 * 캐시된 모든 텍스처의 DirectX 리소스를 해제하고 캐시를 비움
 */
void UResourceManager::ReleaseAllTextures()
{
	for (auto& Pair : TextureCache)
	{
		if (Pair.second)
		{
			Pair.second->Release();
		}
	}
	TextureCache.clear();
}

/**
 * @brief 파일에서 DirectX 텍스처를 생성하는 내부 함수
 * DirectXTK의 WICTextureLoader를 사용
 * @param InFilePath 로드할 이미지 파일의 경로
 * @return 성공시 ID3D11ShaderResourceView 포인터, 실패시 nullptr
 */
ID3D11ShaderResourceView* UResourceManager::CreateTextureFromFile(const FString& InFilePath)
{
	URenderer& Renderer = URenderer::GetInstance();
	ID3D11Device* Device = Renderer.GetDevice();
	ID3D11DeviceContext* DeviceContext = Renderer.GetDeviceContext();

	if (!Device || !DeviceContext)
	{
		UE_LOG_ERROR("ResourceManager: Texture 생성 실패 - Device 또는 DeviceContext가 null입니다");
		return nullptr;
	}

	// 파일 확장자에 따라 적절한 로더 선택
	FString FileExtension = InFilePath.substr(InFilePath.find_last_of('.'));
	transform(FileExtension.begin(), FileExtension.end(), FileExtension.begin(), ::tolower);

	ID3D11ShaderResourceView* TextureSRV = nullptr;
	HRESULT ResultHandle;

	try
	{
		if (FileExtension == ".dds")
		{
			// DDS 파일은 DDSTextureLoader 사용
			ResultHandle = DirectX::CreateDDSTextureFromFile(
				Device,
				DeviceContext,
				StringToWideString(InFilePath).data(),
				nullptr,
				&TextureSRV
			);

			if (SUCCEEDED(ResultHandle))
			{
				UE_LOG_SUCCESS("ResourceManager: DDS 텍스처 로드 성공 - %s", InFilePath.data());
			}
			else
			{
				UE_LOG_ERROR("ResourceManager: DDS 텍스처 로드 실패 - %s (HRESULT: 0x%08lX)", InFilePath.data(), ResultHandle);
			}
		}
		else
		{
			// 기타 포맷은 WICTextureLoader 사용 (PNG, JPG, BMP, TIFF 등)
			ResultHandle = DirectX::CreateWICTextureFromFile(
				Device,
				DeviceContext,
				StringToWideString(InFilePath).data(),
				nullptr, // 텍스처 리소스는 필요 없음
				&TextureSRV
			);

			if (SUCCEEDED(ResultHandle))
			{
				UE_LOG_SUCCESS("ResourceManager: WIC 텍스처 로드 성공 - %s", InFilePath.data());
			}
			else
			{
				UE_LOG_ERROR("ResourceManager: WIC 텍스처 로드 실패 - %s (HRESULT: 0x%08lX)", InFilePath.data(), ResultHandle);
			}
		}
	}
	catch (const exception& Exception)
	{
		UE_LOG_ERROR("ResourceManager: 텍스처 로드 중 예외 발생 - %s: %s", InFilePath.data(), Exception.what());
		return nullptr;
	}

	return SUCCEEDED(ResultHandle) ? TextureSRV : nullptr;
}

/**
 * @brief 메모리 데이터에서 DirectX 텍스처를 생성하는 함수
 * DirectXTK의 WICTextureLoader와 DDSTextureLoader를 사용하여 메모리 데이터에서 텍스처 생성
 * @param InData 이미지 데이터의 포인터
 * @param InDataSize 데이터의 크기 (Byte)
 * @return 성공시 ID3D11ShaderResourceView 포인터, 실패시 nullptr
 * @note DDS 포맷 감지를 위해 매직 넘버를 확인하고 적절한 로더 선택
 * @note 네트워크에서 다운로드한 이미지나 리소스 팩에서 추출한 데이터 처리에 유용
 */
ID3D11ShaderResourceView* UResourceManager::CreateTextureFromMemory(const void* InData, size_t InDataSize)
{
	if (!InData || InDataSize == 0)
	{
		UE_LOG_ERROR("ResourceManager: 메모리 텍스처 생성 실패 - 잘못된 데이터");
		return nullptr;
	}

	URenderer& Renderer = URenderer::GetInstance();
	ID3D11Device* Device = Renderer.GetDevice();
	ID3D11DeviceContext* DeviceContext = Renderer.GetDeviceContext();

	if (!Device || !DeviceContext)
	{
		UE_LOG_ERROR("ResourceManager: 메모리 텍스처 생성 실패 - Device 또는 DeviceContext가 null입니다");
		return nullptr;
	}

	ID3D11ShaderResourceView* TextureSRV = nullptr;
	HRESULT ResultHandle;

	try
	{
		// DDS 매직 넘버 확인 (DDS 파일은 "DDS " 로 시작)
		const uint32 DDS_MAGIC = 0x20534444; // "DDS " in little-endian
		bool bIsDDS = (InDataSize >= 4 && *static_cast<const uint32*>(InData) == DDS_MAGIC);

		if (bIsDDS)
		{
			// DDS 데이터는 DDSTextureLoader 사용
			ResultHandle = DirectX::CreateDDSTextureFromMemory(
				Device,
				DeviceContext,
				static_cast<const uint8*>(InData),
				InDataSize,
				nullptr, // 텍스처 리소스는 필요 없음
				&TextureSRV
			);

			if (SUCCEEDED(ResultHandle))
			{
				UE_LOG_SUCCESS("ResourceManager: DDS 메모리 텍스처 생성 성공 (크기: %zu bytes)", InDataSize);
			}
			else
			{
				UE_LOG_ERROR("ResourceManager: DDS 메모리 텍스처 생성 실패 (HRESULT: 0x%08lX)", ResultHandle);
			}
		}
		else
		{
			// 기타 포맷은 WICTextureLoader 사용 (PNG, JPG, BMP, TIFF 등)
			ResultHandle = DirectX::CreateWICTextureFromMemory(
				Device,
				DeviceContext,
				reinterpret_cast<const uint8*>(InData),
				InDataSize,
				nullptr, // 텍스처 리소스는 필요 없음
				&TextureSRV
			);

			if (SUCCEEDED(ResultHandle))
			{
				UE_LOG_SUCCESS("ResourceManager: WIC 메모리 텍스처 생성 성공 (크기: %zu bytes)", InDataSize);
			}
			else
			{
				UE_LOG_ERROR("ResourceManager: WIC 메모리 텍스처 생성 실패 (HRESULT: 0x%08lX)", ResultHandle);
			}
		}
	}
	catch (const exception& Exception)
	{
		UE_LOG_ERROR("ResourceManager: 메모리 텍스처 생성 중 예외 발생: %s", Exception.what());
		return nullptr;
	}

	return SUCCEEDED(ResultHandle) ? TextureSRV : nullptr;
}
