#pragma once
#include "Core/Public/Object.h"
#include "Editor/Public/Gizmo.h"
#include "Editor/Public/Grid.h"
#include "Editor/Public/ObjectPicker.h"
#include "Editor/Public/BatchLines.h"

class UEditor :
	public UObject
{
	DECLARE_CLASS(UEditor, UObject)

public:
	UEditor();
	~UEditor() override;

	void Update();
	void RenderEditorGeometry();
	void Collect2DRender(FViewportClient* InClient, const D3D11_VIEWPORT& InViewport, bool bIsPIEViewport);
	void RenderGizmo(FViewportClient* InClient, const D3D11_VIEWPORT& InViewport);
	void RenderGizmoForHitProxy(FViewportClient* InClient, const D3D11_VIEWPORT& InViewport);

	void SelectActor(AActor* InActor);
	void SelectComponent(UActorComponent* InComponent);
	void SelectActorAndComponent(AActor* InActor, UActorComponent* InComponent);
	void FocusOnSelectedActor();

	// Alt + Drag 복사 기능
	UActorComponent* DuplicateComponent(UActorComponent* InSourceComponent, AActor* InParentActor);
	AActor* DuplicateActor(AActor* InSourceActor);

	// Getter & Setter
	EViewModeIndex GetViewMode() const { return CurrentViewMode; }
	AActor* GetSelectedActor() const { return SelectedActor.Get(); }
	UActorComponent* GetSelectedComponent() const { return SelectedComponent.Get(); }
	bool IsPilotMode() const { return bIsPilotMode; }
	AActor* GetPilotedActor() const { return PilotedActor; }
	UBatchLines* GetBatchLines() { return &BatchLines; }
	UGizmo* GetGizmo() { return &Gizmo; }

	void SetViewMode(EViewModeIndex InNewViewMode) { CurrentViewMode = InNewViewMode; }

	// Pilot Mode Public Interface
	void RequestExitPilotMode();

private:
	UObjectPicker ObjectPicker;
	TWeakObjectPtr<AActor> SelectedActor = nullptr; // 선택된 액터
	TWeakObjectPtr<UActorComponent> SelectedComponent = nullptr; // 선택된 컴포넌트

	// 선택 타입 (Actor vs Component)
	bool bIsActorSelected = true; // true: Actor 선택 (Root Component), false: Component 선택

	// Alt + 드래그 복사 모드
	bool bIsInCopyMode = false; // Alt 키 누른 상태로 드래그 시작 시 true
	AActor* CopiedActor = nullptr; // 복사된 Actor (복사 모드 시)
	UActorComponent* CopiedComponent = nullptr; // 복사된 Component (복사 모드 시)

	// 기즈모 드래그 상태 (뷰포트별 독립 관리)
	bool bWasDraggingGizmo = false;

	UGizmo Gizmo;
	UBatchLines BatchLines;

	EViewModeIndex CurrentViewMode = EViewModeIndex::VMI_BlinnPhong;

	int32 ActiveViewportIndex = 0;

	// 드래그 중 뷰포트 고정 처리를 위한 트래킹
	int32 LockedViewportIndexForDrag = -1;
	bool bWasRightMouseDown = false;

	// 최소 스케일 값 설정
	static constexpr float MIN_SCALE_VALUE = 0.01f;

	// Camera focus animation
	bool bIsCameraAnimating = false;
	float CameraAnimationTime = 0.0f;
	EViewType AnimatingViewType = EViewType::Perspective;
	TArray<FVector> CameraStartLocation;
	TArray<FVector> CameraStartRotation;
	TArray<FVector> CameraTargetLocation;
	TArray<FVector> CameraTargetRotation;
	TArray<float> OrthoZoomStart;
	TArray<float> OrthoZoomTarget;
	static constexpr float CAMERA_ANIMATION_DURATION = 0.3f;

	// Pilot Mode
	bool bIsPilotMode = false;
	AActor* PilotedActor = nullptr;
	int32 PilotModeViewportIndex = -1;
	FVector PilotModeStartCameraLocation;
	FVector PilotModeStartCameraRotation;
	FVector PilotModeFixedGizmoLocation;

	void UpdateBatchLines();
	void ProcessMouseInput();

	// 모든 기즈모 드래그 함수가 ViewportClient를 받도록 통일
	FVector GetGizmoDragLocation(FViewportClient* InClient, FRay& WorldRay);
	FQuaternion GetGizmoDragRotation(FViewportClient* InClient, FRay& WorldRay);
	FVector GetGizmoDragScale(FViewportClient* InClient, FRay& WorldRay);

	// Focus Target Calculation
	bool GetComponentFocusTarget(UActorComponent* Component, FVector& OutCenter, float& OutRadius);
	bool GetActorFocusTarget(AActor* Actor, FVector& OutCenter, float& OutRadius);

	void UpdateCameraAnimation();
	void UpdateViewportCameraInput();

	void TogglePilotMode();
	void UpdatePilotMode();
	void ExitPilotMode();
};
