#include "pch.h"
#include "Actor/Public/UBall.h"

// Static variable definitions
int UBall::TotalNumBalls = 0;

// Global variable definitions (moved from static to extern)
UBall* GravityCenterBall = nullptr;
bool bPinballGravity = true;

UBall::UBall()
{
	// Position Setting
	Location.x = -0.8f + (rand() / static_cast<float>(RAND_MAX)) * 1.6f;
	Location.y = -0.8f + (rand() / static_cast<float>(RAND_MAX)) * 1.6f;
	Location.z = 0.0f;

	// Velocity Setting
	Velocity.x = (-0.2f + (rand() / static_cast<float>(RAND_MAX)) * 0.4f);
	Velocity.y = (-0.2f + (rand() / static_cast<float>(RAND_MAX)) * 0.4f);
	Velocity.z = 0.0f;

	// Radius Setting
	Radius = 0.1f + (rand() / static_cast<float>(RAND_MAX)) * 0.05f;

	// 질량은 면적에 비례하도록 설정
	Mass = Radius * Radius;

	// Increase Total Count
	// ++TotalNumBalls;
}

// DT 기반으로 업데이트된 Move 함수
void UBall::Move()
{
	// 중력 중심으로 설정된 공은 움직이지 않음
	if (GravityCenterBall == this)
	{
		Velocity = FVector3(0.f, 0.f, 0.f);
		return;
	}

	// === 중력 처리 (DeltaTime 기반) ===
	if (GravityCenterBall != nullptr)
	{
		// 중력 중심이 있을 때
		FVector3 Direction = GravityCenterBall->Location - this->Location;
		float DistanceSq = Direction.LengthSquare();

		// 충돌 거리보다 멀리 떨어져 있을 때만 중력 적용
		float MinDistance = (GravityCenterBall->Radius + this->Radius);
		if (DistanceSq > MinDistance * MinDistance)
		{
			Direction.Normalize();

			// 중력 강도 (초당 일정한 힘으로 조정)
			float GravityStrength = 2.0f;
			FVector3 GravityAcceleration = Direction * GravityStrength * DT / (DistanceSq + 0.1f);

			Velocity += GravityAcceleration; // DT 기반 중력 적용
		}
	}
	else if (bPinballGravity)
	{
		// 기본 핀볼 중력 (아래쪽으로)
		float GravityAcceleration = 1.0f; // 초당 1.0 단위의 가속도
		Velocity.y -= GravityAcceleration * DT; // DT 기반 중력 적용
	}

	// === 위치 업데이트 (DeltaTime 기반) ===
	Location += Velocity * DT; // 프레임레이트에 무관한 움직임

	// === 벽 충돌 처리 ===
	// X축 벽 충돌
	if ((Location.x > 1.0f - Radius && Velocity.x > 0) || (Location.x < -1.0f + Radius && Velocity.x < 0))
	{
		Velocity.x *= -1.0f; // 반사

		// 벽 안쪽으로 위치 보정
		if (Location.x > 1.0f - Radius) Location.x = 1.0f - Radius;
		if (Location.x < -1.0f + Radius) Location.x = -1.0f + Radius;
	}

	// Y축 벽 충돌
	if ((Location.y > 1.0f - Radius && Velocity.y > 0) || (Location.y < -1.0f + Radius && Velocity.y < 0))
	{
		Velocity.y *= -1.0f; // 반사

		// 벽 안쪽으로 위치 보정
		if (Location.y > 1.0f - Radius) Location.y = 1.0f - Radius;
		if (Location.y < -1.0f + Radius) Location.y = -1.0f + Radius;
	}
}
