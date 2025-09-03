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
	if (!Shape || !InInput || InDeltaTime <= 0.f)
	{
		return;
	}

	float Current = Shape->Rotation;

	// 키 누르는 동안 위로(=MaxRotation 방향) 회전
	if (RotationKey != EKeyInput::End && InInput->IsKeyDown(RotationKey))
	{
		Current += RotationSpeed * InDeltaTime;
		if (bUseRotationLimit && Current > MaxRotation)
		{
			Current = MaxRotation;
		}
	}
	else
	{
		// 키를 안 누르면 아래(MinRotation)로 복귀
		Current -= RotationSpeed * InDeltaTime; // ReturnSpeed 사용 시 교체
		if (bUseRotationLimit && Current < MinRotation)
		{
			Current = MinRotation;
		}
	}

	Shape->Rotation = Current;
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
