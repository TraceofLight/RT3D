#include "pch.h"
#include "Manager/Asset/Public/FbxParser.h"
#include <fbxsdk.h>

// ========================================
// FSkeleton 구현
// ========================================

void FSkeleton::BuildRefPoseGlobal()
{
	int32 NumBones = GetNumBones();
	RefPoseGlobal.SetNum(NumBones);
	InvRefPoseGlobal.SetNum(NumBones);

	// Root부터 시작하여 자식으로 내려가며 계산
	for (int32 i = 0; i < NumBones; i++)
	{
		// FTransform → FMatrix 변환
		const FTransform& LocalTransform = RefPoseLocal[i];

		FMatrix ScaleMatrix = FMatrix::ScalingMatrix(LocalTransform.Scale);
		FMatrix RotMatrix = FMatrix::RotationMatrix(LocalTransform.Rotation);
		FMatrix TranslationMatrix = FMatrix::TranslationMatrix(LocalTransform.Location);

		FMatrix LocalMatrix = ScaleMatrix * RotMatrix * TranslationMatrix;

		if (Parents[i] == -1)
		{
			// Root Bone
			RefPoseGlobal[i] = LocalMatrix;
		}
		else
		{
			// Child Bone: Global = Local * ParentGlobal
			RefPoseGlobal[i] = LocalMatrix * RefPoseGlobal[Parents[i]];
		}

		// 역행렬 계산 (Skinning용)
		InvRefPoseGlobal[i] = RefPoseGlobal[i].Inverse();
	}
}

int32 FSkeleton::FindBoneIndex(const FName& BoneName) const
{
	for (int32 i = 0; i < BoneNames.Num(); i++)
	{
		if (BoneNames[i] == BoneName)
		{
			return i;
		}
	}
	return -1;
}

// ========================================
// FFbxParser 구현
// ========================================

FFbxParser::FFbxParser()
{
	Initialize();
}

FFbxParser::~FFbxParser()
{
	Cleanup();
}

void FFbxParser::Initialize()
{
	Manager = FbxManager::Create();
	if (!Manager)
	{
		UE_LOG_ERROR("FbxParser: FbxManager 생성 실패");
		return;
	}

	FbxIOSettings* IOSettings = FbxIOSettings::Create(Manager, IOSROOT);
	Manager->SetIOSettings(IOSettings);

	UE_LOG("FbxParser: 초기화 완료 (SDK Version: %s)", Manager->GetVersion());
}

void FFbxParser::Cleanup()
{
	if (Manager)
	{
		Manager->Destroy();
		Manager = nullptr;
	}
	Scene = nullptr;
}

/**
 * FBX 파일을 로드하여 Skeletal Mesh 데이터 추출
 * @param FilePath FBX 파일 경로
 * @param OutMesh 출력될 Skeletal Mesh
 * @return 성공 여부
 */
bool FFbxParser::LoadSkeletalMesh(const FString& FilePath, FSkeletalMesh& OutMesh)
{
	if (!Manager)
	{
		UE_LOG_ERROR("FbxParser: Manager가 초기화되지 않음");
		return false;
	}

	// Scene 임포트
	if (!ImportScene(FilePath))
	{
		return false;
	}

	// 좌표계 변환: Y-up → Z-up, Right-handed → Left-handed
	FbxAxisSystem TargetAxisSystem(FbxAxisSystem::eZAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eLeftHanded);
	FbxAxisSystem SceneAxisSystem = Scene->GetGlobalSettings().GetAxisSystem();
	if (SceneAxisSystem != TargetAxisSystem)
	{
		TargetAxisSystem.ConvertScene(Scene);
	}

	// 단위 변환 (센티미터로 통일)
	FbxSystemUnit SceneSystemUnit = Scene->GetGlobalSettings().GetSystemUnit();
	if (SceneSystemUnit != FbxSystemUnit::cm)
	{
		FbxSystemUnit::cm.ConvertScene(Scene);
	}

	// 삼각형화 (모든 폴리곤을 삼각형으로)
	FbxGeometryConverter Converter(Manager);
	Converter.Triangulate(Scene, true);

	// Scene 처리
	OutMesh.PathFileName = FName(FilePath);
	ProcessScene(OutMesh);

	if (!OutMesh.IsValid())
	{
		UE_LOG_ERROR("FbxParser: 유효한 Skeletal Mesh를 찾지 못함 (%s)", FilePath.c_str());
		return false;
	}

	UE_LOG("FbxParser: Skeletal Mesh 로드 성공 (%s)", FilePath.c_str());
	return true;
}

