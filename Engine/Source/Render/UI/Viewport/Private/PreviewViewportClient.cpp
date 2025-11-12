#include "pch.h"
#include "Render/UI/Viewport/Public/PreviewViewportClient.h"
#include "Editor/Public/Gizmo.h"
#include "Render/UI/Window/Public/PreviewScene.h"
#include "Editor/Public/ObjectPicker.h"
#include "Component/Mesh/Public/SkeletalMeshComponent.h"
#include "Render/UI/Viewport/Public/Viewport.h"

FPreviewViewportClient::FPreviewViewportClient()
{
	// Gizmo 인스턴스 생성
	Gizmo = NewObject<UGizmo>();
}

FPreviewViewportClient::~FPreviewViewportClient()
{
	SafeDelete(Gizmo);
}

bool FPreviewViewportClient::InputKey(EKeyInput Key, bool bPressed)
{
	if (!Gizmo || !bPressed)
	{
		return false;
	}

	// W/E/R: 기즈모 모드 전환
	switch (Key)
	{
	case EKeyInput::W:
		Gizmo->SetGizmoMode(EGizmoMode::Translate);
		return true;
	case EKeyInput::E:
		Gizmo->SetGizmoMode(EGizmoMode::Rotate);
		return true;
	case EKeyInput::R:
		Gizmo->SetGizmoMode(EGizmoMode::Scale);
		return true;
	case EKeyInput::Space:
		Gizmo->ChangeGizmoMode();
		return true;
	case EKeyInput::MouseLeft:
		// 마우스 왼쪽 버튼 Release
		if (!bPressed)
		{
			Gizmo->EndDrag();
			bIsDragging = false;
		}
		return true;
	default:
		break;
	}

	return false;
}

bool FPreviewViewportClient::HandleClick(int32 MouseX, int32 MouseY)
{
	if (!Gizmo || !GetOwningViewport())
	{
		return false;
	}

	// HitProxy를 통한 기즈모/오브젝트 피킹
	// TODO: ObjectPicker 통합 또는 별도 Preview용 피킹 로직 구현
	// 현재는 기즈모 클릭 판정만 구현

	// 기즈모 드래그 시작
	if (Gizmo->GetGizmoDirection() != EGizmoDirection::None)
	{
		// 월드 레이 계산
		const D3D11_VIEWPORT& ViewportInfo = GetOwningViewport()->GetRenderRect();
		const float NdcX = (static_cast<float>(MouseX) / ViewportInfo.Width) * 2.0f - 1.0f;
		const float NdcY = -(static_cast<float>(MouseY) / ViewportInfo.Height) * 2.0f - 1.0f;

		FMatrix ViewMatrix = GetViewMatrix();
		FMatrix ProjectionMatrix = GetProjectionMatrix(ViewportInfo.Width / ViewportInfo.Height);
		FMatrix ViewProjectionMatrixInv = (ViewMatrix * ProjectionMatrix).Inverse();

		FVector4 NearPoint = FVector4(NdcX, NdcY, 0.0f, 1.0f);
		FVector4 FarPoint = FVector4(NdcX, NdcY, 1.0f, 1.0f);

		FVector4 WorldNear4 = FMatrix::VectorMultiply(NearPoint, ViewProjectionMatrixInv);
		FVector4 WorldFar4 = FMatrix::VectorMultiply(FarPoint, ViewProjectionMatrixInv);

		FVector WorldNear = FVector(WorldNear4.X / WorldNear4.W, WorldNear4.Y / WorldNear4.W, WorldNear4.Z / WorldNear4.W);
		FVector WorldFar = FVector(WorldFar4.X / WorldFar4.W, WorldFar4.Y / WorldFar4.W, WorldFar4.Z / WorldFar4.W);

		FVector Direction = (WorldFar - WorldNear);
		Direction.Normalize();

		// 충돌 판정 (간단한 거리 기반)
		const FVector ViewLocation = GetViewLocation();
		DragStartMouseLocation = WorldNear;

		Gizmo->OnMouseDragStart(this, DragStartMouseLocation);
		bIsDragging = true;
		return true;
	}

	return false;
}

bool FPreviewViewportClient::ProcessGizmoDrag(const FVector2& MouseDelta)
{
	if (!Gizmo || !bIsDragging || !Gizmo->IsDragging())
	{
		return false;
	}

	// 기즈모 드래그 처리
	// TODO: Editor의 GetGizmoDragLocation/Rotation/Scale 로직 통합

	return true;
}
