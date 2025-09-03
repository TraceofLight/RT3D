#include "pch.h"
#include "Actor/Public/Shooter.h"

#include "Actor/Public/PinBall.h"
#include "Manager/Public/SceneManager.h"

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
	ChargingTime += DT;

	ChargingTime = min(ChargingTime, 3.0f);
	DEBUG_PRINT_FORMAT("[SHOOTER] Charging Time: %.2f\n", ChargingTime);
}

void UShooter::Shoot()
{
	// 생성되는 볼에 속도를 전달해야 함
	float ShotPower = ChargingTime * 2.0f; // 차징 시간에 비례한 파워
	DEBUG_PRINT_FORMAT("[SHOOTER] Shot Power: %.2f\n", ShotPower);

	// UBall 대신 PinBall 사용
	UPinBall* NewBall = new UPinBall();

	// Shooter 위치에서 약간 위쪽에 공 생성
	NewBall->GetLocation() = Location + FVector3(0.0f, 0.1f, 0.0f);

	float LaunchSpeed = ShotPower * 5.0f;
	NewBall->GetVelocity() = FVector3(0.0f, LaunchSpeed, 0.0f);

	DEBUG_PRINT_FORMAT("[SHOOTER] Ball created at (%.2f, %.2f) with velocity (%.2f, %.2f)\n",
		NewBall->GetLocation().x, NewBall->GetLocation().y,
		NewBall->GetVelocity().x, NewBall->GetVelocity().y);

	DEBUG_PRINT("[SHOOTER] Ball creation attempted\n");

	// 차징 시간 초기화
	ChargingTime = 0.0f;

	FSceneManager& SceneManager = FSceneManager::GetInstance();
	SceneManager.AddPrimitiveToScene(NewBall);
}
