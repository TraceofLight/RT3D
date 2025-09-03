#include "pch.h"
#include "Mesh/Public/URectangle.h"

URectangle::URectangle()
{
	// Position Setting
	Location.x = 0.0f;
	Location.y = 0.0f;
	Location.z = 0.0f;
	// Velocity Setting
	Velocity.x = (-0.2f + (rand() / (float)RAND_MAX) * 0.4f);
	Velocity.y = (-0.2f + (rand() / (float)RAND_MAX) * 0.4f);
	Velocity.z = 0.0f;
	// Size Setting
	Width = 0.1f + (rand() / (float)RAND_MAX) * 0.1f;
	Height = 0.1f + (rand() / (float)RAND_MAX) * 0.1f;
	// Mass Setting
	Mass = Width * Height;
}

void URectangle::Move()
{
	// === DeltaTime 기반 위치 업데이트 ===
	Location += Velocity * DT; // 프레임레이트에 무관한 움직임

	// === 벽 충돌 처리 ===
	// X축 벽 충돌
	if ((Location.x > 1.0f - Width / 2 && Velocity.x > 0) || (Location.x < -1.0f + Width / 2 && Velocity.x < 0))
	{
		Velocity.x *= -1.0f; // 반사

		// 벽 안쪽으로 위치 보정
		if (Location.x > 1.0f - Width / 2) Location.x = 1.0f - Width / 2;
		if (Location.x < -1.0f + Width / 2) Location.x = -1.0f + Width / 2;
	}

	// Y축 벽 충돌
	if ((Location.y > 1.0f - Height / 2 && Velocity.y > 0) || (Location.y < -1.0f + Height / 2 && Velocity.y < 0))
	{
		Velocity.y *= -1.0f; // 반사

		// 벽 안쪽으로 위치 보정
		if (Location.y > 1.0f - Height / 2) Location.y = 1.0f - Height / 2;
		if (Location.y < -1.0f + Height / 2) Location.y = -1.0f + Height / 2;
	}
}

URectangle GRectangle = URectangle();
