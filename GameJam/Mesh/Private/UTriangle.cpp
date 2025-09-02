#include "pch.h"
#include "Mesh/Public/UTriangle.h"

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
	// Rotation Setting
	Rotation = 0;
}
