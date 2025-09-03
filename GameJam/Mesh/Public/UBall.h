#pragma once
#include "Core/Public/Primitive.h"

class UBall :
	public UPrimitive
{
private:
	float Radius;
	float Mass;

public:
	float GetRadius() const { return Radius; }
	float GetMass() const { return Mass; }

	UBall();
	UBall(float InRadius);
	~UBall() override;
};
