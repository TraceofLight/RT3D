#include "pch.h"
#include "Actor/Public/PinBall.h"

#include "Mesh/Public/UBall.h"

UPinBall* GravityCenterBall = nullptr;

UPinBall::UPinBall()
{
	Init();
}

UPinBall::~UPinBall()
{
	Destroy();
}

void UPinBall::Init()
{
	Shape = new UBall(0.03f);
	Location = FVector3(0.0f, 0.0f, 0.0f);
	Velocity = FVector3(0.0f, 0.0f, 0.0f);
	bPinballGravity = true; // 기본 중력 활성화
}

void UPinBall::Destroy()
{
	SafeDelete(Shape);
}

void UPinBall::Move()
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
		float MinDistance = (GravityCenterBall->GetShape()->GetRadius() + this->GetShape()->GetRadius());
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
		Velocity.y -= GRAVITY_ACCELERATION * DT; // DT 기반 중력 적용
	}

	Location += Velocity * DT;

	if ((Location.x > 1.0f - Shape->GetRadius() && Velocity.x > 0) || (Location.x < -1.0f + Shape->GetRadius() &&
		Velocity.x < 0))
	{
		Velocity.x *= -1.0f; // 반사

		// 벽 안쪽으로 위치 보정
		Location.x = min(Location.x, 1.0f - Shape->GetRadius());
		Location.x = max(Location.x, -1.0f + Shape->GetRadius());
	}

	// Y축 벽 충돌
	if ((Location.y > 1.0f - Shape->GetRadius() && Velocity.y > 0) /* || (Location.y < -1.0f + Shape->GetRadius() &&
		Velocity.y < 0*/)
	{
		Velocity.y *= -1.0f; // 반사

		// 벽 안쪽으로 위치 보정
		Location.y = min(Location.y, 1.0f - Shape->GetRadius());
		Location.y = max(Location.y, -1.0f + Shape->GetRadius());
	}
}
