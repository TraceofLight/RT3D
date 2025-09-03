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
	OutputDebugStringA(("[SHOOTER] Charging Time: " + to_string(ChargingTime) + "\n").c_str());
}

void UShooter::Shoot()
{
	// 생성되는 볼에 속도를 전달해야 함
	float ShotPower = ChargingTime * 2.0f; // 차징 시간에 비례한 파워
	OutputDebugStringA(("[SHOOTER] Shot Power: " + to_string(ShotPower) + "\n").c_str());

	// UBall 대신 PinBall 사용
	UPinBall* NewBall = new UPinBall();

	// Shooter 위치에서 약간 위쪽에 공 생성
	NewBall->GetLocation() = Location + FVector3(0.0f, 0.1f, 0.0f);

	float LaunchSpeed = ShotPower * 5.0f;
	NewBall->GetVelocity() = FVector3(0.0f, LaunchSpeed, 0.0f);

	OutputDebugStringA(
		("[SHOOTER] Ball created at (" + to_string(NewBall->GetLocation().x) + ", " +
			to_string(NewBall->GetLocation().y) + ") with velocity (" + to_string(NewBall->GetVelocity().x) + ", " +
			to_string(NewBall->GetVelocity().y) + ")\n").c_str());

	OutputDebugStringA("[SHOOTER] Ball creation attempted\n");

	// 차징 시간 초기화
	ChargingTime = 0.0f;

	FSceneManager& SceneManager = FSceneManager::GetInstance();
	SceneManager.AddPrimitiveToScene(NewBall);
}
