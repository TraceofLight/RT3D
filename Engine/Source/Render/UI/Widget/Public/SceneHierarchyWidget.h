#pragma once
#include "Widget.h"

class AActor;
class ULevel;
class UCamera;

/**
 * @brief 현재 Level의 모든 Actor들을 트리 형태로 표시하는 Widget
 * Actor를 클릭하면 Level에서 선택되도록 하는 기능 포함
 */
class USceneHierarchyWidget :
	public UWidget
{
public:
	void Initialize() override;
	void Update() override;
	void RenderWidget() override;

	// Camera 설정을 위한 함수
	void SetCamera(UCamera* InCamera) { Camera = InCamera; }

	// Special Member Function
	USceneHierarchyWidget();
	~USceneHierarchyWidget() override;

private:
	// UI 상태
	bool bShowDetails = true;

	// 카메라 참조
	TObjectPtr<UCamera> Camera = nullptr;

	// Camera focus animation
	bool bIsCameraAnimating = false;
	float CameraAnimationTime = 0.0f;
	FVector CameraStartLocation;
	FVector CameraTargetLocation;
	FVector CameraCurrentRotation;

	// Heuristic constant
	static constexpr float CAMERA_ANIMATION_DURATION = 0.8f;
	static constexpr float FOCUS_DISTANCE = 5.0f;

	// Camera movement
	void RenderActorInfo(TObjectPtr<AActor> InActor, int32 InIndex);
	void SelectActor(TObjectPtr<AActor> InActor, bool bFocusCamera = false);
	void FocusOnActor(TObjectPtr<AActor> InActor);
	void UpdateCameraAnimation();
};