bool FFbxParser::ImportScene(const FString& FilePath)
{
	Scene = FbxScene::Create(Manager, "ImportScene");
	if (!Scene)
	{
		UE_LOG_ERROR("FbxParser: Scene 생성 실패");
		return false;
	}

	FbxImporter* Importer = FbxImporter::Create(Manager, "");
	if (!Importer->Initialize(FilePath.c_str(), -1, Manager->GetIOSettings()))
	{
		UE_LOG_ERROR("FbxParser: Importer 초기화 실패: %s", Importer->GetStatus().GetErrorString());
		Importer->Destroy();
		return false;
	}

	if (!Importer->Import(Scene))
	{
		UE_LOG_ERROR("FbxParser: Scene Import 실패");
		Importer->Destroy();
		return false;
	}

	Importer->Destroy();
	return true;
}

void FFbxParser::ProcessScene(FSkeletalMesh& OutMesh) const
{
	FbxNode* RootNode = Scene->GetRootNode();
	if (!RootNode)
	{
		return;
	}

	ProcessNode(RootNode, OutMesh);
}

void FFbxParser::ProcessNode(FbxNode* Node, FSkeletalMesh& OutMesh)
{
	if (!Node)
	{
		return;
	}

	// Mesh 노드 확인
	FbxNodeAttribute* Attribute = Node->GetNodeAttribute();
	if (Attribute && Attribute->GetAttributeType() == FbxNodeAttribute::eMesh)
	{
		FbxMesh* Mesh = Node->GetMesh();
		if (Mesh && IsMeshSkinned(Mesh))
		{
			ProcessSkeletalMesh(Node, OutMesh);
			return; // 첫 번째 Skeletal Mesh만 로드
		}
	}

	// 자식 노드 순회
	for (int i = 0; i < Node->GetChildCount(); i++)
	{
		ProcessNode(Node->GetChild(i), OutMesh);
	}
}

bool FFbxParser::ProcessSkeletalMesh(FbxNode* MeshNode, FSkeletalMesh& OutMesh)
{
	FbxMesh* Mesh = MeshNode->GetMesh();
	if (!Mesh)
	{
		return false;
	}

	UE_LOG("FbxParser: Skeletal Mesh 처리 시작 (%s)", MeshNode->GetName());

	// Skeleton 생성 및 빌드
	OutMesh.Skeleton = new FSkeleton();
	BuildSkeleton(Mesh, *OutMesh.Skeleton);

	// Mesh Section 빌드
	BuildMeshSections(Mesh, OutMesh);

	// Material 추출
	ExtractMaterials(MeshNode, OutMesh);

	// RefPoseGlobal 계산
	OutMesh.Skeleton->BuildRefPoseGlobal();

	UE_LOG("FbxParser: Skeletal Mesh 처리 완료 (Bones: %d, Sections: %d)",
		OutMesh.Skeleton->GetNumBones(), OutMesh.Sections.Num());

	return true;
}

void FFbxParser::BuildSkeleton(const FbxMesh* Mesh, FSkeleton& OutSkeleton)
{
	FbxSkin* Skin = GetSkin(Mesh);
	if (!Skin)
	{
		return;
	}

	// 모든 Bone 노드 수집
	TArray<FbxNode*> BoneNodes;
	CollectBones(Skin, BoneNodes);

	if (BoneNodes.Num() == 0)
	{
		UE_LOG_WARNING("FbxParser: Bone이 없음");
		return;
	}

	// Bone 계층 구조 빌드
	BuildBoneHierarchy(BoneNodes, OutSkeleton);

	UE_LOG("FbxParser: Skeleton 빌드 완료 (Bones: %d)", OutSkeleton.GetNumBones());
}

