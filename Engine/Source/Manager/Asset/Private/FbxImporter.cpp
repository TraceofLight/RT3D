#include "pch.h"
#include "Manager/Asset/Public/FbxImporter.h"

#include "Component/Mesh/Public/StaticMesh.h"
#include "Texture/Public/Material.h"
#include "Manager/Path/Public/PathManager.h"
#include <vector>

// Current FBX file directory for resolving relative texture paths
static std::filesystem::path GCurrentFbxDir;

FbxAMatrix FFbxImporter::JointPostConversionMatrix;
FbxAMatrix FFbxImporter::AxisConversionMatrix;
FbxAMatrix FFbxImporter::AxisConversionMatrixInv;
 
// ========================================
// FFbxParser 구현
// ========================================

FFbxImporter::FFbxImporter()
{
	Initialize();
}

FFbxImporter::~FFbxImporter()
{
	Cleanup();
}

void FFbxImporter::Initialize()
{
	Manager = FbxManager::Create();
	if (!Manager)
	{
		UE_LOG_ERROR("FbxParser: FbxManager 생성 실패");
		return;
	}

	FbxIOSettings* IOSettings = FbxIOSettings::Create(Manager, IOSROOT);
	Manager->SetIOSettings(IOSettings);

	// FBX 플러그인 로드 진단
	FbxString PluginPath = FbxGetApplicationDirectory();
	int32 PluginCount = Manager->LoadPluginsDirectory(PluginPath.Buffer());

	UE_LOG("FbxImporter: 초기화 완료 (SDK Version: %s)", Manager->GetVersion());
}

void FFbxImporter::Cleanup()
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
bool FFbxImporter::LoadSkeletalMesh(const FString& FilePath, FSkeletalMesh& OutMesh)
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
	  
	FbxAMatrix AxisConversionMatrix;
	AxisConversionMatrix.SetIdentity();

	FbxAMatrix JointOrientationMatrix;
	JointOrientationMatrix.SetIdentity();

	//FbxAxisSystem::ECoordSystem CoordSystem = FbxAxisSystem::eRightHanded;
	// Left Hand + UP:eZAxis  + Front: Even
	// Up: Z, Front: -Y, Right: X

	//Right Hand+ UP:eZAxis  + Front: Even
	// UP: Z Front: Y, Right: -X

	// RightHand  + up: XAxis + Even
	//blender 거쳤을 때 (z up, x front)
	//FbxAxisSystem::ECoordSystem CoordSystem = FbxAxisSystem::eLeftHanded;
	//FbxAxisSystem::EUpVector UpVector = FbxAxisSystem::eXAxis;
	//FbxAxisSystem::EFrontVector FrontVector = FbxAxisSystem::eParityEven; 
	//FbxAxisSystem UnrealImportAxis(UpVector, FrontVector, CoordSystem);
	//UnrealImportAxis.DeepConvertScene(Scene);

	// MIXAMO
	//FbxAxisSystem::ECoordSystem CoordSystem = FbxAxisSystem::eRightHanded;
	//FbxAxisSystem::EUpVector UpVector = FbxAxisSystem::eZAxis;
	//FbxAxisSystem::EFrontVector FrontVector = FbxAxisSystem::eParityEven; 
	//FbxAxisSystem UnrealImportAxis(UpVector, FrontVector, CoordSystem);
	//UnrealImportAxis.DeepConvertScene(Scene);

	const FbxAxisSystem EngineTargetAxis(
		FbxAxisSystem::eZAxis,
		FbxAxisSystem::eParityEven,
		FbxAxisSystem::eLeftHanded
	);
	 
	FbxAxisSystem SourceAxis = Scene->GetGlobalSettings().GetAxisSystem();

	// 3. 소스 좌표계와 우리 엔진의 목표 좌표계가 다른지 비교합니다.
	if (SourceAxis != EngineTargetAxis)
	{ 
		EngineTargetAxis.DeepConvertScene(Scene);
	}
	 
	 // 단위 변환 (센티미터로 통일)
	FbxSystemUnit SceneSystemUnit = Scene->GetGlobalSettings().GetSystemUnit();
	if (SceneSystemUnit != FbxSystemUnit::cm)
	{
		FbxSystemUnit::cm.ConvertScene(Scene);
	}

	//ApplySceneRotation();
	
	// 삼각형화 (모든 폴리곤을 삼각형으로)
	FbxGeometryConverter Converter(Manager); 
	Converter.Triangulate(Scene, true);

	// Scene 처리
	OutMesh.PathFileNameString = FilePath;
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

bool FFbxImporter::LoadStaticMesh(const FString& FilePath, FStaticMesh& OutMesh)
{
	if (!Manager)
	{
		UE_LOG_ERROR("FbxParser: Manager가 초기화되지 않음");
		return false;
	}

	if (!ImportScene(FilePath))
	{
		return false;
	}

	// Scene 처리
	OutMesh.PathFileName = FName(FilePath);
	ProcessSceneAsStatic(OutMesh);

	if (OutMesh.Vertices.IsEmpty() || OutMesh.Indices.IsEmpty())
	{
		UE_LOG_ERROR("FbxParser: 유효한 Static Mesh를 찾지 못함 (%s)", FilePath.c_str());
		return false;
	}

	UE_LOG("FbxParser: Static Mesh 로드 성공 (%s)", FilePath.c_str());
	return true;
}

