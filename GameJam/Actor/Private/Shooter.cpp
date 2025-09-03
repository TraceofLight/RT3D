#include "pch.h"
#include "Actor/Public/Shooter.h"

#include "Mesh/Public/UBall.h"

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

	UBall* NewBall = new UBall();

	// Shooter 위치에서 약간 위쪽에 공 생성
	NewBall->Location = Location + FVector3(0.0f, 0.1f, 0.0f);

	float LaunchSpeed = ShotPower * 5.0f;
	NewBall->Velocity = FVector3(0.0f, LaunchSpeed, 0.0f);

	OutputDebugStringA(
		("[SHOOTER] Ball created at (" + std::to_string(NewBall->Location.x) + ", " +
			std::to_string(NewBall->Location.y) + ") with velocity (" + std::to_string(NewBall->Velocity.x) + ", " +
			std::to_string(NewBall->Velocity.y) + ")\n").c_str());

	// 전역 공 리스트에 추가
	extern UPrimitive** PrimitiveList;
	extern int TotalPrimitives;

	// 새로운 리스트 생성
	UPrimitive** NewList = new UPrimitive*[TotalPrimitives + 1];

	// 기존 공들 복사
	for (int i = 0; i < TotalPrimitives; ++i)
	{
		NewList[i] = PrimitiveList[i];
	}

	// 새 공 추가
	NewList[TotalPrimitives] = NewBall;

	// 기존 리스트 해제
	if (PrimitiveList != nullptr)
	{
		delete[] PrimitiveList;
	}

	// 리스트 교체
	PrimitiveList = NewList;
	++TotalPrimitives;
	++UBall::TotalNumBalls; // UBall 총 개수도 증가

	OutputDebugStringA(
		("[SHOOTER] Ball added to list! Total balls: " + std::to_string(TotalPrimitives) + "\n").c_str());

	// 차징 시간 초기화
	ChargingTime = 0.0f;
}
