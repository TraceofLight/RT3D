#pragma once
#include "Component/Mesh/Public/MeshComponent.h"
#include "Component/Mesh/Public/SkeletalMesh.h"

// Base component for skinned rendering. Holds mesh and final skin matrices.
class USkinnedMeshComponent : public UMeshComponent
{
    GENERATED_BODY()
    DECLARE_CLASS(USkinnedMeshComponent, UMeshComponent)

public:
    void SetSkeletalMesh(FSkeletalMesh* InMesh);
    FSkeletalMesh* GetSkeletalMesh() const { return SkeletalMesh; }
	 
    void SetSkinMatrices(const TArray<FMatrix>& InMatrices);
    void SetSkinMatrices(const FMatrix* InMatrices, int32 Count);
    const TArray<FMatrix>& GetSkinMatrices() const { return FinalSkinMatrices; }
	 
    static FVector SkinPosition(const FSkeletalVertex& V, const FSkeletalMeshSection& Sec, const TArray<FMatrix>& SkinMats);
    static FVector SkinNormal(const FSkeletalVertex& V, const FSkeletalMeshSection& Sec, const TArray<FMatrix>& SkinMats);

protected:
	/** 사용 중인 SkeletalMesh */
    FSkeletalMesh* SkeletalMesh = nullptr;
    TArray<FMatrix> FinalSkinMatrices; // sized to Skeleton->GetNumBones()
};
