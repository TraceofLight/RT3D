#include "pch.h"
#include "Component/Mesh/Public/SkinnedMeshComponent.h"

/*
void USkinnedMeshComponent::SetSkeletalMesh(FSkeletalMesh* InMesh)
{
	SkeletalMesh = InMesh;

	for (int32 i = 0; i < FinalSkinMatrices.Num(); i++)
	{
		FinalSkinMatrices[i] = FMatrix::Identity();
	}
}
*/

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
	FVector Out = FVector::Zero();

	for (int i = 0; i < 4; ++i)
	{
		float Weight = V.Skin.BoneWeights[i];
		if (Weight <= 0.0f) continue;

		uint16 Idx = V.Skin.BoneIndices[i];
		if (Idx < 0 || Idx > Sec.BoneMap.Num() ) continue;

		uint16 SkelIdx = Sec.BoneMap[Idx];
		if (SkelIdx < 0 || SkelIdx > SkinMats.Num()) continue;
		const FMatrix& M = SkinMats[SkelIdx];

		Out += M.TransformPosition(V.Vertex.Position) * Weight;
	}

	return Out;
}

FVector USkinnedMeshComponent::SkinNormal(const FSkeletalVertex& V, const FSkeletalMeshSection& Sec, const TArray<FMatrix>& SkinMats)
{
	FVector Out = FVector::Zero();
	
	for (int i = 0; i < 4; ++i)
	{
		float Weight = V.Skin.BoneWeights[i];
		if (Weight <= 0.0f) continue;

		uint16 Idx = V.Skin.BoneIndices[i];
		if (Idx < 0 || Idx > Sec.BoneMap.Num()) continue;
		
		uint16 SkelIdx = Sec.BoneMap[Idx];
		if (SkelIdx < 0 || SkelIdx > SkinMats.Num()) continue;

		const FMatrix& M = SkinMats[SkelIdx];

		Out += M.TransformPosition(V.Vertex.Normal) * Weight; 
	}

	Out.Normalize();
	return Out;
}
