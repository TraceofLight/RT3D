#pragma once
#include "Component/Mesh/Public/SkinnedMeshComponent.h"
#include "Global/CoreTypes.h"

class UBatchLines;
class USkeletalMeshComponent : public USkinnedMeshComponent
{

    GENERATED_BODY()
    DECLARE_CLASS(USkeletalMeshComponent, USkinnedMeshComponent)

public:
	USkeletalMeshComponent();
	~USkeletalMeshComponent() override;

	void Serialize(bool bInIsLoading, JSON& InOutHandle ) override;
	UClass* GetSpecificWidgetClass() const override;

	/** RefPos를 위한 Matrix 세팅 */
	void UseReferencePose();

    void SetLocalPose(const TArray<FTransform>& InLocalPose);

	FTransform& GetLocalPose(uint32 Idx);
	void SetLocalPose(uint32 Idx, const FTransform& InLocalPose);

	// Bone Transform 설정 (World 좌표 -> Local 좌표 변환 후 LocalPose 업데이트)
	void SetBoneWorldLocation(int32 BoneIndex, const FVector& NewWorldLocation);
	void SetBoneWorldRotation(int32 BoneIndex, const FQuat& NewWorldRotation);
	void SetBoneWorldScale(int32 BoneIndex, const FVector& NewWorldScale);

	/** World Space로 변환 행렬 만들어주는 함수 => GlobalPose[i] = Local * GlobalPose[Parent]*/
    void BuildComponentWorldSpacePose();

	/** 최종으로 사용할 Matrix => LocalM * GlobalPose[i]* InvGlobalPose[i] */
    void BuildSkinMatrices();

    virtual void TickComponent(float DeltaTime) override;

	void RenderDebugBones(UBatchLines& BatchLines, int32 SelectedBoneIdx = -1);

	// Bone Edit Mode (본별 HitProxy 렌더링 활성화)
	void SetBoneEditMode(bool bInEditMode) { bInBoneEditMode = bInEditMode; }
	bool IsInBoneEditMode() const { return bInBoneEditMode; }

	// GlobalPose 접근자
	const TArray<FMatrix>& GetGlobalPose() const { return GlobalPose; }

	// HitProxy 렌더링 (Joint + Bone 입체)
	void RenderBoneHitProxies();

private:
    TArray<FTransform> LocalPose;   // 부모와 상대적인 좌표
    TArray<FMatrix>    GlobalPose;  // component/global space matrices
	bool bInBoneEditMode;           // 본 편집 모드 플래그
};
