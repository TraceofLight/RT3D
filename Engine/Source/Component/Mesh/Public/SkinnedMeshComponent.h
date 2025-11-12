#pragma once
#include "Component/Mesh/Public/MeshComponent.h"
#include "Component/Mesh/Public/SkeletalMesh.h"
#include "Source/Physics/Public/AABB.h"


struct FDynamicMeshBuffer;

// Base component for skinned rendering. Holds mesh and final skin matrices.
class USkinnedMeshComponent : public UMeshComponent
{
    GENERATED_BODY()
    DECLARE_CLASS(USkinnedMeshComponent, UMeshComponent)

public:
	USkinnedMeshComponent();
	~USkinnedMeshComponent();
    void SetSkeletalMesh(USkeletalMesh* InMesh);
    USkeletalMesh* GetSkeletalMesh() const { return SkeletalMesh; }

    void SetSkinMatrices(const TArray<FMatrix>& InMatrices);
    void SetSkinMatrices(const FMatrix* InMatrices, int32 Count);
    const TArray<FMatrix>& GetSkinMatrices() const { return FinalSkinMatrices; }

	static FVector SkinTangent(const FSkeletalVertex& V, const FSkeletalMeshSection& Sec, const TArray<FMatrix>& SkinMats, const FVector& Normal); 
    static FVector SkinPosition(const FSkeletalVertex& V, const FSkeletalMeshSection& Sec, const TArray<FMatrix>& SkinMats);
    static FVector SkinNormal(const FSkeletalVertex& V, const FSkeletalMeshSection& Sec, const TArray<FMatrix>& SkinMats);

    // Skinned Vertices 접근자 (lazy evaluation)
    const TArray<FNormalVertex>& GetSkinnedVertices() const;
    const TArray<uint32>& GetSkinnedIndices() const;
    void MarkSkinnedVerticesDirty() { bSkinnedVerticesDirty = true; }

	const TArray<FNormalVertex>* GetVerticesData() const override;
	const TArray<uint32>* GetIndicesData() const override;
	ID3D11Buffer* GetVertexBuffer() const override;
	ID3D11Buffer* GetIndexBuffer() const override;

	UMaterial* GetMaterial(int32 ElementIndex) const override;
private:
    void UpdateSkinnedVerticesCache() const;

protected:
	/** 사용 중인 SkeletalMesh */
    ///FSkeletalMesh* SkeletalMesh = nullptr;
	USkeletalMesh* SkeletalMesh = nullptr;
    TArray<FMatrix> FinalSkinMatrices; // sized to Skeleton->GetNumBones()
	//GlobalPose * InvGlobal

	// Skinned Vertices 캐시 및 Dirty Flag
    mutable TArray<FNormalVertex> CachedSkinnedVertices;
    mutable TArray<uint32> CachedSkinnedIndices;
    mutable bool bSkinnedVerticesDirty = true;

	FDynamicMeshBuffer* DynamicMeshBuffer;
};
