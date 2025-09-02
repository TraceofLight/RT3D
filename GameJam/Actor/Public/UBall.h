#pragma once
#include "Core/Public/Primitive.h"

class UBall :
    public UPrimitive
{
public:
    FVector3 Location; // [Fixed]
    FVector3 Velocity; // [Fixed]
    float Radius; // [Fixed]
    float Mass; // [Fixed]
    static int TotalNumBalls; // [Fixed]

	UBall();

	// TODO(KHJ): DT
	void Move();
};
