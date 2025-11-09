#pragma once

struct FMaterial;
class UMaterial;

/** 영향을 주는 Vertex을 저장한 구조체 */
struct FSkinInfluence
{
    uint16 BoneIndices[4] = { 0, 0, 0, 0 };
    float  BoneWeights[4] = { 0.f, 0.f, 0.f, 0.f };

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
*	Skeletal Mesh를 읽는 Vertex
*
*	FNormalVertex + FSkinIbnfluence
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
    TArray<uint32>          Indices;
    TArray<uint16>          BoneMap;			/** 영향을 주는 Bone들, BoneMap의Indices == Verties.Skin.Indices */
    uint32                  MaterialSlot = 0;
};

// 1. 처음 로드 했을 때
// Vertex는 local space // Root가 0,0,0 일 때, 상대좌표로
// Bone(RefPoseLocal)은 부모 뼈 기준 변환 값으로 불러들인다. (->  원래는 root 기준이여서 후처리해야 됨)

// 2. BuildRefPoseGlobal()을 통해 변환 시작
// RefPoseLocal 데이터 사용해서 RefPoseGlobal, InvRefPoseGlobal을 계산
// Root 부터 시작해서, 자식 뼈로 내려간다.
// RefPoseGlobal[i] = RefPoseLocal[i] * RefPoseGlobal[Parent[i]] => 회전할거면 여기서 이미 계산됨
// RefPoseGlobal: i번 뼈 로컬 공간(부모 상대 -> root가 0,0,0일 떄) => 모델 공간(root가 0,0,0일 때ㅔ 0-
// InvRefPoseGlobal: 모델 공간 => i번 뼈 로컬 공간

// Vertex * InvRefPoseGlobal[i] -> (업데이트 X) * RefPoseGlobal[i] -> (업데이트 0)
// weight 처리

// 모든 SKeletal Mesh Bone  대한 정보
struct FSkeleton
{
    TArray<FName>		BoneNames;				/** 모든 Bone 이름의 배열 */
	TArray<int32>		Parents;				/** 부모 인덱스 배열, -1은 Root */
	//ARRAY<ARRAY<>> Children;
	TArray<FTransform>	RefPoseLocal;			/** 부모 뼈에 대한 상대 변환 값 */
    TArray<FMatrix>		RefPoseGlobal;			/** 각 bone을 Root가 0,0,0인 Space로 변환하는 행렬 */
    TArray<FMatrix>		InvRefPoseGlobal;		/** Root 기준 모델 Space에서 각 뼈의 local Space로 변환 */

	/**
	* RefPoseLocal,RefPoseGlobal, InvRefPoseGlobal 를 세팅해주는 함수
	*/
	void BuildRefPoseGlobal();
	int32 FindBoneIndex(const FName& BoneName) const;
    int32 GetNumBones() const { return static_cast<int32>(BoneNames.Num()); }
};

struct FSkeletalMesh
{
	FName               PathFileName;

	// 뼈대
	FSkeleton*     Skeleton = nullptr;

	// 살
	TArray<FSkeletalMeshSection> Sections;

	// 피부
    TArray<FMaterial>   MaterialInfo;

	bool IsValid() const { return Skeleton != nullptr && Sections.Num() > 0; }
};

UCLASS()
class USkeletalMesh : public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(USkeletalMesh, UObject)

public:
	USkeletalMesh();
	virtual ~USkeletalMesh();

	FSkeletalMesh* GetSkeletalMeshAsset() { return SkeletalMesh; };
	void SetSkeletalMeshAsset(FSkeletalMesh* InSkeletalMeshAsset);

	FSkeleton* GetSkeleton() { return SkeletalMesh->Skeleton; }


	// Geometry Data
	const TArray<FNormalVertex>& GetVertices() const;
	TArray<FNormalVertex>& GetVertices();
	const TArray<uint32>& GetIndices() const;

	// Material Data
	UMaterial* GetMaterial(int32 MaterialIndex) const;
	void SetMaterial(int32 MaterialIndex, UMaterial* Material);
	int32 GetNumMaterials() const;

	// 유효성 검사
	bool IsValid() const { return SkeletalMesh != nullptr; }

private:
	FSkeletalMesh* SkeletalMesh;
	TArray<UMaterial*> Materials;
};