bool FFbxImporter::IsSkeletalMesh(const FString& FilePath)
{
	FFbxImporter& Parser = FFbxImporter::GetInstance();

	if (!Parser.ImportScene(FilePath))
	{
		return false;
	}

	// Scene에서 재귀적으로 Mesh 노드를 찾아 Skinned 여부 확인
	FbxNode* RootNode = Parser.Scene->GetRootNode();
	if (!RootNode)
	{
		Parser.Cleanup();
		return false;
	}

	// 재귀 탐색용 람다 함수
	std::function<bool(FbxNode*)> CheckNodeRecursive = [&](FbxNode* Node) -> bool
	{
		if (!Node)
		{
			return false;
		}

		// 현재 노드가 Mesh인지 확인
		FbxNodeAttribute* Attribute = Node->GetNodeAttribute();
		if (Attribute && Attribute->GetAttributeType() == FbxNodeAttribute::eMesh)
		{
			FbxMesh* Mesh = Node->GetMesh();
			if (Mesh && IsMeshSkinned(Mesh))
			{
				UE_LOG("FbxParser: Found skinned mesh: %s", Node->GetName());
				return true;
			}
		}

		// 자식 노드 재귀 탐색
		for (int32 i = 0; i < Node->GetChildCount(); i++)
		{
			if (CheckNodeRecursive(Node->GetChild(i)))
			{
				return true;
			}
		}

		return false;
	};

	bool bIsSkeletal = CheckNodeRecursive(RootNode);
	return bIsSkeletal;
}

