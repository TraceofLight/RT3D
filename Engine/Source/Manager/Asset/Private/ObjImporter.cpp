#include "pch.h"

#include "Runtime/CoreUObject/Public/WindowsBinReader.h"
#include "Runtime/CoreUObject/Public/WindowsBinWriter.h"
#include "Manager/Asset/Public/ObjImporter.h"
#include "Manager/Path/Public/PathManager.h"

bool FObjImporter::LoadObj(const std::filesystem::path& FilePath, FObjInfo* OutObjInfo, Configuration Config)
{
	if (!OutObjInfo)
	{
		return false;
	}

	if (!std::filesystem::exists(FilePath))
	{
		UE_LOG_ERROR("파일을 찾지 못했습니다: %s", FilePath.string().c_str());
		return false;
	}

	// objbin 파일 경로를 Data/Cooked 폴더에 생성
	path CookedPath = UPathManager::GetInstance().GetCookedPath();
	path BinFilePath = CookedPath / (FilePath.stem().wstring() + L".objbin");

	if (Config.bIsBinaryEnabled && std::filesystem::exists(BinFilePath))
	{
		auto ObjTime = std::filesystem::last_write_time(FilePath);
		auto BinTime = std::filesystem::last_write_time(BinFilePath);

		if (BinTime >= ObjTime)
		{
			UE_LOG_SUCCESS("ObjCache: Loaded cached objbin '%ls'", BinFilePath.c_str());
			FWindowsBinReader WindowsBinReader(BinFilePath);
			WindowsBinReader << *OutObjInfo;

			return true;
		}
		else
		{
			UE_LOG_INFO("ObjCache: objbin outdated, reloading from obj '%ls'", FilePath.c_str());
		}
	}

	if (FilePath.extension() != ".obj")
	{
		UE_LOG_ERROR("잘못된 파일 확장자입니다: %ls", FilePath.c_str());
		return false;
	}

	std::ifstream File(FilePath);
	if (!File)
	{
		UE_LOG_ERROR("파일을 열지 못했습니다: %ls", FilePath.c_str());
		return false;
	}

	size_t FaceCount = 0;

	TOptional<FObjectInfo> OptObjectInfo;

	FString Buffer;
	while (std::getline(File, Buffer))
	{
		std::istringstream Tokenizer(Buffer);
		FString Prefix;

		Tokenizer >> Prefix;

		// ========================== Vertex Information ============================ //

		/** Vertex Position */
		if (Prefix == "v")
		{
			FVector Position;
			if (!(Tokenizer >> Position.X >> Position.Y >> Position.Z))
			{
				UE_LOG_ERROR("정점 위치 형식이 잘못되었습니다");
				return false;
			}

			if (Config.bPositionToUEBasis)
			{
				OutObjInfo->VertexList.Emplace(PositionToUEBasis(Position));
			}
			else
			{
				OutObjInfo->VertexList.Emplace(Position);
			}

		}
		/** Vertex Normal */
		else if (Prefix == "vn")
		{
			FVector Normal;
			if (!(Tokenizer >> Normal.X >> Normal.Y >> Normal.Z))
			{
				UE_LOG_ERROR("정점 법선 형식이 잘못되었습니다");
				return false;
			}

			if (Config.bNormalToUEBasis)
			{
				OutObjInfo->NormalList.Emplace(NormalToUEBasis(Normal));
			}
			else
			{
				OutObjInfo->NormalList.Emplace(Normal);
			}
		}
		/** Texture Coordinate */
		else if (Prefix == "vt")
		{
			/** @note: Ignore 3D Texture */
			FVector2 TexCoord;
			if (!(Tokenizer >> TexCoord.X >> TexCoord.Y))
			{
				UE_LOG_ERROR("정점 텍스쳐 좌표 형식이 잘못되었습니다");
				return false;
			}

			if (Config.bUVToUEBasis)
			{
				OutObjInfo->TexCoordList.Emplace(UVToUEBasis(TexCoord));
			}
			else
			{
				OutObjInfo->TexCoordList.Emplace(TexCoord);
			}
		}

		// =========================== Group Information ============================ //

		/** Object Information */
		else if (Prefix == "o")
		{
			if (!Config.bIsObjectEnabled)
			{
				continue; // Ignore 'o' prefix
			}

			if (OptObjectInfo)
			{
				OutObjInfo->ObjectInfoList.Emplace(std::move(*OptObjectInfo));
			}

			FString ObjectName;
			if (!(Tokenizer >> ObjectName))
			{
				UE_LOG_ERROR("오브젝트 이름 형식이 잘못되었습니다");
				return false;
			}
			OptObjectInfo.emplace();
			OptObjectInfo->Name = std::move(ObjectName);

			FaceCount = 0;
		}

		/** Group Information */
		else if (Prefix == "g")
		{
			if (!OptObjectInfo)
			{
				OptObjectInfo.emplace();
				OptObjectInfo->Name = Config.DefaultName;
			}

			FString GroupName;
			if (!(Tokenizer >> GroupName))
			{
				UE_LOG_ERROR("잘못된 그룹 이름 형식입니다");
				return false;
			}

			OptObjectInfo->GroupNameList.Emplace(std::move(GroupName));
			OptObjectInfo->GroupIndexList.Emplace(FaceCount);
		}

		// ============================ Face Information ============================ //

		/** Face Information */
		else if (Prefix == "f")
		{
			if (!OptObjectInfo)
			{
				OptObjectInfo.emplace();
				OptObjectInfo->Name = Config.DefaultName;
			}

			TArray<FString> FaceBuffers;
			FString FaceBuffer;
			while (Tokenizer >> FaceBuffer)
			{
				FaceBuffers.Emplace(FaceBuffer);
			}

			if (FaceBuffers.Num() < 2)
			{
				UE_LOG_ERROR("면 형식이 잘못되었습니다");
				return false;
			}

			/** @todo: 오목 다각형에 대한 지원 필요, 현재는 볼록 다각형만 지원 */
			for (int32 i = 1; i + 1 < FaceBuffers.Num(); ++i)
			{
				if (Config.bFlipWindingOrder)
				{
					if (!ParseFaceBuffer(FaceBuffers[0], &(*OptObjectInfo)))
					{
						UE_LOG_ERROR("면 파싱에 실패했습니다");
						return false;
					}

					if (!ParseFaceBuffer(FaceBuffers[i + 1], &(*OptObjectInfo)))
					{
						UE_LOG_ERROR("면 파싱에 실패했습니다");
						return false;
					}

					if (!ParseFaceBuffer(FaceBuffers[i], &(*OptObjectInfo)))
					{
						UE_LOG_ERROR("면 파싱에 실패했습니다");
						return false;
					}
				}
				else
				{
					if (!ParseFaceBuffer(FaceBuffers[0], &(*OptObjectInfo)))
					{
						UE_LOG_ERROR("면 파싱에 실패했습니다");
						return false;
					}

					if (!ParseFaceBuffer(FaceBuffers[i], &(*OptObjectInfo)))
					{
						UE_LOG_ERROR("면 파싱에 실패했습니다");
						return false;
					}

					if (!ParseFaceBuffer(FaceBuffers[i + 1], &(*OptObjectInfo)))
					{
						UE_LOG_ERROR("면 파싱에 실패했습니다");
						return false;
					}
				}
				++FaceCount;
			}
		}

		// ============================ Material Information ============================ //

		else if (Prefix == "mtllib")
		{
			/** @todo: Parse material data */
			/** @todo: Support relative path from .obj file to find .mtl file */
			FString MaterialFileName;
			Tokenizer >> MaterialFileName;

			std::filesystem::path MaterialFilePath = FilePath.parent_path() / MaterialFileName;

			MaterialFilePath = std::filesystem::weakly_canonical(MaterialFilePath);

			if (!LoadMaterial(MaterialFilePath, OutObjInfo))
			{
				UE_LOG_ERROR("머티리얼을 불러오는데 실패했습니다: %ls", MaterialFilePath.c_str());
				return false;
			}
		}

		else if (Prefix == "usemtl")
		{
			FString MaterialName;
			Tokenizer >> MaterialName;

			if (!OptObjectInfo)
			{
				OptObjectInfo.emplace();
				OptObjectInfo->Name = Config.DefaultName;
			}

			OptObjectInfo->MaterialNameList.Emplace(std::move(MaterialName));
			OptObjectInfo->MaterialIndexList.Emplace(FaceCount);
		}
	}

	if (OptObjectInfo)
	{
		OutObjInfo->ObjectInfoList.Emplace(std::move(*OptObjectInfo));
	}

	// objbin 저장 (캐싱)
	if (Config.bIsBinaryEnabled)
	{
		// Cooked 폴더가 이미 PathManager에 의해 생성되어 있음
		FWindowsBinWriter WindowsBinWriter(BinFilePath);
		WindowsBinWriter << *OutObjInfo;
		UE_LOG_SUCCESS("ObjCache: Saved objbin '%ls'", BinFilePath.c_str());
	}

	return true;
}

