#include "pch.h"
#include "Scene/Public/GameScene.h"
#include "Manager/Public/InputManager.h"
#include "Manager/Public/TimeManager.h"
#include "Manager/Public/SceneManager.h"
#include "Render/Public/Renderer.h"
#include "Actor/Public/PinBall.h"
#include "Mesh/Public/UBall.h"

// 생성자: 멤버 변수 초기화
GameScene::GameScene()
	: Scene("GAME SCENE")
{
	Init();
}

// 소멸자: 동적 할당된 메모리 해제
GameScene::~GameScene()
{
	Cleanup();
}

void GameScene::Init()
{
	// 씬이 시작될 때 필요한 초기화 로직
	// 예를 들어, 처음에 공을 몇 개 생성하고 싶다면 여기서 AddNewBall() 호출
}

void GameScene::Cleanup()
{
	for (UPinBall* Ball : PinBalls)
	{
		delete Ball;
	}

	PinBalls.clear();
}

void GameScene::Update(float deltaTime)
{
	// ================== MainLoop에서 가져온 게임 로직 ==================
	FTimeManager* TimeManager = FTimeManager::GetInstance();
	FInputManager* KeyManager = FInputManager::GetInstance();
	FSceneManager& SceneManager = FSceneManager::GetInstance();

	ScenePrimitives = SceneManager.GetAllScenePrimivites();

	// 공들의 물리 시뮬레이션 업데이트
	for (UPinBall* Ball : PinBalls)
	{
	// 	// PinBall의 물리 업데이트 처리
	// 	Ball->Update();
	}

	// 사각형 물리 업데이트
	GRectangle.Move();

	// === 충돌 처리 ===
	// HandleCollisions(); 공이 1개인 것을 전제로 한다
	HandleBallRectangleCollisions();
}


void GameScene::Render()
{
	URenderer* InRenderer = URenderer::GetInstance(); // 렌더러 인스턴스 가져오기

	// ================== RenderProcess에서 가져온 렌더링 로직 ==================
	// 공들 렌더링
	for (UPinBall* Ball : PinBalls)
	{
		InRenderer->UpdateConstant(Ball->GetLocation(), Ball->GetShape()->GetRadius());
		InRenderer->RenderPrimitive();
	}

	// 사각형 렌더링
	InRenderer->UpdateConstantForRectangle(GRectangle.Location, GRectangle.Width, GRectangle.Height);
	InRenderer->RenderRectangle();
}

/**
 * @brief 공들 간의 충돌을 감지하고 처리하는 함수
 */
[[deprecated]]
void GameScene::HandleCollisions()
{
	// for (int i = 0; i < ScenePrimitives.size(); ++i)
	// {
	// 	for (int j = i + 1; j < ScenePrimitives.size(); ++j)
	// 	{
	// 		UBall* Ball1 = static_cast<UBall*>(ScenePrimitives[i]);
	// 		UBall* Ball2 = static_cast<UBall*>(ScenePrimitives[j]);
	//
	// 		FVector3 Delta = Ball1->Location - Ball2->Location;
	// 		float DistanceSq = Delta.LengthSquare();
	// 		float CombinedRadius = Ball1->Radius + Ball2->Radius;
	//
	// 		if (DistanceSq < CombinedRadius * CombinedRadius && DistanceSq > 0.0f)
	// 		{
	// 			// 충돌 발생
	// 			float Distance = sqrtf(DistanceSq);
	// 			FVector3 Normal = Delta / Distance;
	//
	// 			// 겹침 해결
	// 			float Overlap = 0.5f * (CombinedRadius - Distance);
	// 			Ball1->Location += Normal * Overlap;
	// 			Ball2->Location -= Normal * Overlap;
	//
	// 			// 탄성 충돌 계산
	// 			FVector3 relativeVelocity = Ball1->Velocity - Ball2->Velocity;
	// 			float VelocityAlongNormal = Dot(relativeVelocity, Normal);
	//
	// 			if (VelocityAlongNormal < 0)
	// 			{
	// 				float Restitution = 1.0f; // 완전 탄성 충돌
	// 				float ImpulseScalar = -(1.0f + Restitution) * VelocityAlongNormal;
	// 				ImpulseScalar /= (1.0f / Ball1->Mass) + (1.0f / Ball2->Mass);
	//
	// 				FVector3 impulse = Normal * ImpulseScalar;
	// 				Ball1->Velocity += impulse * (1.0f / Ball1->Mass);
	// 				Ball2->Velocity -= impulse * (1.0f / Ball2->Mass);
	// 			}
	// 		}
	// 	}
	// }
}

/**
 * @brief 공과 사각형 간의 충돌을 감지하고 처리하는 함수
 * @param Ball 충돌을 검사할 공 객체
 * @param Rect 충돌을 검사할 사각형 객체
 */
void GameScene::ResolveBallRectangle(UPinBall* Ball, const URectangle* Rect)
{
	float HalfW = Rect->Width * 0.5f;
	float HalfH = Rect->Height * 0.5f;

	// 볼 중심에서 사각형 중심으로의 벡터 (사각형 로컬 좌표)
	FVector3 Delta = Ball->GetLocation() - Rect->Location;

	// 사각형 안에서 가장 가까운 점 (로컬)
	float ClampedX = Clamp(Delta.x, -HalfW, HalfW);
	float ClampedY = Clamp(Delta.y, -HalfH, HalfH);

	// 월드 좌표의 가장 가까운 점
	FVector3 Closest(Rect->Location.x + ClampedX,
	                 Rect->Location.y + ClampedY,
	                 Rect->Location.z);

	FVector3 Diff = Ball->GetLocation() - Closest;
	float DistSq = Diff.LengthSquare();
	float Radius = Ball->GetShape()->GetRadius();

	if (DistSq > Radius * Radius)
	{
		return; // 충돌 없음
	}

	FVector3 Normal;
	float Dist = sqrtf(DistSq);

	if (Dist > 0.00001f)
	{
		Normal = Diff / Dist;
	}
	else
	{
		// 중심선과 겹쳤을 때(볼 중심이 사각형 내부 깊숙하거나 정확히 중심)
		float PenX = HalfW - fabsf(Delta.x);
		float PenY = HalfH - fabsf(Delta.y);

		if (PenX < PenY)
		{
			Normal = FVector3((Delta.x >= 0.f) ? 1.f : -1.f, 0.f, 0.f);
			Dist = Radius - PenX;
		}
		else
		{
			Normal = FVector3(0.f, (Delta.y >= 0.f) ? 1.f : -1.f, 0.f);
			Dist = Radius - PenY;
		}
	}

	// 침투 깊이
	float Penetration = Radius - Dist;
	if (Penetration < 0.f)
	{
		return;
	}

	// 위치 보정 (사각형은 정적취급)
	Ball->GetLocation() += Normal * Penetration;

	// 속도 반사
	float Vn = Dot(Ball->GetVelocity(), Normal);
	if (Vn < 0.f)
	{
		float Restitution = 1.0f; // 필요시 조정
		Ball->GetVelocity() -= Normal * (1.f + Restitution) * Vn;
	}
}

/**
 * @brief 공과 사각형 간의 충돌을 감지하고 처리하는 함수
 */
void GameScene::HandleBallRectangleCollisions()
{
	for (int i = 0; i < static_cast<int>(ScenePrimitives.size()); ++i)
	{
		UPinBall* Ball = static_cast<UPinBall*>(ScenePrimitives[i]);
		ResolveBallRectangle(Ball, &GRectangle);
	}
}
