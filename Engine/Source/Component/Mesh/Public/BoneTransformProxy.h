#pragma once
#include "Component/Public/SceneComponent.h"

class USkeletalMeshComponent;

// 본 Transform을 Gizmo가 조작할 수 있도록 래핑하는 Proxy 클래스
// FbxViewportWindow에서 본 선택 시 생성되어 Gizmo의 타겟으로 사용됨
UCLASS()
class UBoneTransformProxy : public USceneComponent
{
	GENERATED_BODY()
	DECLARE_CLASS(UBoneTransformProxy, USceneComponent)

public:
	UBoneTransformProxy();
	~UBoneTransformProxy() override;

	// 본 정보 설정
	void SetBoneInfo(USkeletalMeshComponent* InMeshComponent, int32 InBoneIndex);

	// 본 Transform 동기화
	void SyncTransformFromBone();
	void ApplyTransformToBone();

	// Transform setters (본 Transform 적용)
	void SetWorldLocation(const FVector& NewLocation);
	void SetWorldRotation(const FQuat& NewRotation);
	void SetWorldScale3D(const FVector& NewScale);

	// Getters
	USkeletalMeshComponent* GetMeshComponent() const { return MeshComponent; }
	int32 GetBoneIndex() const { return BoneIndex; }

private:
	USkeletalMeshComponent* MeshComponent = nullptr;
	int32 BoneIndex = -1;
};