bool FObjImporter::LoadMaterial(const std::filesystem::path& FilePath, FObjInfo* OutObjInfo)
{
	if (!OutObjInfo)
	{
		return false;
	}

	if (!std::filesystem::exists(FilePath))
	{
		UE_LOG_ERROR("파일을 찾지 못했습니다: %ls", FilePath.c_str());
		return false;
	}

	if (FilePath.extension() != ".mtl")
	{
		UE_LOG_ERROR("잘못된 파일 확장자입니다: %ls", FilePath.c_str());
		return false;
	}

	std::ifstream File(FilePath);
	if (!File)
	{
		UE_LOG_ERROR("파일을 열지 못했습니다: %ls", FilePath.c_str());
		return false;
	}

	TOptional<FObjectMaterialInfo> OptMaterialInfo;

	FString Buffer;
	while (std::getline(File, Buffer))
	{
		std::istringstream Tokenizer(Buffer);
		FString Prefix;

		Tokenizer >> Prefix;

		if (Prefix == "newmtl")
		{
			if (OptMaterialInfo)
			{
				OutObjInfo->ObjectMaterialInfoList.Emplace(std::move(*OptMaterialInfo));
			}

			OptMaterialInfo.emplace();
			if (!(Tokenizer >> OptMaterialInfo->Name))
			{
				UE_LOG_ERROR("머티리얼 이름 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "Ns")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			if (!(Tokenizer >> OptMaterialInfo->Ns))
			{
				UE_LOG_ERROR("Ns(광택) 속성 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "Ka")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			if (!(Tokenizer >> OptMaterialInfo->Ka.X >> OptMaterialInfo->Ka.Y >> OptMaterialInfo->Ka.Z))
			{
				UE_LOG_ERROR("Ka(주변) 속성 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "Kd")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			if (!(Tokenizer >> OptMaterialInfo->Kd.X >> OptMaterialInfo->Kd.Y >> OptMaterialInfo->Kd.Z))
			{
				UE_LOG_ERROR("Kd(분산) 속성 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "Ks")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			if (!(Tokenizer >> OptMaterialInfo->Ks.X >> OptMaterialInfo->Ks.Y >> OptMaterialInfo->Ks.Z))
			{
				UE_LOG_ERROR("Ks(반사) 속성 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "Ke")
		{
			// Ke is not in FObjectMaterialInfo, skipping
		}
		else if (Prefix == "Ni")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			if (!(Tokenizer >> OptMaterialInfo->Ni))
			{
				UE_LOG_ERROR("Ni(굴절률) 속성 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "d")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			if (!(Tokenizer >> OptMaterialInfo->D))
			{
				UE_LOG_ERROR("d(투명도) 속성 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "Tr")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}
			float Tr;
			if (!(Tokenizer >> Tr))
			{
				UE_LOG_ERROR("Tr(투명도) 속성 형식이 잘못되었습니다");
				return false;
			}
			OptMaterialInfo->D = 1.0f - Tr;
		}
		else if (Prefix == "Tf")
		{
			// Tf is not in FObjectMaterialInfo, skipping
		}
		else if (Prefix == "illum")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			if (!(Tokenizer >> OptMaterialInfo->Illumination))
			{
				UE_LOG_ERROR("illum(조명 모델) 속성 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "map_Ka")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			if (!(Tokenizer >> OptMaterialInfo->KaMap))
			{
				UE_LOG_ERROR("map_Ka(주변 텍스처) 속성 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "map_Kd")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			if (!(Tokenizer >> OptMaterialInfo->KdMap))
			{
				UE_LOG_ERROR("map_Kd(분산 텍스처) 속성 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "map_Ks")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			if (!(Tokenizer >> OptMaterialInfo->KsMap))
			{
				UE_LOG_ERROR("map_Ks(반사 텍스처) 속성 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "map_Ns")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			if (!(Tokenizer >> OptMaterialInfo->NsMap))
			{
				UE_LOG_ERROR("map_Ns(광택 텍스처) 속성 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "map_d")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			if (!(Tokenizer >> OptMaterialInfo->DMap))
			{
				UE_LOG_ERROR("map_d(투명도 텍스처) 속성 형식이 잘못되었습니다");
				return false;
			}
		}
		else if (Prefix == "map_Bump" || Prefix == "bump")
		{
			if (!OptMaterialInfo)
			{
				UE_LOG_ERROR("머티리얼이 정의되지 않았습니다");
				return false;
			}

			FString Token;
			FString BumpPath;
			float BumpScaleValue = 1.0f;

			while (Tokenizer >> Token)
			{
				if (Token == "-bm")
				{
					FString ScaleString;
					if (!(Tokenizer >> ScaleString))
					{
						UE_LOG_ERROR("map_Bump -bm 옵션 값이 없습니다");
						return false;
					}
					try
					{
						BumpScaleValue = std::stof(ScaleString);
					}
					catch ([[maybe_unused]] const std::invalid_argument& Exception)
					{
						UE_LOG_ERROR("map_Bump -bm 옵션 값 형식이 잘못되었습니다");
						return false;
					}
					continue;
				}

				if (!Token.empty() && Token[0] == '-')
				{
					// 다른 옵션들은 현재 무시
					continue;
				}

				BumpPath = Token;
				break;
			}

			if (BumpPath.empty())
			{
				UE_LOG_ERROR("map_Bump(범프 텍스처) 경로가 없습니다");
				return false;
			}

			OptMaterialInfo->BumpMap = BumpPath;

			// TODO(선택): -bm을 저장하려면 ObjImporter.h에 BumpScale 필드를 추가하고 아래 대입을 활성화
			// 노멀맵만 사용한다면 굳이 필요 없음!
			// OptMaterialInfo->BumpScale = BumpScaleValue;
			/*
				 선택 사항 1: Bump 슬롯(t5)도 실제로 바인딩하려면
					  - 파일: Engine/Source/Render/RenderPass/Private/StaticMeshPass.cpp:214 다음에 추가
						코드(추가)
						if (UTexture* BumpTexture = Material->GetBumpTexture())
						{
						Pipeline->SetTexture(5, false, BumpTexture->GetTextureSRV());
						// Pipeline->SetSamplerState(5, false, BumpTexture->GetTextureSampler()); // 필요 시
						}

				  선택 사항 2: -bm 스케일까지 실제 반영하려면
				  - 저장 경로 추가
					  - 파일: Engine/Source/Manager/Asset/Public/ObjImporter.h:126 다음 줄에 필드 추가
						코드(추가)
						float BumpScale = 1.0f;
					  - 파일: Engine/Source/Manager/Asset/Public/ObjImporter.h:145 다음 줄에 직렬화 항목 추가
						코드(추가)
						Ar << ObjectMaterialInfo.BumpScale;
					  - 필요 시 Engine/Source/Texture/Public/Material.h에도 FMaterial과 UMaterial에 BumpScale 필드/Getter/Setter 추가
				  - 상수 버퍼 전달
					  - 파일: Engine/Source/Global/CoreTypes.h:38의 struct FMaterialConstants에 float BumpScale; 추가(정렬 맞추기 위해
						패딩 조정 필요할 수 있음)
					  - 셰이더 상수 버퍼(Engine/Asset/Shader/*hlsl의 cbuffer MaterialConstants)에도 float BumpScale; 추가
					  - 파일: Engine/Source/Render/RenderPass/Private/StaticMeshPass.cpp에서 MaterialConstants.BumpScale =
						Material->GetBumpScale(); 설정
					  - HLSL에서 노말 강도 조절(예: 노말맵 사용 시 normal.xy *= BumpScale; normal = normalize(normal);) 또는 높이맵 미분
						계산 시 스케일 반영
			*/
		}
	}

	if (OptMaterialInfo)
	{
		OutObjInfo->ObjectMaterialInfoList.Emplace(std::move(*OptMaterialInfo));
	}

	return true;
}

bool FObjImporter::ParseFaceBuffer(const FString& FaceBuffer, FObjectInfo* OutObjectInfo)
{
	/** Ignore data when ObjInfo is nullptr */
	if (!OutObjectInfo)
	{
		return false;
	}

	TArray<FString> IndexBuffers;
	std::istringstream Tokenizer(FaceBuffer);
	FString IndexBuffer;
	while (std::getline(Tokenizer, IndexBuffer, '/'))
	{
		IndexBuffers.Emplace(IndexBuffer);
	}

	if (IndexBuffers.IsEmpty())
	{
		UE_LOG_ERROR("면 형식이 잘못되었습니다");
		return false;
	}

	if (IndexBuffers[0].empty())
	{
		UE_LOG_ERROR("정점 위치 형식이 잘못되었습니다");
		return false;
	}

	try
	{
		OutObjectInfo->VertexIndexList.Add(std::stoull(IndexBuffers[0]) - 1);
	}
	catch ([[maybe_unused]] const std::invalid_argument& Exception)
	{
		UE_LOG_ERROR("정점 위치 인덱스 형식이 잘못되었습니다");
		return false;
	}

	switch (IndexBuffers.Num())
	{
	case 1:
		/** @brief: Only position data (e.g., 'f 1 2 3') */
		break;
	case 2:
		/** @brief: Position and texture coordinate data (e.g., 'f 1/1 2/1') */
		if (IndexBuffers[1].empty())
		{
			UE_LOG_ERROR("정점 텍스쳐 좌표 인덱스 형식이 잘못되었습니다");
			return false;
		}

		try
		{
			OutObjectInfo->TexCoordIndexList.Add(std::stoull(IndexBuffers[1]) - 1);
		}
		catch ([[maybe_unused]] const std::invalid_argument& Exception)
		{
			UE_LOG_ERROR("정점 텍스쳐 좌표 인덱스 형식이 잘못되었습니다");
			return false;
		}
		break;
	case 3:
		/** @brief: Position, texture coordinate and vertex normal data (e.g., 'f 1/1/1 2/2/1' or 'f 1//1 2//1') */
		if (IndexBuffers[1].empty()) /** Position and vertex normal */
		{
			if (IndexBuffers[2].empty())
			{
				UE_LOG_ERROR("정점 법선 인덱스 형식이 잘못되었습니다");
				return false;
			}

			try
			{
				OutObjectInfo->NormalIndexList.Add(std::stoull(IndexBuffers[2]) - 1);
			}
			catch ([[maybe_unused]] const std::invalid_argument& Exception)
			{
				UE_LOG_ERROR("정점 법선 인덱스 형식이 잘못되었습니다");
				return false;
			}
		}
		else /** Position, texture coordinate, and vertex normal */
		{
			if (IndexBuffers[1].empty() || IndexBuffers[2].empty())
			{
				UE_LOG_ERROR("정점 텍스쳐 좌표 또는 법선 인덱스 형식이 잘못되었습니다");
				return false;
			}

			try
			{
				OutObjectInfo->TexCoordIndexList.Add(std::stoull(IndexBuffers[1]) - 1);
				OutObjectInfo->NormalIndexList.Add(std::stoull(IndexBuffers[2]) - 1);
			}
			catch ([[maybe_unused]] const std::invalid_argument& Exception)
			{
				UE_LOG_ERROR("정점 텍스쳐 좌표 또는 법선 인덱스 형식이 잘못되었습니다");
				return false;
			}
		}
		break;
	}

	return true;
}

/**
 * @brief FObjInfo 구조체를 .obj 파일로 저장합니다.
 * @param FilePath 저장할 .obj 파일의 절대 또는 상대 경로
 * @param ObjInfo 저장할 메쉬 데이터를 담고 있는 FObjInfo 구조체 포인터
 * @param Config Export 옵션 (winding order 변환, 좌표계 변환 등)
 * @return 파일 저장 성공 시 true, 실패 시 false
 */
bool FObjImporter::SaveObj(const path& FilePath, const FObjInfo* ObjInfo, Configuration Config)
{
	if (!ObjInfo)
	{
		UE_LOG_ERROR("ObjInfo is null");
		return false;
	}

	std::ofstream File(FilePath);
	if (!File)
	{
		UE_LOG_ERROR("파일을 생성하지 못했습니다: %ls", FilePath.c_str());
		return false;
	}

	File << "# Exported by FutureEngine ObjImporter\n";
	File << "# Vertices: " << ObjInfo->VertexList.Num() << "\n";
	File << "# Normals: " << ObjInfo->NormalList.Num() << "\n";
	File << "# TexCoords: " << ObjInfo->TexCoordList.Num() << "\n\n";

	// MTL 파일 참조 (머티리얼이 있는 경우)
	if (!ObjInfo->ObjectMaterialInfoList.IsEmpty())
	{
		path MtlFileName = FilePath.stem().wstring() + L".mtl";
		File << "mtllib " << MtlFileName.string() << "\n\n";

		// MTL 파일 저장
		path MtlFilePath = FilePath.parent_path() / MtlFileName;
		if (!SaveMaterial(MtlFilePath, ObjInfo))
		{
			UE_LOG_WARNING("머티리얼 저장 실패: %ls", MtlFilePath.c_str());
		}
	}

	// Vertex positions
	for (const FVector& Vertex : ObjInfo->VertexList)
	{
		FVector OutVertex = Vertex;
		if (Config.bPositionToUEBasis)
		{
			// UE basis에서 다시 원본으로 변환 (Y축 반전)
			OutVertex = FVector(Vertex.X, -Vertex.Y, Vertex.Z);
		}
		File << "v " << OutVertex.X << " " << OutVertex.Y << " " << OutVertex.Z << "\n";
	}
	File << "\n";

	// Texture coordinates
	for (const FVector2& TexCoord : ObjInfo->TexCoordList)
	{
		FVector2 OutTexCoord = TexCoord;
		if (Config.bUVToUEBasis)
		{
			// UE basis에서 다시 원본으로 변환 (V축 반전)
			OutTexCoord = FVector2(TexCoord.X, 1.0f - TexCoord.Y);
		}
		File << "vt " << OutTexCoord.X << " " << OutTexCoord.Y << "\n";
	}
	File << "\n";

	// Vertex normals
	for (const FVector& Normal : ObjInfo->NormalList)
	{
		FVector OutNormal = Normal;
		if (Config.bNormalToUEBasis)
		{
			// UE basis에서 다시 원본으로 변환 (Y축 반전)
			OutNormal = FVector(Normal.X, -Normal.Y, Normal.Z);
		}
		File << "vn " << OutNormal.X << " " << OutNormal.Y << " " << OutNormal.Z << "\n";
	}
	File << "\n";

	// Objects and faces
	for (const FObjectInfo& ObjectInfo : ObjInfo->ObjectInfoList)
	{
		if (!ObjectInfo.Name.empty())
		{
			File << "o " << ObjectInfo.Name << "\n";
		}

		bool bHasTexCoords = !ObjectInfo.TexCoordIndexList.IsEmpty();
		bool bHasNormals = !ObjectInfo.NormalIndexList.IsEmpty();

		int32 FaceCount = ObjectInfo.VertexIndexList.Num() / 3;
		int32 CurrentMaterialIndex = 0;

		for (int32 FaceIndex = 0; FaceIndex < FaceCount; ++FaceIndex)
		{
			// 머티리얼 변경 확인
			if (CurrentMaterialIndex < ObjectInfo.MaterialIndexList.Num())
			{
				if (FaceIndex >= static_cast<int32>(ObjectInfo.MaterialIndexList[CurrentMaterialIndex]))
				{
					File << "usemtl " << ObjectInfo.MaterialNameList[CurrentMaterialIndex] << "\n";
					++CurrentMaterialIndex;
				}
			}

			File << "f";

			// Winding order 처리
			TArray<size_t> VertexOrder = {0, 1, 2};
			if (Config.bFlipWindingOrder)
			{
				// Winding order 뒤집기: [0, 1, 2] -> [0, 2, 1]
				VertexOrder = {0, 2, 1};
			}

			for (size_t LocalVertexIndex : VertexOrder)
			{
				size_t GlobalVertexIndex = FaceIndex * 3 + LocalVertexIndex;

				size_t VertexIndex = ObjectInfo.VertexIndexList[static_cast<int32>(GlobalVertexIndex)] + 1; // OBJ는 1-based index

				File << " " << VertexIndex;

				if (bHasTexCoords || bHasNormals)
				{
					File << "/";
					if (bHasTexCoords)
					{
						size_t TexCoordIndex = ObjectInfo.TexCoordIndexList[static_cast<int32>(GlobalVertexIndex)] + 1;
						File << TexCoordIndex;
					}

					if (bHasNormals)
					{
						File << "/";
						size_t NormalIndex = ObjectInfo.NormalIndexList[static_cast<int32>(GlobalVertexIndex)] + 1;
						File << NormalIndex;
					}
				}
			}

			File << "\n";
		}

		File << "\n";
	}

	File.close();

	UE_LOG_SUCCESS("OBJ 파일 저장 완료: %ls", FilePath.c_str());
	return true;
}

/**
 * @brief Material 정보를 .mtl 파일로 저장합니다.
 * @param FilePath 저장할 .mtl 파일의 경로
 * @param ObjInfo Material 데이터를 담고 있는 FObjInfo 구조체 포인터
 * @return Material library 저장 성공 시 true, 실패 시 false
 */
bool FObjImporter::SaveMaterial(const path& FilePath, const FObjInfo* ObjInfo)
{
	if (!ObjInfo)
	{
		return false;
	}

	std::ofstream File(FilePath);
	if (!File)
	{
		UE_LOG_ERROR("MTL 파일을 생성하지 못했습니다: %ls", FilePath.c_str());
		return false;
	}

	File << "# Exported by FutureEngine ObjImporter\n\n";

	for (const FObjectMaterialInfo& MaterialInfo : ObjInfo->ObjectMaterialInfoList)
	{
		File << "newmtl " << MaterialInfo.Name << "\n";

		// Ambient
		File << "Ka " << MaterialInfo.Ka.X << " " << MaterialInfo.Ka.Y << " " << MaterialInfo.Ka.Z << "\n";

		// Diffuse
		File << "Kd " << MaterialInfo.Kd.X << " " << MaterialInfo.Kd.Y << " " << MaterialInfo.Kd.Z << "\n";

		// Specular
		File << "Ks " << MaterialInfo.Ks.X << " " << MaterialInfo.Ks.Y << " " << MaterialInfo.Ks.Z << "\n";

		// Emissive
		if (MaterialInfo.Ke.X != 0.0f || MaterialInfo.Ke.Y != 0.0f || MaterialInfo.Ke.Z != 0.0f)
		{
			File << "Ke " << MaterialInfo.Ke.X << " " << MaterialInfo.Ke.Y << " " << MaterialInfo.Ke.Z << "\n";
		}

		// Specular exponent
		File << "Ns " << MaterialInfo.Ns << "\n";

		// Optical density
		File << "Ni " << MaterialInfo.Ni << "\n";

		// Dissolve
		File << "d " << MaterialInfo.D << "\n";

		// Illumination model
		File << "illum " << MaterialInfo.Illumination << "\n";

		// Texture maps
		if (!MaterialInfo.KaMap.empty())
		{
			File << "map_Ka " << MaterialInfo.KaMap << "\n";
		}
		if (!MaterialInfo.KdMap.empty())
		{
			File << "map_Kd " << MaterialInfo.KdMap << "\n";
		}
		if (!MaterialInfo.KsMap.empty())
		{
			File << "map_Ks " << MaterialInfo.KsMap << "\n";
		}
		if (!MaterialInfo.NsMap.empty())
		{
			File << "map_Ns " << MaterialInfo.NsMap << "\n";
		}
		if (!MaterialInfo.DMap.empty())
		{
			File << "map_d " << MaterialInfo.DMap << "\n";
		}
		if (!MaterialInfo.BumpMap.empty())
		{
			File << "map_Bump " << MaterialInfo.BumpMap << "\n";
		}

		File << "\n";
	}

	File.close();

	UE_LOG_SUCCESS("MTL 파일 저장 완료: %ls", FilePath.c_str());
	return true;
}
