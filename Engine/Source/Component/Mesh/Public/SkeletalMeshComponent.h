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

	/** World Space로 변환 행렬 만들어주는 함수 => GlobalPose[i] = Local * GlobalPose[Parent]*/
    void BuildComponentWorldSpacePose();

	/** 최종으로 사용할 Matrix => LocalM * GlobalPose[i]* InvGlobalPose[i] */
    void BuildSkinMatrices();

    virtual void TickComponent(float DeltaTime) override;

	void RenderDebugBones(UBatchLines& BatchLines, int32 SelectedBoneIdx = -1);
private:
    TArray<FTransform> LocalPose;   // 부모와 상대적인 좌표
    TArray<FMatrix>    GlobalPose;  // component/global space matrices
};
