#pragma once
#include "Core/Public/Primitive.h"

class UBall :
	public UPrimitive
{
private:
	float Radius;
	float Mass;
	FVector3 Location;
	FVector3 Velocity;
	float Restitution = 1.0f;

public:
	float GetRadius() const { return Radius; }
	float GetMass() const { return Mass; }
	const FVector3& GetLocation() const { return Location; }
	const FVector3& GetVelocity() const { return Velocity; }

	void SetRadius(float InRadius) { Radius = InRadius; Mass = Radius * Radius; }
	void SetLocation(const FVector3& InLocation) { Location = InLocation; }
	void SetVelocity(const FVector3& InVelocity) { Velocity = InVelocity; }
	float GetRestitution() const { return Restitution; }
	void SetRestitution(float InRestitution) { Restitution = InRestitution; }

	UBall();
	UBall(float InRadius);
	~UBall() override;
};
