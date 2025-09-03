#include "pch.h"
#include "Mesh/Public/UTriangle.h"
#include "Manager/Public/InputManager.h"

UTriangle::UTriangle()
{
	Location = { 0.f, 0.f, 0.f };
	Base = 0.1f;
	Height = 1.0f;

	// 이등변 삼각형 Inradius
	float halfB = Base * 0.5f;
	float side = sqrtf(halfB * halfB + Height * Height);
	Radius = (Base * Height) / (Base + 2.0f * side);

	Rotation = DegToRad(-120.0f);
	RotationSpeed = 3.14159265f;

	bUseRotationLimit = true;
	MinRotation = DegToRad(-120.0f);
	MaxRotation = DegToRad(-60.0f);
}

void UTriangle::UpdateRotation(FInputManager* InInput, float InDeltaTime)
{
	if (!InInput || InDeltaTime <= 0.0f)
	{
		return;
	}

	float Direction = 0.0f;

	// 반시계 방향
	if (InInput->IsKeyDown(EKeyInput::A) || InInput->IsKeyDown(EKeyInput::Left))
	{
		Direction += 1.0f;
	}
	// 시계 방향
	if (InInput->IsKeyDown(EKeyInput::D) || InInput->IsKeyDown(EKeyInput::Right))
	{
		Direction -= 1.0f;
	}

	if (Direction == 0.0f)
	{
		return;
	}

	float NewRotation = Rotation + Direction * RotationSpeed * InDeltaTime;

	if (bUseRotationLimit)
	{
		// 제한 모드: Clamp
		Rotation = std::clamp(NewRotation, MinRotation, MaxRotation);
	}
	else
	{
		// 무제한 모드: 0~2π 래핑
		const float TwoPi = 6.28318530717958647692f;
		NewRotation = std::fmod(NewRotation, TwoPi);
		if (NewRotation < 0.0f)
			NewRotation += TwoPi;
		Rotation = NewRotation;
	}
}

void UTriangle::SetRotationLimit(float InMinRadians, float InMaxRadians)
{
	if (InMinRadians > InMaxRadians)
	{
		std::swap(InMinRadians, InMaxRadians);
	}
	MinRotation = InMinRadians;
	MaxRotation = InMaxRadians;
	bUseRotationLimit = true;

	// 현재 회전 값도 즉시 제한
	Rotation = std::clamp(Rotation, MinRotation, MaxRotation);
}

void UTriangle::ClearRotationLimit()
{
	bUseRotationLimit = false;
}