void FFbxParser::CollectBones(FbxNode* Node, TArray<FbxNode*>& OutBoneNodes)
{
	// FbxNode를 재귀적으로 탐색하여 Skeleton Bone 수집 (미사용 - FbxSkin 기반 수집 사용)
	if (!Node)
	{
		return;
	}

	FbxNodeAttribute* Attribute = Node->GetNodeAttribute();
	if (Attribute && Attribute->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		OutBoneNodes.Add(Node);
	}

	for (int i = 0; i < Node->GetChildCount(); i++)
	{
		CollectBones(Node->GetChild(i), OutBoneNodes);
	}
}

void FFbxParser::CollectBones(FbxSkin* Skin, TArray<FbxNode*>& OutBoneNodes)
{
	// FbxSkin의 Cluster에서 Bone 노드 수집
	int32 ClusterCount = Skin->GetClusterCount();

	for (int32 i = 0; i < ClusterCount; i++)
	{
		FbxCluster* Cluster = Skin->GetCluster(i);
		FbxNode* BoneNode = Cluster->GetLink();
		if (BoneNode)
		{
			OutBoneNodes.Add(BoneNode);
		}
	}
}

void FFbxParser::BuildBoneHierarchy(const TArray<FbxNode*>& BoneNodes, FSkeleton& OutSkeleton)
{
	int32 NumBones = BoneNodes.Num();
	OutSkeleton.BoneNames.SetNum(NumBones);
	OutSkeleton.Parents.SetNum(NumBones);
	OutSkeleton.RefPoseLocal.SetNum(NumBones);

	for (int32 i = 0; i < NumBones; i++)
	{
		FbxNode* BoneNode = BoneNodes[i];

		// Bone 이름
		OutSkeleton.BoneNames[i] = FName(BoneNode->GetName());

		// 부모 인덱스 찾기
		OutSkeleton.Parents[i] = FindParentBoneIndex(BoneNode, BoneNodes);

		// RefPoseLocal (부모 상대 Transform)
		OutSkeleton.RefPoseLocal[i] = ConvertTransform(BoneNode);
	}
}

int32 FFbxParser::FindParentBoneIndex(FbxNode* BoneNode, const TArray<FbxNode*>& BoneNodes)
{
	FbxNode* ParentNode = BoneNode->GetParent();
	if (!ParentNode)
	{
		return -1;
	}

	// Parent가 Bone 목록에 있는지 확인
	for (int32 i = 0; i < BoneNodes.Num(); i++)
	{
		if (BoneNodes[i] == ParentNode)
		{
			return i;
		}
	}

	// Parent가 Bone이 아니면 Root
	return -1;
}

