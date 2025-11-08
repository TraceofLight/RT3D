#pragma once
#include "Component/Mesh/Public/SkinnedMeshComponent.h"
#include "Global/CoreTypes.h"
#include "Global/Quaternion.h"

class USkeletalMeshComponent : public USkinnedMeshComponent
{

    GENERATED_BODY()
    DECLARE_CLASS(USkeletalMeshComponent, USkinnedMeshComponent)

public:

	/** RefPos를 위한 Matrix 세팅 */
	void UseReferencePose();
	 
    void SetLocalPose(const TArray<FTransform>& InLocalPose);

	/** World Space로 변환 행렬 만들어주는 함수 => GlobalPose[i] = Local * GlobalPose[Parent]*/
    void BuildComponentWorldSpacePose();

	/** 최종으로 사용할 Matrix => LocalM * GlobalPose[i]* InvGlobalPose[i] */
    void BuildSkinMatrices();

    // Optional tick to keep skin up to date each frame
    virtual void TickComponent(float DeltaTime) override;

private:
    TArray<FTransform> LocalPose;   // 부모와 상대적인 좌표
    TArray<FMatrix>    GlobalPose;  // component/global space matrices
};
