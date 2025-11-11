#include "pch.h"
#include "Component/Mesh/Public/SkeletalMeshComponent.h"
#include "Render/UI/Widget/Public/SkeletalMeshComponentWidget.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Utility/Public/JsonSerializer.h"
#include "Editor/Public/BatchLines.h"

IMPLEMENT_CLASS(USkeletalMeshComponent, USkinnedMeshComponent)

USkeletalMeshComponent::USkeletalMeshComponent()
	: bInBoneEditMode(false)
{
}

USkeletalMeshComponent::~USkeletalMeshComponent() = default;

void USkeletalMeshComponent::Serialize(bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	// 불러오기
	if (bInIsLoading)
	{
		FString AssetPath;
		if (FJsonSerializer::ReadString(InOutHandle, "SkeletalMeshAsset", AssetPath))
		{
			UAssetManager& AssetManager = UAssetManager::GetInstance();
			USkeletalMesh* LoadedMesh = AssetManager.LoadSkeletalMesh(AssetPath);
			if (LoadedMesh)
			{
				SetSkeletalMesh(LoadedMesh);
			}
		}
	}
	// 저장
	else
	{
		if (SkeletalMesh && SkeletalMesh->IsValid())
		{
			InOutHandle["SkeletalMeshAsset"] = SkeletalMesh->GetAssetPath().ToString();
		}
	}
}

UClass* USkeletalMeshComponent::GetSpecificWidgetClass() const
{
	UClass* WidgetClass = USkeletalMeshComponentWidget::StaticClass();
	return WidgetClass;
}

void USkeletalMeshComponent::UseReferencePose()
{
	if (!SkeletalMesh || !SkeletalMesh->GetSkeletalMeshAsset()->Skeleton)
	{
		return;
	}
	const int32 NumBones = SkeletalMesh->GetSkeletalMeshAsset()->Skeleton->GetNumBones();

	LocalPose.SetNum(NumBones);
	for (int i = 0; i < NumBones; ++i)
	{
		LocalPose[i] = SkeletalMesh->GetSkeletalMeshAsset()->Skeleton->RefPoseLocal[i];
	}

	BuildComponentWorldSpacePose();
    BuildSkinMatrices();
    bSkinnedVerticesDirty = true;
}

void USkeletalMeshComponent::SetLocalPose(const TArray<FTransform>& InLocalPose)
{
    if (!SkeletalMesh || !SkeletalMesh->GetSkeleton())
    {
	    return;
    }
    const int32 NumBones = SkeletalMesh->GetSkeletalMeshAsset()->Skeleton->GetNumBones();
    if (InLocalPose.Num() != NumBones)
    {
	    return;
    }
    LocalPose = InLocalPose;
    bSkinnedVerticesDirty = true;
}
FTransform& USkeletalMeshComponent::GetLocalPose(uint32 Idx)
{
	return LocalPose[Idx];
}
void USkeletalMeshComponent::SetLocalPose(uint32 Idx, const FTransform& InLocalPose)
{
	LocalPose[Idx] = InLocalPose;
	BuildComponentWorldSpacePose();
	BuildSkinMatrices();
}

void USkeletalMeshComponent::SetBoneWorldLocation(int32 BoneIndex, const FVector& NewWorldLocation)
{
	if (!SkeletalMesh || !SkeletalMesh->GetSkeleton())
	{
		return;
	}

	const FSkeleton& Skel = *SkeletalMesh->GetSkeleton();
	if (BoneIndex < 0 || BoneIndex >= Skel.GetNumBones())
	{
		return;
	}

	// World -> Component Space
	const FMatrix& ComponentToWorld = GetWorldTransformMatrix();
	const FMatrix WorldToComponent = ComponentToWorld.Inverse();
	const FVector BoneComponentLocation = WorldToComponent.TransformPosition(NewWorldLocation);

	// Component Space -> Local Space (부모 기준)
	const int32 ParentIdx = Skel.Parents[BoneIndex];
	if (ParentIdx >= 0 && ParentIdx < GlobalPose.Num())
	{
		const FMatrix& ParentGlobal = GlobalPose[ParentIdx];
		const FMatrix ParentGlobalInv = ParentGlobal.Inverse();
		const FVector BoneLocalLocation = ParentGlobalInv.TransformPosition(BoneComponentLocation);

		LocalPose[BoneIndex].Location = BoneLocalLocation;
	}
	else
	{
		// 루트 본인 경우 Component Space = Local Space
		LocalPose[BoneIndex].Location = BoneComponentLocation;
	}

	// Pose 재계산
	BuildComponentWorldSpacePose();
	BuildSkinMatrices();
	bSkinnedVerticesDirty = true;
}

