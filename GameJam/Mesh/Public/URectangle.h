#pragma once
#include "Core/Public/Primitive.h"

class URectangle :
	public UPrimitive
{
public:
	FVector3 Location;
	float Rotation;
	FVector3 Velocity;
	float Width;
	float Height;
	float Mass;

	URectangle();

	void Move();
};

extern URectangle GRectangle;
