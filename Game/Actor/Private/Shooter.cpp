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

	// 발사 상태 초기화
	bHasBallLoaded = true;
	CurrentBall = nullptr;
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

UPinBall* UShooter::Shoot()
{
	// 발사 가능성 검사
	if (!bHasBallLoaded)
	{
		DEBUG_PRINT("[SHOOTER] Cannot shoot - no ball loaded!\n");
		return nullptr;
	}

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

	// 현재 공 참조 저장 및 발사 상태 변경
	CurrentBall = NewBall;
	bHasBallLoaded = false;  // 더 이상 발사 불가

	DEBUG_PRINT("[SHOOTER] Ball fired - shooter now disabled\n");

	// 차징 시간 초기화
	ChargingTime = 0.0f;

	FSceneManager& SceneManager = FSceneManager::GetInstance();
	SceneManager.AddPrimitiveToScene(NewBall);

	return NewBall;
}

void UShooter::LoadBall()
{
	if (bHasBallLoaded)
	{
		DEBUG_PRINT("[SHOOTER] Ball already loaded!\n");
		return;
	}

	bHasBallLoaded = true;
	CurrentBall = nullptr;
	DEBUG_PRINT("[SHOOTER] New ball loaded - shooter ready to fire!\n");
}

void UShooter::OnBallTrigger()
{
	// 공이 트리거 영역에 도달했을 때 호출
	// 예: 화면 하단, 특정 영역, 또는 공이 사라졌을 때
	DEBUG_PRINT("[SHOOTER] Ball trigger activated - reloading shooter\n");
	LoadBall();
}
