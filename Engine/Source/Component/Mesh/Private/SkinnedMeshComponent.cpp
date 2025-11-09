#include "pch.h"
#include "Component/Mesh/Public/SkinnedMeshComponent.h"
#include "Component/Mesh/Public/SkeletalMeshComponent.h"

IMPLEMENT_CLASS(USkinnedMeshComponent, UMeshComponent)

void USkinnedMeshComponent::SetSkeletalMesh(USkeletalMesh* InMesh)
{
    SkeletalMesh = InMesh;
    FinalSkinMatrices.Empty();

    if (SkeletalMesh && SkeletalMesh->GetSkeleton())
    {
        FinalSkinMatrices.SetNum(SkeletalMesh->GetSkeleton()->GetNumBones());
        // Initialize with identity skinning (ref pose)
        for (int32 i = 0; i < FinalSkinMatrices.Num(); ++i)
        {
            FinalSkinMatrices[i] = FMatrix::Identity();
        }

        // USkeletalMeshComponent인 경우 UseReferencePose 호출
        if (USkeletalMeshComponent* SkelComp = Cast<USkeletalMeshComponent>(this))
        {
            SkelComp->UseReferencePose();
        }
    }
}

void USkinnedMeshComponent::SetSkinMatrices(const TArray<FMatrix>& InMatrices)
{
    FinalSkinMatrices = InMatrices;
}

void USkinnedMeshComponent::SetSkinMatrices(const FMatrix* InMatrices, int32 Count)
{
    if (!InMatrices || Count <= 0) { FinalSkinMatrices.Empty(); return; }
    FinalSkinMatrices.SetNum(Count);
    for (int32 i = 0; i < Count; ++i) { FinalSkinMatrices[i] = InMatrices[i]; }
}

FVector USkinnedMeshComponent::SkinPosition(const FSkeletalVertex& V, const FSkeletalMeshSection& Sec, const TArray<FMatrix>& SkinMats)
{
    FVector Out = FVector::ZeroVector();
    for (int k = 0; k < 4; ++k)
    {
        const float w = V.Skin.BoneWeights[k];
        if (w <= 0.0f) continue;
        const uint16 secIdx = V.Skin.BoneIndices[k];
        if (secIdx >= Sec.BoneMap.Num()) continue;
        const uint16 skelIdx = Sec.BoneMap[secIdx];
        if (skelIdx >= SkinMats.Num()) continue;
        const FMatrix& M = SkinMats[skelIdx];
        Out += M.TransformPosition(V.Vertex.Position) * w;
    }
    return Out;
}

FVector USkinnedMeshComponent::SkinNormal(const FSkeletalVertex& V, const FSkeletalMeshSection& Sec, const TArray<FMatrix>& SkinMats)
{
    FVector Out = FVector::ZeroVector();
    for (int k = 0; k < 4; ++k)
    {
        const float w = V.Skin.BoneWeights[k];
        if (w <= 0.0f) continue;
        const uint16 secIdx = V.Skin.BoneIndices[k];
        if (secIdx >= Sec.BoneMap.Num()) continue;
        const uint16 skelIdx = Sec.BoneMap[secIdx];
        if (skelIdx >= SkinMats.Num()) continue;
        const FMatrix& M = SkinMats[skelIdx];
        Out += M.TransformVector(V.Vertex.Normal) * w; // ignore translation
    }
    Out.Normalize();
    return Out;
}
