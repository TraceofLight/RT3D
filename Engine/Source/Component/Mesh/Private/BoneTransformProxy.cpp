#include "pch.h"
#include "Component/Mesh/Public/BoneTransformProxy.h"
#include "Component/Mesh/Public/SkeletalMeshComponent.h"

IMPLEMENT_CLASS(UBoneTransformProxy, USceneComponent)

UBoneTransformProxy::UBoneTransformProxy()
{
	// BoneTransformProxy는 World Space에서 독립적으로 동작해야 함
	// 부모의 Transform을 무시하고 절대 좌표계 사용
	SetAbsoluteLocation(true);
	SetAbsoluteRotation(true);
	SetAbsoluteScale(true);
}

UBoneTransformProxy::~UBoneTransformProxy() = default;

void UBoneTransformProxy::SetBoneInfo(USkeletalMeshComponent* InMeshComponent, int32 InBoneIndex)
{
	MeshComponent = InMeshComponent;
	BoneIndex = InBoneIndex;

	// 본 Transform으로 초기화
	SyncTransformFromBone();
}

void UBoneTransformProxy::SyncTransformFromBone()
{
	if (!MeshComponent || BoneIndex < 0)
	{
		return;
	}

	const TArray<FMatrix>& GlobalPose = MeshComponent->GetGlobalPose();
	if (BoneIndex >= GlobalPose.Num())
	{
		return;
	}

	// 본의 Component Space Transform을 가져옴
	const FMatrix& BoneComponentMatrix = GlobalPose[BoneIndex];

	// Component Space -> World Space
	const FMatrix& ComponentToWorld = MeshComponent->GetWorldTransformMatrix();
	const FMatrix BoneWorldMatrix = BoneComponentMatrix * ComponentToWorld;

	// World Transform 설정 (Absolute 플래그로 부모 Transform 무시)
	FVector Location = BoneWorldMatrix.GetLocation();
	FQuat Rotation = BoneWorldMatrix.ToQuaternion();
	FVector Scale = BoneWorldMatrix.GetScale();

	USceneComponent::SetWorldLocation(Location);
	USceneComponent::SetWorldRotation(Rotation);
	USceneComponent::SetWorldScale3D(Scale);
}

void UBoneTransformProxy::ApplyTransformToBone()
{
	if (!MeshComponent || BoneIndex < 0)
	{
		return;
	}

	// Proxy의 World Transform을 본에 적용
	const FVector WorldLocation = GetWorldLocation();
	const FQuat WorldRotation = GetWorldRotationAsQuaternion();
	const FVector WorldScale = GetWorldScale3D();

	MeshComponent->SetBoneWorldLocation(BoneIndex, WorldLocation);
	MeshComponent->SetBoneWorldRotation(BoneIndex, WorldRotation);
	MeshComponent->SetBoneWorldScale(BoneIndex, WorldScale);
}

void UBoneTransformProxy::SetWorldLocation(const FVector& NewLocation)
{
	USceneComponent::SetWorldLocation(NewLocation);
	ApplyTransformToBone();
}

void UBoneTransformProxy::SetWorldRotation(const FQuat& NewRotation)
{
	USceneComponent::SetWorldRotation(NewRotation);
	ApplyTransformToBone();
}

void UBoneTransformProxy::SetWorldScale3D(const FVector& NewScale)
{
	USceneComponent::SetWorldScale3D(NewScale);
	ApplyTransformToBone();
}
