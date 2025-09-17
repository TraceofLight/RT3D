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
	UCamera* Camera = nullptr;

	// 카메라 이동을 위한 상수값
	static constexpr float FOCUS_DISTANCE = 5.0f; // Actor로부터의 기본 거리
	static constexpr float FOCUS_HEIGHT_OFFSET = 2.0f; // 약간 위에서 바라보도록

	void RenderActorInfo(AActor* InActor, int32 InIndex);
	void SelectActor(AActor* InActor);
	void FocusOnActor(const AActor* InActor) const;
};
