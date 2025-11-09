#pragma once
#include "Component/Mesh/Public/MeshComponent.h"
#include "Component/Mesh/Public/SkeletalMesh.h"

// Base component for skinned rendering. Holds mesh and final skin matrices.
class USkinnedMeshComponent : public UMeshComponent
{
    GENERATED_BODY()
    DECLARE_CLASS(USkinnedMeshComponent, UMeshComponent)

public:
    void SetSkeletalMesh(USkeletalMesh* InMesh);
    USkeletalMesh* GetSkeletalMesh() const { return SkeletalMesh; }

    void SetSkinMatrices(const TArray<FMatrix>& InMatrices);
    void SetSkinMatrices(const FMatrix* InMatrices, int32 Count);
    const TArray<FMatrix>& GetSkinMatrices() const { return FinalSkinMatrices; }

    static FVector SkinPosition(const FSkeletalVertex& V, const FSkeletalMeshSection& Sec, const TArray<FMatrix>& SkinMats);
    static FVector SkinNormal(const FSkeletalVertex& V, const FSkeletalMeshSection& Sec, const TArray<FMatrix>& SkinMats);

    // Skinned Vertices 접근자 (lazy evaluation)
    const TArray<FNormalVertex>& GetSkinnedVertices() const;
    const TArray<uint32>& GetSkinnedIndices() const;
    void MarkSkinnedVerticesDirty() { bSkinnedVerticesDirty = true; }

private:
    void UpdateSkinnedVerticesCache() const;

protected:
	/** 사용 중인 SkeletalMesh */
    ///FSkeletalMesh* SkeletalMesh = nullptr;
	USkeletalMesh* SkeletalMesh = nullptr;
    TArray<FMatrix> FinalSkinMatrices; // sized to Skeleton->GetNumBones()

    // Skinned Vertices 캐시 및 Dirty Flag
    mutable TArray<FNormalVertex> CachedSkinnedVertices;
    mutable TArray<uint32> CachedSkinnedIndices;
    mutable bool bSkinnedVerticesDirty = true;
};