void FFbxParser::BuildMeshSections(FbxMesh* Mesh, FSkeletalMesh& OutMesh)
{
	// 전체 Mesh를 하나의 Section으로 처리
	FSkeletalMeshSection Section;

	int32 ControlPointCount = Mesh->GetControlPointsCount();
	int32 PolygonCount = Mesh->GetPolygonCount();

	FbxVector4* ControlPoints = Mesh->GetControlPoints();

	// Skin 정보 수집 (ControlPoint별)
	TArray<TArray<std::pair<uint16, float>>> ControlPointWeights;
	ControlPointWeights.SetNum(ControlPointCount);

	FbxSkin* Skin = GetSkin(Mesh);
	if (Skin)
	{
		// BoneMap 빌드
		TArray<int32> GlobalToSectionBoneMap;
		GlobalToSectionBoneMap.SetNum(OutMesh.Skeleton->GetNumBones());
		for (int32 i = 0; i < OutMesh.Skeleton->GetNumBones(); i++)
		{
			GlobalToSectionBoneMap[i] = -1;
		}

		int32 ClusterCount = Skin->GetClusterCount();
		for (int32 ClusterIdx = 0; ClusterIdx < ClusterCount; ClusterIdx++)
		{
			FbxCluster* Cluster = Skin->GetCluster(ClusterIdx);
			FbxNode* BoneNode = Cluster->GetLink();
			if (!BoneNode)
			{
				continue;
			}

			FName BoneName(BoneNode->GetName());
			int32 GlobalBoneIdx = OutMesh.Skeleton->FindBoneIndex(BoneName);
			if (GlobalBoneIdx == -1)
			{
				continue;
			}

			// BoneMap에 추가
			if (GlobalToSectionBoneMap[GlobalBoneIdx] == -1)
			{
				GlobalToSectionBoneMap[GlobalBoneIdx] = Section.BoneMap.Num();
				Section.BoneMap.Add(static_cast<uint16>(GlobalBoneIdx));
			}

			uint16 SectionBoneIdx = static_cast<uint16>(GlobalToSectionBoneMap[GlobalBoneIdx]);

			// Weight 정보 추출
			int32* Indices = Cluster->GetControlPointIndices();
			double* Weights = Cluster->GetControlPointWeights();
			int32 IndexCount = Cluster->GetControlPointIndicesCount();

			for (int32 i = 0; i < IndexCount; i++)
			{
				int32 CPIdx = Indices[i];
				float Weight = static_cast<float>(Weights[i]);

				if (CPIdx >= 0 && CPIdx < ControlPointCount && Weight > 1e-5f)
				{
					ControlPointWeights[CPIdx].Add({ SectionBoneIdx, Weight });
				}
			}
		}
	}

	// Normal Layer
	FbxGeometryElementNormal* NormalElement = Mesh->GetElementNormal();

	// UV Layer
	FbxGeometryElementUV* UVElement = Mesh->GetElementUV();

	// Vertex 및 Index 빌드
	int32 VertexCounter = 0;
	for (int32 PolyIndex = 0; PolyIndex < PolygonCount; PolyIndex++)
	{
		int32 PolySize = Mesh->GetPolygonSize(PolyIndex);
		if (PolySize != 3)
		{
			continue;
		}

		for (int32 VertIndex = 0; VertIndex < 3; VertIndex++)
		{
			int32 ControlPointIndex = Mesh->GetPolygonVertex(PolyIndex, VertIndex);

			FSkeletalVertex SkVertex;

			// Position
			FbxVector4 Pos = ControlPoints[ControlPointIndex];
			SkVertex.Vertex.Position = ConvertPosition(Pos);

			// Normal
			if (NormalElement)
			{
				FbxVector4 Normal;
				Mesh->GetPolygonVertexNormal(PolyIndex, VertIndex, Normal);
				SkVertex.Vertex.Normal = ConvertNormal(Normal);
			}

			// UV
			if (UVElement)
			{
				FbxVector2 UV;
				bool Unmapped = false;
				Mesh->GetPolygonVertexUV(PolyIndex, VertIndex, UVElement->GetName(), UV, Unmapped);
				SkVertex.Vertex.TexCoord = FVector2(static_cast<float>(UV[0]), static_cast<float>(1.0 - UV[1]));
			}

			// Skin Influence (최대 4개 Bone, 내림차순 정렬)
			auto& Weights = ControlPointWeights[ControlPointIndex];
			if (Weights.Num() > 0)
			{
				// Weight 기준 내림차순 정렬
				std::ranges::sort(Weights,
				                  [](const std::pair<uint16, float>& A, const std::pair<uint16, float>& B)
				                  {
					                  return A.second > B.second;
				                  });

				// 최대 4개만 선택
				int32 NumInfluences = min(4, Weights.Num());
				for (int32 i = 0; i < NumInfluences; i++)
				{
					SkVertex.Skin.BoneIndices[i] = Weights[i].first;
					SkVertex.Skin.BoneWeights[i] = Weights[i].second;
				}

				// 정규화
				SkVertex.Skin.Normalize();
			}

			Section.Vertices.Add(SkVertex);
			Section.Indices.Add(VertexCounter);
			VertexCounter++;
		}
	}

	OutMesh.Sections.Add(Section);

	UE_LOG("FbxParser: MeshSection 빌드 완료 (Vertices: %d, Indices: %d, BoneMap: %d)",
		Section.Vertices.Num(), Section.Indices.Num(), Section.BoneMap.Num());
}

