#include "pch.h"
#include "Actor/Public/URectangle.h"

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
	const float FixedDeltaTime = 1.0f / 30.0f;
	Location += Velocity * FixedDeltaTime;
	// 벽 충돌 처리
	if ((Location.x > 1.0f - Width / 2 && Velocity.x > 0) || (Location.x < -1.0f + Width / 2 &&
		Velocity.x < 0))
	{
		Velocity.x *= -1.0f;
	}
	if ((Location.y > 1.0f - Height / 2 && Velocity.y > 0) || (Location.y < -1.0f + Height / 2 &&
		Velocity.y < 0))
	{
		Velocity.y *= -1.0f;
	}
}

URectangle GRectangle = URectangle();
