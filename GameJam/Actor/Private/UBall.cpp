#include "pch.h"
#include "Actor/Public/UBall.h"
#include "Actor/Public/URectangle.h"

// Static variable definitions
int UBall::TotalNumBalls = 0;

// Global variable definitions (moved from static to extern)
UBall* GravityCenterBall = nullptr;
bool bPinballGravity = true;

UBall::UBall()
{
	// Position Setting
	Location.x = -0.8f + (rand() / (float)RAND_MAX) * 1.6f;
	Location.y = -0.8f + (rand() / (float)RAND_MAX) * 1.6f;
	Location.z = 0.0f;

	// Velocity Setting
	Velocity.x = (-0.2f + (rand() / (float)RAND_MAX) * 0.4f);
	Velocity.y = (-0.2f + (rand() / (float)RAND_MAX) * 0.4f);
	Velocity.z = 0.0f;

	// Radius Setting
	Radius = 0.1f + (rand() / (float)RAND_MAX) * 0.05f;

	// 질량은 면적에 비례하도록 설정
	Mass = Radius * Radius;

	// Increase Total Count
	// ++TotalNumBalls;
}

// TODO(KHJ): DT
void UBall::Move()
{
	const float FixedDeltaTime = 1.0f / 30.0f;

	// Make Gravity Center Stop
	if (GravityCenterBall == this)
	{
		Velocity = FVector3(0.f, 0.f, 0.f);
		return;
	}

	// 중력 중심 존재 여부에 따라, 중력 적용
	if (GravityCenterBall != nullptr)
	{
		FVector3 Direction = GravityCenterBall->Location - this->Location;
		float DistanceSq = Direction.LengthSquare();

		// 힘의 최대값 조정
		if (DistanceSq > (GravityCenterBall->Radius + this->Radius) * (GravityCenterBall->Radius
			+ this->Radius))
		{
			Direction.Normalize();
			float GravityStrength = 2.0f;
			Velocity += Direction * GravityStrength * FixedDeltaTime / (DistanceSq + 0.1f);
		}
	}
	else if (bPinballGravity) // 기존 핀볼 중력
	{
		Velocity.y -= (1.0f * FixedDeltaTime);
	}

	Location += Velocity * FixedDeltaTime;

	// 벽 충돌 처리
	if ((Location.x > 1.0f - Radius && Velocity.x > 0) || (Location.x < -1.0f + Radius &&
		Velocity.x < 0))
	{
		Velocity.x *= -1.0f;
	}
	if ((Location.y > 1.0f - Radius && Velocity.y > 0) || (Location.y < -1.0f + Radius &&
		Velocity.y < 0))
	{
		Velocity.y *= -1.0f;
	}
}
