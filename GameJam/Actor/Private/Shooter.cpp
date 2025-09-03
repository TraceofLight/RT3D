#include "pch.h"
#include "Actor/Public/Shooter.h"

UShooter::UShooter()
{
	Init();
}

UShooter::~UShooter()
{
	Destroy();
}

void UShooter::Init()
{
	ChargingTime = 0.0f;
	Shape = new URectangle();
}

void UShooter::Destroy()
{
	SafeDelete(Shape);
}

void UShooter::Charging()
{
	// SpaceBar로 누르는 시간만큼 Charging
}

void UShooter::Shoot()
{
	// 생성되는 볼에 속도를 전달해야 함
}
