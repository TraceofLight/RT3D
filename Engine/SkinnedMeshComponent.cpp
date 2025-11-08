#include "pch.h"
#include "Component/Mesh/Public/SkinnedMeshComponent.h"

void USkinnedMeshComponent::SetSkeletalMesh(FSkeletalMesh* InMesh)
{
	SkeletalMesh = InMesh;

	for (int32 i = 0; i < FinalSkinMatrices.Num(); i++)
	{
		FinalSkinMatrices[i] = FMatrix::Identity();
	}
}

void USkinnedMeshComponent::SetSkinMatrices(const TArray<FMatrix>& InMatrices)
{
	FinalSkinMatrices = InMatrices;
}

void USkinnedMeshComponent::SetSkinMatrices(const FMatrix* InMatrices, int32 Count)
{
	if (!InMatrices || Count < 0)
	{
		return;
	}

	FinalSkinMatrices.SetNum(Count);
	for (int i = 0; i < Count; ++i)
	{
		FinalSkinMatrices[i] = InMatrices[i];
	} 
}

FVector USkinnedMeshComponent::SkinPosition(const FSkeletalVertex& V, const FSkeletalMeshSection& Sec, const TArray<FMatrix>& SkinMats)
{
	return FVector();
}

FVector USkinnedMeshComponent::SkinNormal(const FSkeletalVertex& V, const FSkeletalMeshSection& Sec, const TArray<FMatrix>& SkinMats)
{
	return FVector();
}
