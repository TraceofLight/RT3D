#include "pch.h"
#include "Actor/Public/Shooter.h"

FShooter::FShooter()
{
	Init();
}

FShooter::~FShooter()
{
	Destroy();
}

void FShooter::Init()
{
	ChargingTime = 0.0f;
	Shape = new URectangle();
}

void FShooter::Destroy()
{
	SafeDelete(Shape);
}

void FShooter::Charging()
{
	// SpaceBar로 누르는 시간만큼 Charging
}

void FShooter::Shoot()
{
	// 생성되는 볼에 속도를 전달해야 함
}
