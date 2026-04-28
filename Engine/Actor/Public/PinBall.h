#pragma once
#include "Core/Public/Primitive.h"

class UBall;

class UPinBall :
	public UPrimitive
{
private:
	UBall* Shape;
	FVector3 Location;
	FVector3 Velocity;

	bool bPinballGravity;

public:
	void Init();
	void Destroy();
	void Move();

	UBall* GetShape() const { return Shape; }
	FVector3 GetLocation() const { return Location; }
	FVector3 GetVelocity() const { return Velocity; }
	FVector3& GetLocation() { return Location; }
	FVector3& GetVelocity() { return Velocity; }

	// Special Member Function
	UPinBall();
	~UPinBall() override;
};
