#pragma once
#include "Editor/Public/EditorPrimitive.h"
#include "Editor/Public/GizmoTypes.h"
#include "Runtime/CoreUObject/Public/Object.h"
#include "Actor/Public/Actor.h"

class FViewportClient;
class UConfigManager;

/**
 * @brief Gizmo 렌더링용 배칭 구조체
 * - 여러 메쉬를 한 번에 수집하고 일괄 렌더링
 * - 버퍼 생성/해제 보일러플레이트 제거
 */
struct FGizmoBatchRenderer
{
	struct FBatchedMesh
	{
		TArray<FNormalVertex> Vertices;
		TArray<uint32> Indices;
		FVector4 Color;
		FVector Location;
		FQuat Rotation;
		FVector Scale;
		bool bAlwaysVisible;

		FBatchedMesh()
			: Color(1, 1, 1, 1)
			, Location(0, 0, 0)
			, Rotation(FQuat::Identity())
			, Scale(1, 1, 1)
			, bAlwaysVisible(true)
		{}
	};

	TArray<FBatchedMesh> Meshes;

	void AddMesh(const TArray<FNormalVertex>& InVertices, const TArray<uint32>& InIndices,
	             const FVector4& InColor, const FVector& InLocation,
	             const FQuat& InRotation = FQuat::Identity(),
	             const FVector& InScale = FVector(1, 1, 1));

	void FlushAndRender(const FRenderState& InRenderState);

	void Clear();
};

