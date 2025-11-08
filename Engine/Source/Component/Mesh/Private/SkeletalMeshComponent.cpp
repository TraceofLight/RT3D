#include "pch.h"
#include "Component/Mesh/Public/SkeletalMeshComponent.h"

void USkeletalMeshComponent::UseReferencePose()
{
	if (!SkeletalMesh || !SkeletalMesh->Skeleton) return;
	const int32 NumBones = SkeletalMesh->Skeleton->GetNumBones();

	LocalPose.SetNum(NumBones);
	for (int i = 0; i < NumBones; ++i)
	{
		LocalPose[i] = SkeletalMesh->Skeleton->RefPoseLocal[i];
	}

	BuildComponentWorldSpacePose();
    BuildSkinMatrices();
}

void USkeletalMeshComponent::SetLocalPose(const TArray<FTransform>& InLocalPose)
{
    if (!SkeletalMesh || !SkeletalMesh->Skeleton) return;
    const int32 NumBones = SkeletalMesh->Skeleton->GetNumBones();
    if (InLocalPose.Num() != NumBones) return;
    LocalPose = InLocalPose;
}

void USkeletalMeshComponent::BuildComponentWorldSpacePose()
{
	if (!SkeletalMesh || !SkeletalMesh->Skeleton) return;
	const FSkeleton& Skel = *(SkeletalMesh->Skeleton);
	const int32 NumBones = Skel.GetNumBones();

	if (LocalPose.Num() != NumBones) return;

    GlobalPose.SetNum(NumBones);

    for (int32 i = 0; i < NumBones; ++i)
    {
		const FTransform& L = LocalPose[i];
		const FQuaternion LocalRot = FQuaternion::FromEuler(L.Rotation);
		const FMatrix LocalM = FMatrix::GetModelMatrix(L.Location, LocalRot, L.Scale);

		const int32 Parent = ( i < Skel.Parents.Num() ) ? Skel.Parents[i] : -1;

		if (Parent >= 0)
		{
			GlobalPose[i] = LocalM * GlobalPose[Parent];
		}
		else
		{
			GlobalPose[i] = LocalM;
		}
    }
}

void USkeletalMeshComponent::BuildSkinMatrices()
{
	if (!SkeletalMesh || !SkeletalMesh->Skeleton) return;

    const FSkeleton& Skel = *SkeletalMesh->Skeleton;
    const int32 NumBones = Skel.GetNumBones();
    if (GlobalPose.Num() != NumBones) return;

	//최종으로 사용할 matrix 
    FinalSkinMatrices.SetNum(NumBones);
    for (int32 i = 0; i < NumBones; ++i)
    {
		FinalSkinMatrices[i] = GlobalPose[i] * Skel.InvRefPoseGlobal[i];
	}
}

/** 매틱 Matrix 를 업데이트 해준다. GlobalPose은 업데이트, InvGlobalPose는 X */
void USkeletalMeshComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    if (!SkeletalMesh || !SkeletalMesh->Skeleton) return;
    if (LocalPose.IsEmpty())
    {
        UseReferencePose();
        return;
    }
    BuildComponentWorldSpacePose();
    BuildSkinMatrices();
}

