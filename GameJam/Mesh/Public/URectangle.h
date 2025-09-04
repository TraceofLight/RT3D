#pragma once
#include "Core/Public/Primitive.h"

class URectangle :
	public UPrimitive
{
public:
	FVector3 Location;
	FVector3 Velocity;
	float Width;
	float Height;
	float Mass;
	float Rotation;
	bool AutoRotation = false;
	float AutoRotRate;
	bool clockwise = false;

	URectangle();
	void Move();
	void Update();

	// TODO(KHJ): 멤버 은닉
	float GetWidth() const { return Width; }
	float GetHeight() const { return Height; }
};
