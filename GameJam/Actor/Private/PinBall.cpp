#include "pch.h"
#include "Actor/Public/PinBall.h"

#include "Mesh/Public/UBall.h"

UPinBall::UPinBall()
{
	Init();
}

UPinBall::~UPinBall()
{
	Destroy();
}

void UPinBall::Init()
{
	Shape = new UBall();
}

void UPinBall::Destroy()
{
	SafeDelete(Shape);
}
