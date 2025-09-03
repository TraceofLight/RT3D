#include "pch.h"
#include "Mesh/Public/UTriangle.h"
#include "Manager/Public/InputManager.h"

// 임시 테스트용 삼각형
UTriangle GTriangle = UTriangle();

UTriangle::UTriangle()
{
	// Position Setting
	Location.x = 0.0f;
	Location.y = 0.0f;
	Location.z = 0.0f;
	// Velocity Setting
	Velocity.x = 0.0f;
	Velocity.y = 0.0f;
	Velocity.z = 0.0f;
	// Size Setting
	Base = 0.1f;
	Height = 1.0f;
	Radius = (Base * Height) / (Base + Height + sqrtf(Base * Base + Height * Height));
	// Rotation Setting
	Rotation = 0;
	RotationSpeed = 3.14159265f; // 180 deg/sec
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

	Rotation += Direction * RotationSpeed * InDeltaTime;

	// 0 ~ 2π 래핑
	const float TwoPi = 6.28318530717958647692f;
	Rotation = std::fmod(Rotation, TwoPi);
	if (Rotation < 0.0f)
	{
		Rotation += TwoPi;
	}
}