void USkeletalMeshComponent::SetBoneWorldRotation(int32 BoneIndex, const FQuat& NewWorldRotation)
{
	if (!SkeletalMesh || !SkeletalMesh->GetSkeleton())
	{
		return;
	}

	const FSkeleton& Skel = *SkeletalMesh->GetSkeleton();
	if (BoneIndex < 0 || BoneIndex >= Skel.GetNumBones())
	{
		return;
	}

	// World -> Component Space
	const FQuat ComponentRotation = GetWorldRotationAsQuaternion();
	const FQuat ComponentRotationInv = ComponentRotation.Inverse();
	const FQuat BoneComponentRotation = ComponentRotationInv * NewWorldRotation;

	// Component Space -> Local Space (부모 기준)
	const int32 ParentIdx = Skel.Parents[BoneIndex];
	if (ParentIdx >= 0 && ParentIdx < GlobalPose.Num())
	{
		const FQuat ParentGlobalRotation = GlobalPose[ParentIdx].ToQuaternion();
		const FQuat ParentGlobalRotationInv = ParentGlobalRotation.Inverse();
		const FQuat BoneLocalRotation = ParentGlobalRotationInv * BoneComponentRotation;

		LocalPose[BoneIndex].Rotation = BoneLocalRotation;
	}
	else
	{
		// 루트 본인 경우 Component Space = Local Space
		LocalPose[BoneIndex].Rotation = BoneComponentRotation;
	}

	// Pose 재계산
	BuildComponentWorldSpacePose();
	BuildSkinMatrices();
	bSkinnedVerticesDirty = true;
}

void USkeletalMeshComponent::SetBoneWorldScale(int32 BoneIndex, const FVector& NewWorldScale)
{
	if (!SkeletalMesh || !SkeletalMesh->GetSkeleton())
	{
		return;
	}

	const FSkeleton& Skel = *SkeletalMesh->GetSkeleton();
	if (BoneIndex < 0 || BoneIndex >= Skel.GetNumBones())
	{
		return;
	}

	// World -> Component Space
	const FVector ComponentScale = GetWorldScale3D();
	const FVector BoneComponentScale = FVector(
		NewWorldScale.X / ComponentScale.X,
		NewWorldScale.Y / ComponentScale.Y,
		NewWorldScale.Z / ComponentScale.Z
	);

	// Component Space -> Local Space (부모 기준)
	const int32 ParentIdx = Skel.Parents[BoneIndex];
	if (ParentIdx >= 0 && ParentIdx < GlobalPose.Num())
	{
		const FVector ParentGlobalScale = GlobalPose[ParentIdx].GetScale();
		const FVector BoneLocalScale = FVector(
			BoneComponentScale.X / ParentGlobalScale.X,
			BoneComponentScale.Y / ParentGlobalScale.Y,
			BoneComponentScale.Z / ParentGlobalScale.Z
		);

		LocalPose[BoneIndex].Scale = BoneLocalScale;
	}
	else
	{
		// 루트 본인 경우 Component Space = Local Space
		LocalPose[BoneIndex].Scale = BoneComponentScale;
	}

	// Pose 재계산
	BuildComponentWorldSpacePose();
	BuildSkinMatrices();
	bSkinnedVerticesDirty = true;
}
void USkeletalMeshComponent::BuildComponentWorldSpacePose()
{
	if (!SkeletalMesh || !SkeletalMesh->GetSkeleton())
	{
		return;
	}
	const FSkeleton& Skel = *(SkeletalMesh->GetSkeleton());
	const int32 NumBones = Skel.GetNumBones();

	if (LocalPose.Num() != NumBones)
	{
		return;
	}

    GlobalPose.SetNum(NumBones);

    for (int32 i = 0; i < NumBones; ++i)
    {
		const FTransform& L = LocalPose[i];
		const FQuat LocalRot = L.Rotation;
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
	if (!SkeletalMesh || !SkeletalMesh->GetSkeleton())
	{
		return;
	}

    const FSkeleton& Skel = *SkeletalMesh->GetSkeleton();
    const int32 NumBones = Skel.GetNumBones();
    if (GlobalPose.Num() != NumBones)
    {
	    return;
    }

	// 최종으로 사용할 matrix (부모 기준 변환)
    FinalSkinMatrices.SetNum(NumBones);
    for (int32 i = 0; i < NumBones; ++i)
    {
		FinalSkinMatrices[i] = Skel.InvRefPoseGlobal[i] * GlobalPose[i];
	}
    bSkinnedVerticesDirty = true;
}

/** 매틱 Matrix 를 업데이트 해준다. GlobalPose은 업데이트, InvGlobalPose는 X */
void USkeletalMeshComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    if (!SkeletalMesh || !SkeletalMesh->GetSkeleton())
    {
	    return;
    }
    if (LocalPose.IsEmpty())
    {
        UseReferencePose();
        return;
    }
    BuildComponentWorldSpacePose();
    BuildSkinMatrices();
}

void USkeletalMeshComponent::RenderDebugBones(UBatchLines& BatchLines, int32 SelectedBoneIdx)
{
	if (!SkeletalMesh || !SkeletalMesh->GetSkeleton()) return;

	if (LocalPose.IsEmpty()) {
		UseReferencePose();
	}

	// 컴포넌트 -> 월드 행렬 구성
	const FVector      WorldLocation = GetWorldLocation();
	const FQuat  WorldRotation = GetWorldRotationAsQuaternion();
	const FVector      WorldScale = GetWorldScale3D();
	const FMatrix      CompToWorld = FMatrix::GetModelMatrix(WorldLocation, WorldRotation, WorldScale);

	// 라인 생성
	BatchLines.UpdateSkeletonVertices(
		SkeletalMesh->GetSkeleton(),
		GlobalPose,                 // 현재 프레임 본 글로벌
		CompToWorld,
		SelectedBoneIdx,
		0.7f,
		0.06f,
		0.35f
	);
}