bool FFbxImporter::ImportScene(const FString& FilePath)
{
	Scene = FbxScene::Create(Manager, "ImportScene");
	if (!Scene)
	{
		UE_LOG_ERROR("FbxParser: Scene 생성 실패");
		return false;
	}

	// 상대 경로를 절대 경로로 변환
	path AbsolutePath = std::filesystem::absolute(FilePath);
	FString AbsolutePathString = AbsolutePath.string();

	// Remember FBX directory for resolving relative texture paths
	GCurrentFbxDir = AbsolutePath.parent_path();

	// 파일 존재 확인
	if (!exists(AbsolutePath))
	{
		UE_LOG_ERROR("FbxParser: 파일이 존재하지 않음: %s", AbsolutePathString.c_str());
		return false;
	}

	UE_LOG("FbxParser: Loading FBX file: %s", AbsolutePathString.c_str());

	FbxImporter* Importer = FbxImporter::Create(Manager, "");

	// FBX 파일 포맷을 명시적으로 지정 (ASCII 또는 Binary 자동 감지)
	int32 FileFormat = -1;
	if (!Manager->GetIOPluginRegistry()->DetectReaderFileFormat(AbsolutePathString.c_str(), FileFormat))
	{
		// 감지 실패 시 FBX 기본 포맷 사용
		FileFormat = Manager->GetIOPluginRegistry()->GetNativeReaderFormat();
		UE_LOG_WARNING("FbxParser: 파일 포맷 자동 감지 실패, 기본 포맷 사용");
	}

	if (!Importer->Initialize(AbsolutePathString.c_str(), FileFormat, Manager->GetIOSettings()))
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

void FFbxImporter::ProcessScene(FSkeletalMesh& OutMesh) const
{
	FbxNode* RootNode = Scene->GetRootNode();
	if (!RootNode)
	{
		UE_LOG_ERROR("FbxParser: RootNode is null");
		return;
	}

	UE_LOG("FbxParser: Processing scene, RootNode: %s, Children: %d", RootNode->GetName(), RootNode->GetChildCount());

	ProcessNode(RootNode, OutMesh);

	if (!OutMesh.IsValid())
	{
		UE_LOG_ERROR("FbxParser: No valid skeletal mesh found in scene");
	}
}

void FFbxImporter::ProcessSceneAsStatic(FStaticMesh& OutMesh) const
{
	if (!Scene)
	{
		UE_LOG_ERROR("FbxParser: Scene이 없음");
		return;
	}

	FbxNode* RootNode = Scene->GetRootNode();
	if (!RootNode)
	{
		UE_LOG_ERROR("FbxParser: RootNode가 없음");
		return;
	}

	UE_LOG("FbxParser: Processing scene as static mesh, RootNode: %s, Children: %d", RootNode->GetName(), RootNode->GetChildCount());

	ProcessNodeAsStatic(RootNode, OutMesh);

	if (OutMesh.Vertices.IsEmpty() || OutMesh.Indices.IsEmpty())
	{
		UE_LOG_ERROR("FbxParser: No valid static mesh found in scene");
	}
}

void FFbxImporter::ProcessNode(FbxNode* Node, FSkeletalMesh& OutMesh)
{
	if (!Node)
	{
		return;
	}

	UE_LOG("FbxParser: Processing node: %s", Node->GetName());

	// Mesh 노드 확인
	FbxNodeAttribute* Attribute = Node->GetNodeAttribute();
	if (Attribute)
	{
		FbxNodeAttribute::EType AttributeType = Attribute->GetAttributeType();
		UE_LOG("FbxParser: Node '%s' has attribute type: %d (eMesh=%d)", Node->GetName(), (int)AttributeType, (int)FbxNodeAttribute::eMesh);

		if (AttributeType == FbxNodeAttribute::eMesh)
		{
			FbxMesh* Mesh = Node->GetMesh();
			if (Mesh)
			{
				bool bIsSkinned = IsMeshSkinned(Mesh);
				UE_LOG("FbxParser: Mesh '%s' skinned: %s, Deformers: %d", Node->GetName(), bIsSkinned ? "Yes" : "No", Mesh->GetDeformerCount());

				if (bIsSkinned)
				{
					ProcessSkeletalMesh(Node, OutMesh);
					return; // 첫 번째 Skeletal Mesh만 로드
				}
				else if (!OutMesh.IsValid())
				{
					// Skinned Mesh가 아니면 Static Mesh를 Skeletal Mesh처럼 로드 (Skeleton 없이)
					UE_LOG_WARNING("FbxParser: Mesh '%s'는 Skinned Mesh가 아니지만 로드 시도", Node->GetName());
					ProcessStaticMeshAsSkeletal(Node, OutMesh);
					return;
				}
			}
		}
	}

	// 자식 노드 순회
	for (int i = 0; i < Node->GetChildCount(); i++)
	{
		ProcessNode(Node->GetChild(i), OutMesh);
	}
}

void FFbxImporter::ProcessNodeAsStatic(FbxNode* Node, FStaticMesh& OutMesh)
{
	if (!Node)
	{
		return;
	}

	UE_LOG("FbxParser: Processing static node: %s", Node->GetName());

	// Mesh 노드 확인
	FbxNodeAttribute* Attribute = Node->GetNodeAttribute();
	if (Attribute)
	{
		FbxNodeAttribute::EType AttributeType = Attribute->GetAttributeType();
		if (AttributeType == FbxNodeAttribute::eMesh)
		{
			FbxMesh* Mesh = Node->GetMesh();
			if (Mesh && OutMesh.Vertices.IsEmpty())
			{
				UE_LOG("FbxParser: Static Mesh 처리 시작 (%s)", Node->GetName());

				// Vertex/Index 데이터 추출
				int32 VertexCount = Mesh->GetControlPointsCount();
				FbxVector4* ControlPoints = Mesh->GetControlPoints();

				int32 PolygonCount = Mesh->GetPolygonCount();
				for (int32 PolyIndex = 0; PolyIndex < PolygonCount; PolyIndex++)
				{
					int32 PolygonSize = Mesh->GetPolygonSize(PolyIndex);
					if (PolygonSize != 3)
					{
						continue; // 이미 삼각형화되어 있어야 함
					}

					for (int32 VertIndex = 0; VertIndex < 3; VertIndex++)
					{
						int32 ControlPointIndex = Mesh->GetPolygonVertex(PolyIndex, VertIndex);

						FNormalVertex Vertex;
						Vertex.Position = ConvertPosition(ControlPoints[ControlPointIndex]);

						// Normal 추출
						FbxVector4 FbxNormal;
						Mesh->GetPolygonVertexNormal(PolyIndex, VertIndex, FbxNormal);
						Vertex.Normal = ConvertNormal(FbxNormal);

						// UV 추출
						FbxVector2 FbxUV(0.0, 0.0);
						bool bUnmapped = false;
						if (Mesh->GetElementUVCount() > 0)
						{
							Mesh->GetPolygonVertexUV(PolyIndex, VertIndex, Mesh->GetElementUV(0)->GetName(), FbxUV, bUnmapped);
						}
						Vertex.TexCoord = FVector2(static_cast<float>(FbxUV[0]), 1.0f - static_cast<float>(FbxUV[1]));

						OutMesh.Vertices.Add(Vertex);
						OutMesh.Indices.Add(static_cast<uint32>(OutMesh.Indices.Num()));
					}
				}

				// Material 추출
				ExtractMaterialsForStatic(Node, OutMesh);

				UE_LOG("FbxParser: Static Mesh 처리 완료 (Vertices: %d, Indices: %d, Materials: %d)",
					OutMesh.Vertices.Num(), OutMesh.Indices.Num(), OutMesh.MaterialInfo.Num());
				return; // 첫 번째 Mesh만 로드
			}
		}
	}

	// 자식 노드 순회
	for (int i = 0; i < Node->GetChildCount(); i++)
	{
		ProcessNodeAsStatic(Node->GetChild(i), OutMesh);
	}
}

void FFbxImporter::NormalizeScene(FbxScene* Scene, FbxManager* Manager, const FFbxImportOptions& Opt)
{
	if (!Scene || !Manager) return;

	if (Opt.bConvertScene)
	{
		//FbxAxisSystem Target(FbxAxisSystem::eZAxis, o)
	}
	if (Opt.bConvertSceneUnit)
	{

	}
	if (std::abs(Opt.ImportUniformScale - 1.0f) > 1e-6f)
	{

	}

	// 모든 폴리곤을 삼각형으로 통일
	FbxGeometryConverter Converter(Manager);
	Converter.Triangulate(Scene, true);
		

}

bool FFbxImporter::ProcessSkeletalMesh(FbxNode* MeshNode, FSkeletalMesh& OutMesh)
{
	FbxMesh* Mesh = MeshNode->GetMesh();
	if (!Mesh)
	{
		return false;
	}

	UE_LOG("FbxParser: Skeletal Mesh 처리 시작 (%s)", MeshNode->GetName());

	// Skeleton 생성 및 빌드
	OutMesh.Skeleton = new FSkeleton();
	BuildSkeleton(Mesh, *OutMesh.Skeleton); //=> 여기서 

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

bool FFbxImporter::ProcessStaticMeshAsSkeletal(FbxNode* MeshNode, FSkeletalMesh& OutMesh)
{
	FbxMesh* Mesh = MeshNode->GetMesh();
	if (!Mesh)
	{
		return false;
	}

	UE_LOG("FbxParser: Static Mesh를 Skeletal Mesh로 변환 (%s)", MeshNode->GetName());

	// 단일 Root Bone을 가진 더미 Skeleton 생성
	OutMesh.Skeleton = new FSkeleton();
	OutMesh.Skeleton->BoneNamesString.Add("Root");
	OutMesh.Skeleton->BoneNames.Add(FName("Root"));
	OutMesh.Skeleton->Parents.Add(-1);

	// FbxNode의 변환을 Root Bone의 RefPoseLocal로 사용
	FTransform RootTransform = ConvertTransform(MeshNode);
	OutMesh.Skeleton->RefPoseLocal.Add(RootTransform);

	// Mesh Section 빌드 (스킨 데이터 없이)
	BuildMeshSections(Mesh, OutMesh);

	// Material 추출
	ExtractMaterials(MeshNode, OutMesh);

	// RefPoseGlobal 계산
	OutMesh.Skeleton->BuildRefPoseGlobal();

	UE_LOG("FbxParser: Static Mesh 변환 완료 (Sections: %d)", OutMesh.Sections.Num());

	return OutMesh.IsValid();
}

void FFbxImporter::BuildSkeleton(const FbxMesh* Mesh, FSkeleton& OutSkeleton)
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

void FFbxImporter::CollectBones(FbxNode* Node, TArray<FbxNode*>& OutBoneNodes)
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

void FFbxImporter::CollectBones(FbxSkin* Skin, TArray<FbxNode*>& OutBoneNodes)
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

void FFbxImporter::BuildBoneHierarchy(const TArray<FbxNode*>& BoneNodes, FSkeleton& OutSkeleton)
{
	int32 NumBones = BoneNodes.Num();
	OutSkeleton.BoneNamesString.SetNum(NumBones);
	OutSkeleton.BoneNames.SetNum(NumBones);
	OutSkeleton.Parents.SetNum(NumBones);
	OutSkeleton.Childs.SetNum(NumBones);
	OutSkeleton.RefPoseLocal.SetNum(NumBones);

	UE_LOG("FbxImporter: BuildBoneHierarchy 시작 (BoneCount: %d)", NumBones);

	for (int32 i = 0; i < NumBones; i++)
	{
		FbxNode* BoneNode = BoneNodes[i];

		// Bone 이름
		OutSkeleton.BoneNamesString[i] = BoneNode->GetName();

		// 부모 인덱스 찾기
		OutSkeleton.Parents[i] = FindParentBoneIndex(BoneNode, BoneNodes);

		//부모의 자식인덱스에 현재 i 추가
		if (OutSkeleton.Parents[i] >= 0)
		{
			OutSkeleton.Childs[OutSkeleton.Parents[i]].Add(i);
		}
		 
		// *** RefPoseLocal에 변환 적용 ***
		FTransform BoneTransform = ConvertTransform(BoneNode);

		// *** Root Bone에만 JointPostConversionMatrix 추가 적용 ***
		if (OutSkeleton.Parents[i] == -1) // Root Bone
		{
			// Root Bone에 추가 회전 적용 (예: -90도 Y축 회전)
			FbxAMatrix LocalMatrix = BoneNode->EvaluateLocalTransform();
			FbxAMatrix PostConverted = JointPostConversionMatrix *
				AxisConversionMatrix *
				LocalMatrix *
				AxisConversionMatrixInv;

			// 변환된 행렬에서 Transform 추출
			FbxVector4 Translation = PostConverted.GetT();
			BoneTransform.Location = FVector(
				static_cast<float>(Translation[0]),
				static_cast<float>(Translation[1]),
				static_cast<float>(Translation[2])
			);

			FbxQuaternion FbxQuat = PostConverted.GetQ();
			BoneTransform.Rotation = FQuat(
				static_cast<float>(FbxQuat[0]),
				static_cast<float>(FbxQuat[1]),
				static_cast<float>(FbxQuat[2]),
				static_cast<float>(FbxQuat[3])
			);

			FbxVector4 Scaling = PostConverted.GetS();
			BoneTransform.Scale = FVector(
				static_cast<float>(Scaling[0]),
				static_cast<float>(Scaling[1]),
				static_cast<float>(Scaling[2])
			);
		}

		// RefPoseLocal (부모 상대 Transform)
		OutSkeleton.RefPoseLocal[i] = BoneTransform;
	}
	OutSkeleton.SetName();
}

int32 FFbxImporter::FindParentBoneIndex(FbxNode* BoneNode, const TArray<FbxNode*>& BoneNodes)
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

void FFbxImporter::BuildMeshSections(FbxMesh* Mesh, FSkeletalMesh& OutMesh)
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
	 
	FbxGeometryElementTangent* TangentElement = Mesh->GetElementTangent(0);
	if (TangentElement == nullptr && UVElement != nullptr)
	{
		if (Mesh->GenerateTangentsData(0, true))
		{
			// 생성이 성공하면 TangentElement를 다시 가져옵니다.
			TangentElement = Mesh->GetElementTangent(0);
		}
	}
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

			// Color (기본값 흰색)
			SkVertex.Vertex.Color = FVector4(1.0f, 1.0f, 1.0f, 1.0f);

			// UV
			if (UVElement)
			{
				FbxVector2 UV;
				bool Unmapped = false;
				Mesh->GetPolygonVertexUV(PolyIndex, VertIndex, UVElement->GetName(), UV, Unmapped);
				SkVertex.Vertex.TexCoord = FVector2(static_cast<float>(UV[0]), static_cast<float>(1.0 - UV[1]));
			} 
			if (TangentElement)
			{
				FbxVector4 Tangent;
				int TangentIndex = -1;

				// FBX는 Tangent에 대한 헬퍼 함수가 없으므로 인덱싱 방식 확인
				if (TangentElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
				{
					if (TangentElement->GetReferenceMode() == FbxGeometryElement::eDirect)
					{
						TangentIndex = VertexCounter;
					}
					else // eIndexToDirect
					{
						TangentIndex = TangentElement->GetIndexArray().GetAt(VertexCounter);
					}
				}
				else if (TangentElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
				{
					int32 ControlPointIndex = Mesh->GetPolygonVertex(PolyIndex, VertIndex);
					if (TangentElement->GetReferenceMode() == FbxGeometryElement::eDirect)
					{
						TangentIndex = ControlPointIndex;
					}
					else // eIndexToDirect
					{
						TangentIndex = TangentElement->GetIndexArray().GetAt(ControlPointIndex);
					}
				}

				if (TangentIndex != -1)
				{
					Tangent = TangentElement->GetDirectArray().GetAt(TangentIndex);

					// ConvertNormal 함수가 좌표계 변환 및 정규화를 수행
					FVector Tangent3 = ConvertNormal(Tangent);

					// FBX의 Tangent.W (Tangent[3])는 Handedness 값을 가짐
					float Handedness = (float)Tangent[3];
					// Handedness가 0이면 비정상이므로 1.0f로 보정 (셰이더에서 사용)
					if (Handedness == 0.0f) Handedness = 1.0f;

					SkVertex.Vertex.Tangent = FVector4(Tangent3, Handedness);
				}
				else
				{
					// 인덱싱 실패 시 (있어서는 안 됨)
					SkVertex.Vertex.Tangent = FVector4(1.0f, 0.0f, 0.0f, 1.0f); 
				}
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

	if (Section.Vertices.Num() > 0)
	{
		const FSkeletalVertex& FirstVert = Section.Vertices[0];
		UE_LOG("FbxParser: MeshSection 빌드 완료 (Vertices: %d, Indices: %d, BoneMap: %d, FirstPos: %.3f,%.3f,%.3f, BoneIdx: %d,%d,%d,%d, Weight: %.3f,%.3f,%.3f,%.3f)",
			Section.Vertices.Num(), Section.Indices.Num(), Section.BoneMap.Num(),
			FirstVert.Vertex.Position.X, FirstVert.Vertex.Position.Y, FirstVert.Vertex.Position.Z,
			FirstVert.Skin.BoneIndices[0], FirstVert.Skin.BoneIndices[1], FirstVert.Skin.BoneIndices[2], FirstVert.Skin.BoneIndices[3],
			FirstVert.Skin.BoneWeights[0], FirstVert.Skin.BoneWeights[1], FirstVert.Skin.BoneWeights[2], FirstVert.Skin.BoneWeights[3]);
	}
	else
	{
		UE_LOG("FbxParser: MeshSection 빌드 완료 (Vertices: %d, Indices: %d, BoneMap: %d)",
			Section.Vertices.Num(), Section.Indices.Num(), Section.BoneMap.Num());
	}
}

void FFbxImporter::ExtractMaterials(const FbxNode* Node, FSkeletalMesh& OutMesh)
{
	int32 MaterialCount = Node->GetMaterialCount();
	if (MaterialCount == 0)
	{
		// Material이 없으면 기본 Material 1개 생성
		OutMesh.MaterialInfo.SetNum(1);
		FMaterial& DefaultMat = OutMesh.MaterialInfo[0];
		DefaultMat.Name = "DefaultMaterial";
		DefaultMat.Diffuse = FVector(0.8f, 0.8f, 0.8f);
		DefaultMat.Opacity = 1.0f;
		return;
	}


	OutMesh.MaterialInfo.SetNum(MaterialCount);

	for (int32 i = 0; i < MaterialCount; i++)
	{
		FbxSurfaceMaterial* FbxMaterial = Node->GetMaterial(i);
		if (!FbxMaterial)
		{
			continue;
		}

		FMaterial& Mat = OutMesh.MaterialInfo[i];
		Mat.Name = FbxMaterial->GetName();

		// Phong/Lambert Material 처리
		if (FbxMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
		{
			FbxSurfacePhong* Phong = static_cast<FbxSurfacePhong*>(FbxMaterial);

			// Diffuse Color
			FbxDouble3 Diffuse = Phong->Diffuse.Get();
			Mat.Diffuse = FVector(static_cast<float>(Diffuse[0]), static_cast<float>(Diffuse[1]), static_cast<float>(Diffuse[2]));

			// Ambient Color
			FbxDouble3 Ambient = Phong->Ambient.Get();
			Mat.Ambient = FVector(static_cast<float>(Ambient[0]), static_cast<float>(Ambient[1]), static_cast<float>(Ambient[2]));

			// Specular Color
			FbxDouble3 Specular = Phong->Specular.Get();
			Mat.Specular = FVector(static_cast<float>(Specular[0]), static_cast<float>(Specular[1]), static_cast<float>(Specular[2]));

			// Shininess
			Mat.Shininess = static_cast<float>(Phong->Shininess.Get());
		}
		else if (FbxMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId))
		{
			FbxSurfaceLambert* Lambert = static_cast<FbxSurfaceLambert*>(FbxMaterial);

			// Diffuse Color
			FbxDouble3 Diffuse = Lambert->Diffuse.Get();
			Mat.Diffuse = FVector(static_cast<float>(Diffuse[0]), static_cast<float>(Diffuse[1]), static_cast<float>(Diffuse[2]));

			// Ambient Color
			FbxDouble3 Ambient = Lambert->Ambient.Get();
			Mat.Ambient = FVector(static_cast<float>(Ambient[0]), static_cast<float>(Ambient[1]), static_cast<float>(Ambient[2]));
		}

		// Texture 추출 (임베디드 + 외부 파일)
		if (FbxMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
		{
			FbxSurfacePhong* Phong = static_cast<FbxSurfacePhong*>(FbxMaterial);
			Mat.DiffuseTexturePath = GetTextureFilePath(Phong->Diffuse, Mat.Name, "BaseColor");
			Mat.NormalTexturePath = GetTextureFilePath(Phong->NormalMap, Mat.Name, "Normal");
			Mat.SpecularTexturePath = GetTextureFilePath(Phong->Specular, Mat.Name, "Specular");
		}
		else if (FbxMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId))
		{
			FbxSurfaceLambert* Lambert = static_cast<FbxSurfaceLambert*>(FbxMaterial);
			Mat.DiffuseTexturePath = GetTextureFilePath(Lambert->Diffuse, Mat.Name, "BaseColor");
			Mat.NormalTexturePath = GetTextureFilePath(Lambert->NormalMap, Mat.Name, "Normal");
		}

		UE_LOG("FbxParser: Skeletal Material[%d] = %s (Diffuse: %.2f, %.2f, %.2f, DiffuseTex: %s)",
			i, Mat.Name.c_str(), Mat.Diffuse.X, Mat.Diffuse.Y, Mat.Diffuse.Z,
			Mat.DiffuseTexturePath.empty() ? "None" : Mat.DiffuseTexturePath.c_str());
	}
	/*
	FString("C:\\Users\\Jungle\\Desktop\\GameEngine\\Week10\\Build\\Debug\\Data\\FBX\\castle_guard.fbm\\Guard_02__diffuse.png");
	*/
}

void FFbxImporter::ExtractMaterialsForStatic(const FbxNode* Node, FStaticMesh& OutMesh)
{
	int32 MaterialCount = Node->GetMaterialCount();
	if (MaterialCount == 0)
	{
		// Material이 없으면 기본 Material 1개 생성
		OutMesh.MaterialInfo.SetNum(1);
		FMaterial& DefaultMat = OutMesh.MaterialInfo[0];
		DefaultMat.Name = "DefaultMaterial";
		DefaultMat.Diffuse = FVector(0.8f, 0.8f, 0.8f);
		return;
	}

	OutMesh.MaterialInfo.SetNum(MaterialCount);

	for (int32 i = 0; i < MaterialCount; i++)
	{
		FbxSurfaceMaterial* FbxMaterial = Node->GetMaterial(i);
		if (!FbxMaterial)
		{
			continue;
		}

		FMaterial& Mat = OutMesh.MaterialInfo[i];
		Mat.Name = FbxMaterial->GetName();

		// Phong/Lambert Material 처리
		if (FbxMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
		{
			FbxSurfacePhong* Phong = static_cast<FbxSurfacePhong*>(FbxMaterial);

			// Diffuse Color
			FbxDouble3 Diffuse = Phong->Diffuse.Get();
			Mat.Diffuse = FVector(static_cast<float>(Diffuse[0]), static_cast<float>(Diffuse[1]), static_cast<float>(Diffuse[2]));

			// Ambient Color
			FbxDouble3 Ambient = Phong->Ambient.Get();
			Mat.Ambient = FVector(static_cast<float>(Ambient[0]), static_cast<float>(Ambient[1]), static_cast<float>(Ambient[2]));

			// Specular Color
			FbxDouble3 Specular = Phong->Specular.Get();
			Mat.Specular = FVector(static_cast<float>(Specular[0]), static_cast<float>(Specular[1]), static_cast<float>(Specular[2]));

			// Shininess
			Mat.Shininess = static_cast<float>(Phong->Shininess.Get());
		}
		else if (FbxMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId))
		{
			FbxSurfaceLambert* Lambert = static_cast<FbxSurfaceLambert*>(FbxMaterial);

			// Diffuse Color
			FbxDouble3 Diffuse = Lambert->Diffuse.Get();
			Mat.Diffuse = FVector(static_cast<float>(Diffuse[0]), static_cast<float>(Diffuse[1]), static_cast<float>(Diffuse[2]));

			// Ambient Color
			FbxDouble3 Ambient = Lambert->Ambient.Get();
			Mat.Ambient = FVector(static_cast<float>(Ambient[0]), static_cast<float>(Ambient[1]), static_cast<float>(Ambient[2]));
		}

		// Texture 추출 (임베디드 + 외부 파일)
		if (FbxMaterial->GetClassId().Is(FbxSurfacePhong::ClassId))
		{
			FbxSurfacePhong* Phong = static_cast<FbxSurfacePhong*>(FbxMaterial);
			Mat.DiffuseTexturePath = GetTextureFilePath(Phong->Diffuse, Mat.Name, "BaseColor");
			Mat.NormalTexturePath = GetTextureFilePath(Phong->NormalMap, Mat.Name, "Normal");
			Mat.SpecularTexturePath = GetTextureFilePath(Phong->Specular, Mat.Name, "Specular");
		}
		else if (FbxMaterial->GetClassId().Is(FbxSurfaceLambert::ClassId))
		{
			FbxSurfaceLambert* Lambert = static_cast<FbxSurfaceLambert*>(FbxMaterial);
			Mat.DiffuseTexturePath = GetTextureFilePath(Lambert->Diffuse, Mat.Name, "BaseColor");
			Mat.NormalTexturePath = GetTextureFilePath(Lambert->NormalMap, Mat.Name, "Normal");
		}

		UE_LOG("FbxParser: Material[%d] = %s (Diffuse: %.2f, %.2f, %.2f, DiffuseTex: %s)",
			i, Mat.Name.c_str(), Mat.Diffuse.X, Mat.Diffuse.Y, Mat.Diffuse.Z,
			Mat.DiffuseTexturePath.empty() ? "None" : Mat.DiffuseTexturePath.c_str());
	}
}

FString FFbxImporter::GetTextureFilePath(FbxProperty& Property, const FString& MaterialName, const FString& TextureType)
{
	if (!Property.IsValid())
	{
		return "";
	}

	// Property에 연결된 텍스처 개수 확인
	int32 TextureCount = Property.GetSrcObjectCount<FbxFileTexture>();
	if (TextureCount > 0)
	{
		FbxFileTexture* FileTexture = Property.GetSrcObject<FbxFileTexture>(0);
		if (FileTexture)
		{
			const char* TextureFileName = FileTexture->GetFileName();
			if (TextureFileName && strlen(TextureFileName) > 0)
			{
				path TexturePath(TextureFileName);

				// Try to resolve to an actual file first
				const path& RootPath = UPathManager::GetInstance().GetRootPath();
				const path& DataPath = UPathManager::GetInstance().GetDataPath();

				// a) Absolute path and exists -> use it
				if (TexturePath.is_absolute() && exists(TexturePath))
				{
					return canonical(TexturePath).string();
				}

				// b) Relative to FBX folder
				path Candidate = GCurrentFbxDir / TexturePath;
				if (exists(Candidate))
				{
					return canonical(Candidate).string();
				}

				// c) Relative to engine root
				Candidate = RootPath / TexturePath;
				if (exists(Candidate))
				{
					path RelativePath = relative(canonical(Candidate), RootPath);
					return RelativePath.string();
				}

				// d) Search by exact filename within Data
				{
					FString TargetName = TexturePath.filename().string();
					for (const auto& Entry : std::filesystem::recursive_directory_iterator(DataPath))
					{
						if (!Entry.is_regular_file()) continue;
						if (Entry.path().filename().string() == TargetName)
						{
							path RelativePath = relative(Entry.path(), RootPath);
							return RelativePath.string();
						}
					}
				}

				// 절대 경로인 경우 파일명만 추출
				if (TexturePath.is_absolute())
				{
					FString FileName = TexturePath.filename().string();
					// Data 폴더에서 파일명으로 검색
					FString FoundPath = FindTextureInDataFolder(MaterialName, "");
					if (!FoundPath.empty())
					{
						return FoundPath;
					}
				}

				// 상대 경로 또는 파일명만 있는 경우
				return TextureFileName;
			}
		}
	}

	// Layered Texture 확인
	TextureCount = Property.GetSrcObjectCount<FbxLayeredTexture>();
	if (TextureCount > 0)
	{
		FbxLayeredTexture* LayeredTexture = Property.GetSrcObject<FbxLayeredTexture>(0);
		int32 LayerCount = LayeredTexture->GetSrcObjectCount<FbxFileTexture>();
		if (LayerCount > 0)
		{
			FbxFileTexture* FileTexture = LayeredTexture->GetSrcObject<FbxFileTexture>(0);
			if (FileTexture)
			{
				const char* TextureFileName = FileTexture->GetFileName();
				if (TextureFileName && strlen(TextureFileName) > 0)
				{
					path TexturePath(TextureFileName);
					// Try to resolve as above for layered texture
					{
						const path& RootPath = UPathManager::GetInstance().GetRootPath();
						const path& DataPath = UPathManager::GetInstance().GetDataPath();
						if (TexturePath.is_absolute() && exists(TexturePath))
						{
							return canonical(TexturePath).string();
						}
					// b) Relative to FBX folder
					path Candidate = GCurrentFbxDir / TexturePath;
					if (exists(Candidate))
					{
						return canonical(Candidate).string();
					}

					// c) Relative to engine root
					Candidate = RootPath / TexturePath;
						if (exists(Candidate))
						{
							path RelativePath = relative(canonical(Candidate), RootPath);
							return RelativePath.string();
						}
						FString TargetName = TexturePath.filename().string();
						for (const auto& Entry : std::filesystem::recursive_directory_iterator(DataPath))
						{
							if (!Entry.is_regular_file()) continue;
							if (Entry.path().filename().string() == TargetName)
							{
								path RelativePath = relative(Entry.path(), RootPath);
								return RelativePath.string();
							}
						}
					}
					if (TexturePath.is_absolute())
					{
						FString FileName = TexturePath.filename().string();
						FString FoundPath = FindTextureInDataFolder(MaterialName, "");
						if (!FoundPath.empty())
						{
							return FoundPath;
						}
					}
					return TextureFileName;
				}
			}
		}
	}

	// 텍스처를 찾지 못한 경우 Data 폴더에서 검색
	return FindTextureInDataFolder(MaterialName, "_" + TextureType);
}

void FFbxImporter::ApplySceneRotation(FbxScene* Scene, double Pitch, double Yaw, double Roll)
{

}

FString FFbxImporter::ExtractEmbeddedTexture(FbxFileTexture* FileTexture, const FString& MaterialName, const FString& TextureType)
{
	// FBX SDK는 임베디드 텍스처를 자동으로 .fbm 폴더에 추출합니다.
	// 여기서는 GetFileName()으로 반환된 경로를 그대로 사용하거나,
	// Data 폴더에서 텍스처 파일을 검색합니다.

	if (!FileTexture)
	{
		return "";
	}

	// GetRelativeFileName() 또는 GetFileName() 사용
	const char* RelativeFileName = FileTexture->GetRelativeFileName();
	if (RelativeFileName && strlen(RelativeFileName) > 0)
	{
		path TexturePath(RelativeFileName);

		// Try resolve embedded texture by exact filename inside Data
		{
			const path& RootPath = UPathManager::GetInstance().GetRootPath();
			const path& DataPath = UPathManager::GetInstance().GetDataPath();
			FString TargetName = TexturePath.filename().string();
			for (const auto& Entry : std::filesystem::recursive_directory_iterator(DataPath))
			{
				if (!Entry.is_regular_file()) continue;
				if (Entry.path().filename().string() == TargetName)
				{
					path RelativePath = relative(Entry.path(), RootPath);
					return RelativePath.string();
				}
			}
		}

		// 상대 경로에서 파일명 추출
		FString FileName = TexturePath.filename().string();

		// Data 폴더에서 해당 파일명 검색
		FString FoundPath = FindTextureInDataFolder(MaterialName, "");
		if (!FoundPath.empty())
		{
			return FoundPath;
		}

		// 검색 실패 시 원본 경로 반환
		return RelativeFileName;
	}

	return "";
}

FString FFbxImporter::FindTextureInDataFolder(const FString& MaterialName, const FString& TextureSuffix)
{
	const path DataDirectory = UPathManager::GetInstance().GetDataPath();

	if (!exists(DataDirectory))
	{
		return "";
	}

	// Data 폴더를 재귀적으로 탐색
	for (const auto& Entry : std::filesystem::recursive_directory_iterator(DataDirectory))
	{
		if (!Entry.is_regular_file())
		{
			continue;
		}

		FString FileName = Entry.path().filename().string();
		FString FileNameWithoutExt = Entry.path().stem().string();

		// 패턴 1: MaterialName + TextureSuffix (예: "MI_Manny_02_New_BaseColor")
		FString SearchPattern = MaterialName + TextureSuffix;
		if (FileNameWithoutExt.find(SearchPattern) != FString::npos ||
			FileNameWithoutExt.find("T_" + SearchPattern) != FString::npos)
		{
					// Return path relative to engine root (e.g., Data/Texture/...)
					path RelativePath = relative(Entry.path(), UPathManager::GetInstance().GetRootPath());
					//path RelativePath = relative(Entry.path(), std::filesystem::current_path());
			return RelativePath.string();
		}

		// 패턴 2: Material 이름에서 공통 부분 추출 (예: "MI_Manny_02" -> "Manny_02")
		// TextureSuffix가 "_BaseColor"면 "_D" 매칭, "_Normal"이면 "_N" 매칭
		if (!TextureSuffix.empty())
		{
			FString TextureTypeShort;
			if (TextureSuffix == "_BaseColor") TextureTypeShort = "_D";
			else if (TextureSuffix == "_Normal") TextureTypeShort = "_N";
			else if (TextureSuffix == "_Specular") TextureTypeShort = "_S";

			if (!TextureTypeShort.empty())
			{
				// Material 이름에서 "MI_"나 "M_" 제거하고 매칭 시도
				FString SimplifiedMatName = MaterialName;
				if (SimplifiedMatName.find("MI_") == 0) SimplifiedMatName = SimplifiedMatName.substr(3);
				else if (SimplifiedMatName.find("M_") == 0) SimplifiedMatName = SimplifiedMatName.substr(2);

				// "_New" 같은 접미사 제거
				size_t NewPos = SimplifiedMatName.find("_New");
				if (NewPos != FString::npos) SimplifiedMatName = SimplifiedMatName.substr(0, NewPos);

				// 파일명에 간소화된 이름 + 타입이 포함되어 있는지 확인
				if (FileNameWithoutExt.find(SimplifiedMatName + TextureTypeShort) != FString::npos ||
					FileNameWithoutExt.find("T_" + SimplifiedMatName + TextureTypeShort) != FString::npos)
				{
						path RelativePath = relative(Entry.path(), UPathManager::GetInstance().GetRootPath());
						return RelativePath.string();
					}
				}
		}

		// 패턴 3: MaterialName만으로도 검색 (Suffix가 빈 문자열인 경우)
		if (TextureSuffix.empty() && FileNameWithoutExt.find(MaterialName) != FString::npos)
		{
			path RelativePath = relative(Entry.path(), UPathManager::GetInstance().GetRootPath());
			return RelativePath.string();
		}
	}

	return "";
}

bool FFbxImporter::IsMeshSkinned(const FbxMesh* Mesh)
{
	return GetSkin(Mesh) != nullptr;
}

FbxSkin* FFbxImporter::GetSkin(const FbxMesh* Mesh)
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

FVector FFbxImporter::ConvertPosition(const FbxVector4& FbxVec)
{
	// AxisConversionMatrix 적용
	FbxVector4 Converted = AxisConversionMatrix.MultT(FbxVec);

	// FBX (좌표계 변환 후): Z-up Left-handed
	// 프로젝트: Z-up Left-handed
	// 이미 ConvertScene으로 변환했으므로 직접 매핑
	return {
		static_cast<float>(Converted[0]),
		static_cast<float>(Converted[1]),
		static_cast<float>(Converted[2])
	};
}

FVector FFbxImporter::ConvertNormal(const FbxVector4& FbxVec)
{
	// Normal은 방향 벡터이므로 이동(Translation) 제외하고 회전만 적용
	FbxVector4 Converted = AxisConversionMatrix.MultT(FbxVec);

	FVector Result(
		static_cast<float>(Converted[0]),
		static_cast<float>(Converted[1]),
		static_cast<float>(Converted[2])
	);

	// Normal은 단위 벡터여야 하므로 정규화
	Result.Normalize();
	return Result;
}

FQuat FFbxImporter::ConvertRotation(const FbxQuaternion& FbxQuat)
{
	// Quaternion 변환
	return {
		static_cast<float>(FbxQuat[0]), // X
		static_cast<float>(FbxQuat[1]), // Y
		static_cast<float>(FbxQuat[2]), // Z
		static_cast<float>(FbxQuat[3])  // W
	};
}

FTransform FFbxImporter::ConvertTransform(FbxNode* Node)
{
	FTransform Transform;

	// FBX의 실제 로컬 변환 행렬 계산
	// LclTranslation/Rotation/Scaling은 애니메이션 프로퍼티이므로 EvaluateLocalTransform 사용
	FbxAMatrix LocalMatrix = Node->EvaluateLocalTransform();

	// LocalMatrix를 우리 엔진의 좌표계로 변환
	// TODO: ConversionMatrix는 현재 Identity 임, 폐기될 예정 
	FbxAMatrix ConvertedMatrix = AxisConversionMatrix * LocalMatrix * AxisConversionMatrixInv;

	 
	// Translation
	FbxVector4 Translation = ConvertedMatrix.GetT();
	Transform.Location = FVector(
		static_cast<float>(Translation[0]),
		static_cast<float>(Translation[1]),
		static_cast<float>(Translation[2])
	);

	// Rotation
	FbxQuaternion FbxQuat = ConvertedMatrix.GetQ();
	Transform.Rotation = FQuat(
		static_cast<float>(FbxQuat[0]), // X
		static_cast<float>(FbxQuat[1]), // Y
		static_cast<float>(FbxQuat[2]), // Z
		static_cast<float>(FbxQuat[3])  // W
	);

	// Scale
	FbxVector4 Scaling = ConvertedMatrix.GetS();
	Transform.Scale = FVector(
		static_cast<float>(Scaling[0]),
		static_cast<float>(Scaling[1]),
		static_cast<float>(Scaling[2])
	);

	return Transform;
}

FMatrix FFbxImporter::FbxMatrixToFMatrix(const FbxAMatrix& FbxMat)
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
