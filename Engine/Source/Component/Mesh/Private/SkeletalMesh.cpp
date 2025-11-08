#include "pch.h"
#include "Component/Mesh/Public/SkeletalMesh.h"

void FSkeleton::BuildRefPoseGlobal()
{
	const int32 NumBones = static_cast<int32>(BoneNames.Num());
	if (NumBones <= 0)
	{
		RefPoseGlobal = {};
		InvRefPoseGlobal = {};
		return;
	}
	RefPoseGlobal.SetNum(NumBones);
	InvRefPoseGlobal.SetNum(NumBones);
	 
    if (RefPoseLocal.Num() != NumBones)
    {
        RefPoseLocal.SetNum(NumBones);
        for (int32 i = 0; i < NumBones; ++i)
        {
            RefPoseLocal[i] = FTransform(FVector::ZeroVector(), FVector::ZeroVector(), FVector::OneVector());
        }
    }
	 
    for (int32 i = 0; i < NumBones; ++i)
    {
        const FTransform& L = RefPoseLocal[i];
        const FQuaternion LocalRot = FQuaternion::FromEuler(L.Rotation); 
        const FMatrix LocalM = FMatrix::GetModelMatrix(L.Location, LocalRot, L.Scale);

        const int32 Parent = (i < Parents.Num()) ? Parents[i] : -1;
        if (Parent >= 0)
        {
            RefPoseGlobal[i] = LocalM * RefPoseGlobal[Parent];
        }
        else
        {
            RefPoseGlobal[i] = LocalM; // 루트일 때
        }

        InvRefPoseGlobal[i] = RefPoseGlobal[i].Inverse();
    }
}

int32 FSkeleton::FindBoneIndex(const FName& BoneName) const
{
	for (int32 i = 0; i < BoneNames.Num(); i++)
	{
		if (BoneNames[i] == BoneName)
		{
			return i;
		}
	}
	return -1;
}

void USkeletalMesh::SetSkeletalMeshAsset(FSkeletalMesh* InSkeletalMeshAsset)
{
	SkeletalMesh = InSkeletalMeshAsset;
}
