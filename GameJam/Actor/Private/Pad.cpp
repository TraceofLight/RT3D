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
		return;

	float current = Shape->Rotation;
	const bool keyDown = (RotationKey != EKeyInput::End) && InInput->IsKeyDown(RotationKey);

	if (Side == ESide::Left)
	{
		// 키 누르면 증가, 아니면 복귀(감소)
		if (keyDown)
			current += RotationSpeed * InDeltaTime;
		else
			current -= RotationSpeed * InDeltaTime;
	}
	else // Right
	{
		// 키 누르면 감소(=MinRotation 방향), 아니면 복귀(증가)
		if (keyDown)
			current -= RotationSpeed * InDeltaTime;
		else
			current += RotationSpeed * InDeltaTime;
	}

	if (bUseRotationLimit)
		current = std::clamp(current, MinRotation, MaxRotation);

	Shape->Rotation = current;
}

void UPad::SetRotationKey(EKeyInput InKey)
{
	RotationKey = InKey;
}

UTriangle* UPad::GetShape() const
{
	return Shape;
}

static void RecalcIsoscelesInRadius(UTriangle* InTriangle)
{
	// 이등변 내접원 반지름: r = (B * H) / ( B + 2 * sqrt( (B/2)^2 + H^2 ) )
	float halfB = InTriangle->Base * 0.5f;
	float side = sqrtf(halfB * halfB + InTriangle->Height * InTriangle->Height);
	InTriangle->Radius = (InTriangle->Base * InTriangle->Height) / (InTriangle->Base + 2.f * side);
}

void UPad::ConfigueLeftPad()
{
	if (!Shape)
	{
		Shape = new UTriangle();
	}

	Side = ESide::Left;

	// 공통 형상
	Shape->Base = 0.1f;
	Shape->Height = 0.5f;
	RecalcIsoscelesInRadius(Shape);

	// 위치 (좌측) – 인센터 기준
	Shape->Location = FVector3(-0.45f - 0.15f, -0.45f, 0.0f);

	// 회전 제한 (위로 올릴 때 +방향)
	MinRotation = DegToRad(-120.0f);
	MaxRotation = DegToRad(-60.0f);
	Shape->Rotation = MinRotation;
	
	// 속도
	RotationSpeed = DegToRad(360.0f); // 필요 시 조정 (deg/sec)

	bUseRotationLimit = true;
	RotationKey = EKeyInput::A;
}

void UPad::ConfigueRightPad()
{
	if (!Shape)
	{
		Shape = new UTriangle();
	}

	Side = ESide::Right;

	Shape->Base = 0.1f;
	Shape->Height = 0.5f;
	RecalcIsoscelesInRadius(Shape);

	// 위치
	Shape->Location = FVector3(0.45f - 0.15f, -0.45f, 0.0f);

	// 회전 제한 (위로 올릴 때 -방향)
	MinRotation = DegToRad(60.0f);
	MaxRotation = DegToRad(120.0f);
	Shape->Rotation = MaxRotation;

	RotationSpeed = DegToRad(360.0f);

	bUseRotationLimit = true;
	RotationKey = EKeyInput::D;
}

UPad GLeftPad = UPad();
UPad GRightPad = UPad();
