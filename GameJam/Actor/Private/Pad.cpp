#include "pch.h"
#include "Actor/Public/Pad.h"
#include "Mesh/Public/UTriangle.h"
#include "Render/Public/Renderer.h"
#include "Manager/Public/InputManager.h"

UPad::UPad()
{
	Init();
}

UPad::~UPad()
{
	Destroy();
}

void UPad::Init()
{
	Shape = new UTriangle();
	Shape->Rotation = MinRotation; // 초기 위치를 최소 회전 각도로 설정
}

void UPad::Destroy()
{
	SafeDelete(Shape);
}

void UPad::Render(const URenderer& InRenderer)
{
	InRenderer.UpdateConstantForTriangle(Shape->Location, Shape->Base, Shape->Height, Shape->Rotation, Shape->Radius);
	InRenderer.RenderTriangle();
}

void UPad::HandleInput(FInputManager* InInput, float InDeltaTime)
{
	if (!InInput || InDeltaTime <= 0.0f)
	{
		return;
	}

	float Direction = 0.0f;

	// 반시계 방향
	if (InInput->IsKeyDown(RotationKey))
	{
		Direction += 1.0f;
	}

	if (Direction == 0.0f)
	{
		return;
	}

	float NewRotation = Shape->Rotation + Direction * RotationSpeed * InDeltaTime;

	if (bUseRotationLimit)
	{
		// 제한 모드: Clamp
		Shape->Rotation = std::clamp(NewRotation, MinRotation, MaxRotation);
	}
	else
	{
		// 무제한 모드: 0~2π 래핑
		const float TwoPi = 6.28318530717958647692f;
		NewRotation = std::fmod(NewRotation, TwoPi);
		if (NewRotation < 0.0f)
		{
			NewRotation += TwoPi;
		}
		Shape->Rotation = NewRotation;
	}
}

void UPad::SetRotationKey(EKeyInput InKey)
{
	RotationKey = InKey;
}

UTriangle* UPad::GetShape() const
{
	return Shape;
}

UPad GLeftPad = UPad();