void FFbxParser::ExtractMaterials(const FbxNode* Node, FSkeletalMesh& OutMesh)
{
	int32 MaterialCount = Node->GetMaterialCount();
	OutMesh.MaterialInfo.SetNum(MaterialCount);

	for (int32 i = 0; i < MaterialCount; i++)
	{
		FbxSurfaceMaterial* Material = Node->GetMaterial(i);
		if (!Material)
		{
			continue;
		}

		FMaterial& Mat = OutMesh.MaterialInfo[i];
		// Material 이름
		// Mat.Name = FName(Material->GetName());

		// Diffuse Texture 추출 (예시)
		// FbxProperty 사용하여 텍스처 경로 추출 가능
		// 여기서는 기본 구현만 제공
	}
}

bool FFbxParser::IsMeshSkinned(const FbxMesh* Mesh)
{
	return GetSkin(Mesh) != nullptr;
}

FbxSkin* FFbxParser::GetSkin(const FbxMesh* Mesh)
{
	if (!Mesh)
	{
		return nullptr;
	}

	int32 DeformerCount = Mesh->GetDeformerCount(FbxDeformer::eSkin);
	if (DeformerCount > 0)
	{
		return static_cast<FbxSkin*>(Mesh->GetDeformer(0, FbxDeformer::eSkin));
	}

	return nullptr;
}

// ========================================
// 좌표계 변환 함수
// ========================================

FVector FFbxParser::ConvertPosition(const FbxVector4& FbxVec)
{
	// FBX (좌표계 변환 후): Z-up Left-handed
	// 프로젝트: Z-up Left-handed
	// 이미 ConvertScene으로 변환했으므로 직접 매핑
	return {
		static_cast<float>(FbxVec[0]),
		static_cast<float>(FbxVec[1]),
		static_cast<float>(FbxVec[2])
	};
}

FVector FFbxParser::ConvertNormal(const FbxVector4& FbxVec)
{
	return ConvertPosition(FbxVec); // Normal도 동일하게 변환
}

FQuaternion FFbxParser::ConvertRotation(const FbxQuaternion& FbxQuat)
{
	// Quaternion 변환
	return {
		static_cast<float>(FbxQuat[0]), // X
		static_cast<float>(FbxQuat[1]), // Y
		static_cast<float>(FbxQuat[2]), // Z
		static_cast<float>(FbxQuat[3])  // W
	};
}

FTransform FFbxParser::ConvertTransform(const FbxNode* Node)
{
	FTransform Transform;

	// Local Transform (부모 상대)
	FbxDouble3 Translation = Node->LclTranslation.Get();
	FbxDouble3 Rotation = Node->LclRotation.Get();
	FbxDouble3 Scaling = Node->LclScaling.Get();

	Transform.Location = FVector(
		static_cast<float>(Translation[0]),
		static_cast<float>(Translation[1]),
		static_cast<float>(Translation[2])
	);

	// Rotation: FBX Euler (Degree) → Project Euler (Degree)
	// FTransform.Rotation은 FVector(Euler Degree)
	Transform.Rotation = FVector(
		static_cast<float>(Rotation[0]),
		static_cast<float>(Rotation[1]),
		static_cast<float>(Rotation[2])
	);

	Transform.Scale = FVector(
		static_cast<float>(Scaling[0]),
		static_cast<float>(Scaling[1]),
		static_cast<float>(Scaling[2])
	);

	return Transform;
}

FMatrix FFbxParser::FbxMatrixToFMatrix(const FbxAMatrix& FbxMat)
{
	FMatrix Mat;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			Mat.Data[i][j] = static_cast<float>(FbxMat.Get(i, j));
		}
	}
	return Mat;
}
