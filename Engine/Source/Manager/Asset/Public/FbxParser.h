#pragma once
#include "Texture/Public/Material.h"

/** 영향을 주는 Vertex을 저장한 구조체 */
struct FSkinInfluence
{
	uint16 BoneIndices[4] = {0, 0, 0, 0};
	float BoneWeights[4] = {0.f, 0.f, 0.f, 0.f};

	void Normalize()
	{
		float Sum = BoneWeights[0] + BoneWeights[1] + BoneWeights[2] + BoneWeights[3];
		if (Sum > 1e-8f)
		{
			float Inv = 1.0f / Sum;
			for (int i = 0; i < 4; i++)
			{
				BoneWeights[i] = BoneWeights[i] * Inv;
			}
		}
	}
};

/**
 * Skeletal Mesh를 읽는 Vertex
 * FNormalVertex + FSkinInfluence
 */
struct FSkeletalVertex
{
	FNormalVertex Vertex;
	FSkinInfluence Skin;
};

/** Mesh의 한 부분을 정의한다 */
struct FSkeletalMeshSection
{
	TArray<FSkeletalVertex> Vertices;
	TArray<uint32> Indices;
	TArray<uint16> BoneMap; // 영향을 주는 Bone들
	uint32 MaterialSlot = 0;
};

/** 모든 Skeletal Mesh Bone에 대한 정보 */
struct FSkeleton
{
	TArray<FName> BoneNames; // 모든 Bone 이름의 배열
	TArray<int32> Parents; // 부모 인덱스 배열, -1은 Root
	TArray<FTransform> RefPoseLocal; // 부모 뼈에 대한 상대 변환 값
	TArray<FMatrix> RefPoseGlobal; // Root 기준 월드 변환 행렬
	TArray<FMatrix> InvRefPoseGlobal; // 역행렬 (Skinning용)

	void BuildRefPoseGlobal();
	int32 GetNumBones() const { return BoneNames.Num(); }
	int32 FindBoneIndex(const FName& BoneName) const;
};

struct FSkeletalMesh
{
	FName PathFileName;

	// 뼈대
	FSkeleton* Skeleton = nullptr;

	// 살
	TArray<FSkeletalMeshSection> Sections;

	// 피부
	TArray<FMaterial> MaterialInfo;

	bool IsValid() const { return Skeleton != nullptr && Sections.Num() > 0; }
};

/**
 * FBX 파일에서 Skeletal Mesh를 로드하는 파서
 */
class FFbxParser
{
public:
	FFbxParser();
	virtual ~FFbxParser();

	bool LoadSkeletalMesh(const FString& FilePath, FSkeletalMesh& OutMesh);

private:
	FbxManager* Manager = nullptr;
	FbxScene* Scene = nullptr;

	// FBX SDK 초기화/정리
	void Initialize();
	void Cleanup();

	// Scene 처리
	bool ImportScene(const FString& FilePath);
	void ProcessScene(FSkeletalMesh& OutMesh) const;
	static void ProcessNode(FbxNode* Node, FSkeletalMesh& OutMesh);

	// Skeletal Mesh 처리
	static bool ProcessSkeletalMesh(FbxNode* MeshNode, FSkeletalMesh& OutMesh);
	static void BuildSkeleton(const FbxMesh* Mesh, FSkeleton& OutSkeleton);
	static void BuildMeshSections(FbxMesh* Mesh, FSkeletalMesh& OutMesh);
	static void ExtractMaterials(const FbxNode* Node, FSkeletalMesh& OutMesh);

	// Bone 계층 구조 분석
	static void CollectBones(FbxNode* Node, TArray<FbxNode*>& OutBoneNodes); // Node 재귀 탐색
	static void CollectBones(FbxSkin* Skin, TArray<FbxNode*>& OutBoneNodes); // Skin Cluster 기반
	static void BuildBoneHierarchy(const TArray<FbxNode*>& BoneNodes, FSkeleton& OutSkeleton);
	static int32 FindParentBoneIndex(FbxNode* BoneNode, const TArray<FbxNode*>& BoneNodes);

	// 좌표계 변환 (FBX -> Project)
	static FVector ConvertPosition(const FbxVector4& FbxVec);
	static FVector ConvertNormal(const FbxVector4& FbxVec);
	static FQuaternion ConvertRotation(const FbxQuaternion& FbxQuat);
	static FTransform ConvertTransform(const FbxNode* Node);
	static FMatrix FbxMatrixToFMatrix(const FbxAMatrix& FbxMat);

	// Helper 함수
	static bool IsMeshSkinned(const FbxMesh* Mesh);
	static FbxSkin* GetSkin(const FbxMesh* Mesh);
};
