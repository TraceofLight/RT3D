#pragma once
#include "Component/Mesh/Public/SkeletalMesh.h"

struct FStaticMesh;

/**
 * @brief FBX 파일에서 Mesh를 로드하는 클래스
 */
class FFbxImporter
{
public:
	// Meyers Singleton
	static FFbxImporter& GetInstance()
	{
		static FFbxImporter Instance;
		return Instance;
	}

	// 복사 / 이동 방지
	FFbxImporter(const FFbxImporter&) = delete;
	FFbxImporter& operator=(const FFbxImporter&) = delete;
	FFbxImporter(FFbxImporter&&) = delete;
	FFbxImporter& operator=(FFbxImporter&&) = delete;

	bool LoadSkeletalMesh(const FString& FilePath, FSkeletalMesh& OutMesh);
	bool LoadStaticMesh(const FString& FilePath, FStaticMesh& OutMesh);

	static bool IsSkeletalMesh(const FString& FilePath);

private:
	FbxManager* Manager = nullptr;
	FbxScene* Scene = nullptr;

	FFbxImporter();
	~FFbxImporter();

	// FBX SDK 초기화/정리
	void Initialize();
	void Cleanup();

	// Scene 처리
	bool ImportScene(const FString& FilePath);
	void ProcessScene(FSkeletalMesh& OutMesh) const;
	void ProcessSceneAsStatic(FStaticMesh& OutMesh) const;
	static void ProcessNode(FbxNode* Node, FSkeletalMesh& OutMesh);
	static void ProcessNodeAsStatic(FbxNode* Node, FStaticMesh& OutMesh);

	// Skeletal Mesh 처리
	static bool ProcessSkeletalMesh(FbxNode* MeshNode, FSkeletalMesh& OutMesh);
	static bool ProcessStaticMeshAsSkeletal(FbxNode* MeshNode, FSkeletalMesh& OutMesh);
	static void BuildSkeleton(const FbxMesh* Mesh, FSkeleton& OutSkeleton);
	static void BuildMeshSections(FbxMesh* Mesh, FSkeletalMesh& OutMesh);
	static void ExtractMaterials(const FbxNode* Node, FSkeletalMesh& OutMesh);
	static void ExtractMaterialsForStatic(const FbxNode* Node, FStaticMesh& OutMesh);

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

	// Texture 재귀 탐색 함수
	static FString FindTextureInDataFolder(const FString& MaterialName, const FString& TextureSuffix);

	// FBX 임베디드 텍스처 추출
	static FString ExtractEmbeddedTexture(FbxFileTexture* FileTexture, const FString& MaterialName, const FString& TextureType);
	static FString GetTextureFilePath(FbxProperty& Property, const FString& MaterialName, const FString& TextureType);
};
