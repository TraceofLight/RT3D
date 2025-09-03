#include "pch.h"
#include "Mesh/Public/UBall.h"

UBall::UBall()
	: Radius(0.1f)
{
	Mass = Radius * Radius;
}

UBall::UBall(float InRadius)
	: Radius(InRadius)
{
	Mass = Radius * Radius;
}

UBall::~UBall() = default;