class UGizmo :
	public UObject
{
	GENERATED_BODY()
	DECLARE_CLASS(UGizmo, UObject)

public:
	UGizmo();
	~UGizmo() override;
	void UpdateScale(const FViewportClient* InClient, const D3D11_VIEWPORT& InViewport, bool bUpdateTargetFromEditor = true);
	void RenderGizmo(FViewportClient* InClient, const D3D11_VIEWPORT& InViewport, bool bUpdateTargetFromEditor = true);
	void RenderForHitProxy(FViewportClient* InClient, const D3D11_VIEWPORT& InViewport);
	void CollectRotationAngleOverlay(class FD2DOverlayManager& OverlayManager, FViewportClient* InClient, const D3D11_VIEWPORT& InViewport);
	void ChangeGizmoMode();
	void SetGizmoMode(EGizmoMode Mode);

	// 스크린 공간 축 방향 벡터 계산 (멀티 뷰포트 대응)
	void CalculateScreenAxes(const FViewportClient* InClient, const D3D11_VIEWPORT& InViewport,
	                         FVector2& OutScreenAxisX, FVector2& OutScreenAxisY, FVector2& OutScreenAxisZ, FVector2& OutScreenOrigin) const;

	/**
	 * @brief Setter
	 */
	void SetLocation(const FVector& Location) const;
	void SetGizmoDirection(EGizmoDirection Direction) { GizmoDirection = Direction; }
	void SetComponentRotation(const FQuat& Rotation) const { TargetComponent->SetWorldRotation(Rotation); }
	void SetComponentScale(const FVector& Scale) const { TargetComponent->SetWorldScale3D(Scale); }
	void SetPreviousMouseLocation(const FVector& Location) { PreviousMouseLocation = Location; }
	void SetCurrentRotationAngle(float Angle) { CurrentRotationAngle = Angle; }
	void SetPreviousScreenPos(const FVector2& ScreenPos) { PreviousScreenPos = ScreenPos; }

	void SetWorld() { bIsWorld = true; }
	void SetLocal() { bIsWorld = false; }
	bool IsWorldMode() const { return bIsWorld; }

	// Pilot Mode 기즈모 고정 위치
	void SetFixedLocation(const FVector& InFixedLocation) { bUseFixedLocation = true; FixedLocation = InFixedLocation; }
	void ClearFixedLocation() { bUseFixedLocation = false; }
	bool IsFixedLocation() const { return bUseFixedLocation; }
	FVector GetFixedLocation() const { return FixedLocation; }

	// Getter
	float GetTranslateScale() const { return TranslateCollisionConfig.Scale; }
	float GetRotateScale() const { return RotateCollisionConfig.Scale; }
	EGizmoDirection GetGizmoDirection() const { return GizmoDirection; }
	FVector GetGizmoLocation() const
	{
		// Pilot Mode일 때 고정 위치 사용
		if (bUseFixedLocation)
		{
			return FixedLocation;
		}
		// 충돌 판정과 렌더링이 같은 위치를 사용하도록 TargetComponent 직접 참조
		if (TargetComponent)
		{
			return TargetComponent->GetWorldLocation();
		}
		return FVector(0, 0, 0);
	}
	FQuat GetComponentRotation() const { return TargetComponent->GetWorldRotationAsQuaternion(); }
	FVector GetComponentScale() const { return TargetComponent->GetWorldScale3D(); }
	FVector GetDragStartMouseLocation() { return DragStartMouseLocation; }
	FVector GetDragStartActorLocation() { return DragStartActorLocation; }
	FVector GetDragStartActorRotation() { return DragStartActorRotation; }
	FQuat GetDragStartActorRotationQuat() const { return DragStartActorRotationQuat; }
	FVector GetDragStartActorScale() { return DragStartActorScale; }
	EGizmoMode GetGizmoMode() const { return GizmoMode; }
	FVector GetGizmoAxis() const
	{
		switch (GizmoDirection)
		{
		case EGizmoDirection::Forward:	return {1, 0, 0};  // X축 회전 (YZ 평면)
		case EGizmoDirection::Right:	return {0, 1, 0};  // Y축 회전 (XZ 평면)
		case EGizmoDirection::Up:		return {0, 0, 1};  // Z축 회전 (XY 평면)
		default:						return {0, 1, 0};
		}
	}

	// 평면 법선 반환 (평면 방향에 수직인 축)
	FVector GetPlaneNormal() const
	{
		switch (GizmoDirection)
		{
		case EGizmoDirection::XY_Plane:	return {0, 0, 1};  // Z축
		case EGizmoDirection::XZ_Plane:	return {0, 1, 0};  // Y축
		case EGizmoDirection::YZ_Plane:	return {1, 0, 0};  // X축
		default:						return {0, 0, 1};
		}
	}

	// 평면의 두 접선 벡터 반환
	void GetPlaneTangents(FVector& OutTangent1, FVector& OutTangent2) const
	{
		switch (GizmoDirection)
		{
		case EGizmoDirection::XY_Plane:
			OutTangent1 = {1, 0, 0};  // X축
			OutTangent2 = {0, 1, 0};  // Y축
			break;
		case EGizmoDirection::XZ_Plane:
			OutTangent1 = {1, 0, 0};  // X축
			OutTangent2 = {0, 0, 1};  // Z축
			break;
		case EGizmoDirection::YZ_Plane:
			OutTangent1 = {0, 1, 0};  // Y축
			OutTangent2 = {0, 0, 1};  // Z축
			break;
		default:
			OutTangent1 = {1, 0, 0};
			OutTangent2 = {0, 1, 0};
			break;
		}
	}

	// 방향이 평면인지 확인
	bool IsPlaneDirection() const
	{
		return GizmoDirection == EGizmoDirection::XY_Plane ||
		       GizmoDirection == EGizmoDirection::XZ_Plane ||
		       GizmoDirection == EGizmoDirection::YZ_Plane;
	}

	float GetTranslateRadius() const { return TranslateCollisionConfig.Radius * TranslateCollisionConfig.Scale; }
	float GetTranslateHeight() const { return TranslateCollisionConfig.Height * TranslateCollisionConfig.Scale; }
	float GetRotateOuterRadius() const { return RotateCollisionConfig.OuterRadius * RotateCollisionConfig.Scale; }
	float GetRotateInnerRadius() const { return RotateCollisionConfig.InnerRadius * RotateCollisionConfig.Scale; }
	float GetRotateThickness()   const { return std::max(0.001f, RotateCollisionConfig.InnerRadius * RotateCollisionConfig.Scale); }
	USceneComponent* GetTargetComponent() const { return TargetComponent; }
	void SetSelectedComponent(USceneComponent* InComponent) { TargetComponent = InComponent; }
	USceneComponent* GetSelectedComponent() const { return TargetComponent; }
	bool IsInRadius(float Radius) const;
	bool HasComponent() const { return TargetComponent; }
	FVector GetPreviousMouseLocation() const { return PreviousMouseLocation; }
	FVector2 GetPreviousScreenPos() const { return PreviousScreenPos; }
	FVector2 GetDragStartScreenPos() const { return DragStartScreenPos; }
	float GetCurrentRotationAngle() const { return CurrentRotationAngle; }
	float GetCurrentRotationAngleDegrees() const { return FVector::GetRadianToDegree(CurrentRotationAngle); }
	float GetSnappedRotationAngle(float SnapAngleDegrees) const
	{
		const float SnapAngleRadians = FVector::GetDegreeToRadian(SnapAngleDegrees);
		return std::round(CurrentRotationAngle / SnapAngleRadians) * SnapAngleRadians;
	}

	// 스크린 공간 축 방향 벡터 Getter
	FVector2 GetScreenAxisX() const { return ScreenAxisX; }
	FVector2 GetScreenAxisY() const { return ScreenAxisY; }
	FVector2 GetScreenAxisZ() const { return ScreenAxisZ; }
	FVector2 GetScreenOrigin() const { return ScreenOrigin; }
	FViewportClient* GetDragStartViewportClient() const { return DragStartViewportClient; }

	// 마우스 관련
	void EndDrag()
	{
		bIsDragging = false;
		CurrentRotationAngle = 0.0f;
		GizmoDirection = EGizmoDirection::None;
		DragStartViewportClient = nullptr;
	}
	bool IsDragging() const { return bIsDragging; }
	void OnMouseHovering() {}
	void OnMouseDragStart(FViewportClient* InClient, const FVector& CollisionPoint);
	void OnMouseRelease(EGizmoDirection DirectionReleased) {}

private:
	TArray<FEditorPrimitive> Primitives;
	USceneComponent* TargetComponent = nullptr;

	TArray<FVector4> GizmoColor;
	FVector DragStartActorLocation;
	FVector DragStartMouseLocation;
	FVector DragStartActorRotation;
	FQuat DragStartActorRotationQuat;
	FVector DragStartActorScale;

	FGizmoTranslationCollisionConfig TranslateCollisionConfig;
	FGizmoRotateCollisionConfig RotateCollisionConfig;
	bool bIsDragging = false;
	bool bIsWorld = true;	// Gizmo coordinate mode (true; world)

	FRenderState RenderState;

	EGizmoDirection GizmoDirection = EGizmoDirection::None;
	EGizmoMode      GizmoMode = EGizmoMode::Translate;

	// 회전 드래그 상태
	FVector PreviousMouseLocation;
	float CurrentRotationAngle = 0.0f;
	FVector DragStartDirection;

	// 스크린 공간 드래그 상태
	FVector2 DragStartScreenPos;      // 드래그 시작 시 스크린 좌표
	FVector2 PreviousScreenPos;       // 이전 프레임 스크린 좌표
	FViewportClient* DragStartViewportClient = nullptr;  // 드래그를 시작한 뷰포트

	// 스크린 공간 축 방향 벡터 (렌더링 시 계산)
	FVector2 ScreenAxisX = FVector2(0, 0);  // X축(Forward)의 스크린 방향
	FVector2 ScreenAxisY = FVector2(0, 0);  // Y축(Right)의 스크린 방향
	FVector2 ScreenAxisZ = FVector2(0, 0);  // Z축(Up)의 스크린 방향
	FVector2 ScreenOrigin = FVector2(0, 0); // 기즈모 원점의 스크린 좌표

	// Pilot Mode 고정 위치
	bool bUseFixedLocation = false;
	FVector FixedLocation;

	// 렌더 시 하이라이트 색상 계산 (상태 오염 방지)
	FVector4 ColorFor(EGizmoDirection InAxis) const;

	// Modular rendering functions
	void RenderCenterSphere(const FEditorPrimitive& P, float RenderScale);
	void RenderTranslatePlanes(const FEditorPrimitive& P, const FQuat& BaseRot, float RenderScale);
	void RenderScalePlanes(const FEditorPrimitive& P, const FQuat& BaseRot, float RenderScale);
	void RenderRotationCircles(const FEditorPrimitive& P, const FQuat& AxisRotation,
	const FQuat& BaseRot, const FVector4& AxisColor, const FVector& BaseAxis0, const FVector& BaseAxis1, FViewportClient* InClient);
	void RenderRotationQuarterRing(const FEditorPrimitive& P, const FQuat& BaseRot,
	EGizmoDirection Direction, FViewportClient* InClient, const FVector& BaseAxis0, const FVector& BaseAxis1);

	// 스크린 공간 축 방향 벡터 계산
	void CalculateScreenSpaceAxisDirections(const FViewportClient* InClient, const D3D11_VIEWPORT& InViewport,
		const FVector& GizmoLocation, const FQuat& BaseRot, float RenderScale);
};
