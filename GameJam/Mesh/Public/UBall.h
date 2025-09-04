#pragma once
#include "Core/Public/Primitive.h"

class UBall :
	public UPrimitive
{
private:
	float Radius;
	float Mass;
	float Restitution = 1.0f;

public:
	float GetRadius() const { return Radius; }
	float GetMass() const { return Mass; }
	float GetRestitution() const { return Restitution; }
	void SetRestitution(float InRestitution) { Restitution = InRestitution; }

	UBall();
	UBall(float InRadius);
	~UBall() override;
};
