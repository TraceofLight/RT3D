#include "pch.h"
#include "Scene/Public/GameScene.h"
#include "Manager/Public/InputManager.h"
#include "Manager/Public/TimeManager.h"
#include "Manager/Public/SceneManager.h"
#include "Render/Public/Renderer.h"
#include "Actor/Public/PinBall.h"
#include "Actor/Public/Shooter.h"
#include "Mesh/Public/UBall.h"
#include "Mesh/Public/URectangle.h"
#include "Mesh/Public/UTriangle.h"
#include "Actor/Public/PadPair.h"

GameScene::GameScene() : Scene("GAME")
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
	pause = false;
    m_Rectangle = new URectangle();
	m_Shooted = false;
	m_ActorBall = nullptr;
	Shooter = new UShooter();
	Shooter->SetLocation({0.9f, -0.9f, 0.0f});

	FSceneManager& SceneMgr = FSceneManager::GetInstance();
	m_PrimitiveList = SceneMgr.GetAllScenePrimivites();

	GenerateObstacles();

	//Above Shooter
	AddNewRectangle(FVector3(0.8f, 0.8f, 0.0f), 0.3f, 0.1f, 0.4f,true);

	//Center
	AddNewRectangle(FVector3(0.0f, 0.0f, 0.0f), 0.3f, 0.1f, 0.4f, true);

	// PadPair 설정
	FPadPairConfig PadCfg;
	PadCfg.Base = 0.05f;
	PadCfg.Height = 0.3f;
	PadCfg.CenterX = 0.0f;
	PadCfg.CenterY = 0.0f;
	PadCfg.XOffset = 0.30f;
	PadCfg.YOffset = -0.85f;
	PadCfg.MidAngleDeg = 90.f;
	PadCfg.SweepHalfDeg = 30.f;
	PadCfg.RotationSpeedDeg = 360.f;
	PadCfg.KeyLeft = EKeyInput::A;
	PadCfg.KeyRight = EKeyInput::D;

	m_PadPair = new UPadPair();
	m_PadPair->Init(PadCfg);
}

void GameScene::GenerateObstacles()
{
	// right outer wall
	AddNewRectangle(FVector3(0.92f, -0.2f, 0.0f), 0.0f, 0.08f, 1.6f);
	// right inner wall
	AddNewRectangle(FVector3(0.7f, -0.2f, 0.0f), 0.0f, 0.12f, 1.6f);
	// left wall
	AddNewRectangle(FVector3(-0.88f, -0.2f, 0.0f), 0.0f, 0.35f, 1.6f);

	// left floor
	AddNewRectangle(FVector3(-0.728f, -0.65f, 0.0f), -0.6f, 0.95f, 0.2f);
	// right floor
	AddNewRectangle(FVector3(0.529f, -0.789f, 0.0f), 0.6f, 0.45f, 0.2f);

	// bottom left triangle
	AddNewTriangle(FVector3(-0.4f, -0.47f, 0.0f), 2.0f, 0.4f, 0.15f);
	// bottom right triangle
	AddNewTriangle(FVector3(0.35f, -0.47f, 0.0f), -2.0f, 0.4f, 0.15f);

	// triangle obstacle 1
	// AddNewTriangle(FVector3(0.58f, 0.2f, 0.0f), -2.3f, 0.45f, 0.25f);

	// left inner triangular wall
	AddNewTriangle(FVector3(0.6f, -0.0f, 0.0f), 1.57f, 0.85f, 0.15f);
	// right inner triangular wall
	AddNewTriangle(FVector3(-0.68f, -0.0f, 0.0f), -1.57f, 0.85f, 0.15f);

	// rectangle obstacle 1
	AddNewRectangle(FVector3(-0.3f, 0.3f, 0.0f), -0.4f, 0.35f, 0.1f);
	// rectangle obstacle 2


	// temp shooter top
	AddNewRectangle(FVector3(0.9f, 0.9f, 0.0f), -0.6f, 0.2f, 0.2f);
}

void GameScene::Update(float deltaTime)
{
    FTimeManager* TimeManager = FTimeManager::GetInstance();
    FInputManager* KeyManager = FInputManager::GetInstance();
	FSceneManager& SceneMgr = FSceneManager::GetInstance();

	// 지연 삭제 처리 (프레임 시작 시)
	ProcessDelayedDeletions();

	m_PrimitiveList = SceneMgr.GetAllScenePrimivites();

    InputProcess();

	if (FSceneManager::GetInstance().GetCurrentScene()->GetName() != "GAME")
		return;

	m_TotalPrimitives = static_cast<int>(m_PrimitiveList->size());

	// 삭제 예정인 볼을 제외한 모든 볼 객체의 움직임 업데이트
	for (int i = 0; i < m_TotalPrimitives; ++i)
	{
		auto PrimitiveList = *m_PrimitiveList;
		UPinBall* Ball = dynamic_cast<UPinBall*>(PrimitiveList[i]);
		if (Ball && !IsMarkedForDeletion(Ball))
		{
			Ball->Move();
		}
		URectangle* rect = dynamic_cast<URectangle*>(PrimitiveList[i]);
		if (rect != nullptr)
		{
			rect->Update();
		}
	}

	if (isGameOver())
	{
		pause = true;
	}
	m_PadPair->Update(KeyManager, TimeManager->GetDeltaTime());

	// 모든 충돌 처리
	CollisionProcess();

	// 공 트리거 체크 (공이 화면 하단에 도달하거나 범위를 벗어난 경우)
	CheckBallTriggers();
}

void GameScene::Render()
{
    RenderProcess();
}

void GameScene::Cleanup()
{
    for (int i = 0; i < m_TotalPrimitives; ++i)
    {
        delete (*m_PrimitiveList)[i];
    }
	m_PrimitiveList->clear();
    if(m_Rectangle)
    {
        delete m_Rectangle;
        m_Rectangle = nullptr;
    }

	delete Shooter;
	delete m_PadPair;

	// Clear Delayed Task Target
	m_BallsToDelete.clear();

	m_TotalPrimitives = 0;
}

void GameScene::InputProcess()
{
	FInputManager* KeyManager = FInputManager::GetInstance();

	if (KeyManager->IsKeyPressed(EKeyInput::Esc))
	{
		FSceneManager::GetInstance().LoadScene("LOBBY");
		return;
	}

	// Charging & Shoot
	if (Shooter && Shooter->CanShoot())
	{
		if (KeyManager->IsKeyDown(EKeyInput::Space))
		{
			// 스페이스바를 누르고 있는 동안 차징 (발사 가능할 때만)
			DEBUG_PRINT("[Shooter] Shooter Charging...\n");
			Shooter->Charging();
		}
		else if (KeyManager->IsKeyReleased(EKeyInput::Space))
		{
			if (!m_Shooted)
			{

				m_ActorBall = Shooter->Shoot();
				if (m_ActorBall != nullptr)
				{
					m_Shooted = true;
				}
				DEBUG_PRINT("[Shooter] Shooter Fire!\n");
			}
		}
	}
	else if (Shooter && !Shooter->CanShoot())
	{
		if (KeyManager->IsKeyPressed(EKeyInput::Space))
		{
			DEBUG_PRINT("[Shooter] Shooter not ready - ball already fired!\n");
		}
	}
}

void GameScene::CheckBallTriggers()
{
	for (UPrimitive* Ball : *m_PrimitiveList)
	{
		UPinBall* PinBall = dynamic_cast<UPinBall*>(Ball);
		if (PinBall && PinBall != m_GravityCenterBall)
		{
			// 화면 범위를 벗어났거나 하단에 도달했는지 확인
			bool ShouldTrigger = false;
			FVector3 pos = PinBall->GetLocation();

			// 화면 하단 도달 체크 (정규화된 좌표계에서 -1.0f가 화면 하단)
			if (pos.y <= -1.0f + PinBall->GetShape()->GetRadius())
			{
				ShouldTrigger = true;
				DEBUG_PRINT("[Ball Trigger] Ball reached bottom of screen\n");
			}
			// 좌우 범위 벗어남 체크 (정규화된 좌표계에서 ±1.5f 정도가 화면 밖)
			else if (pos.x < -1.5f || pos.x > 1.5f)
			{
				ShouldTrigger = true;
				DEBUG_PRINT("[Ball Trigger] Ball went out of bounds (x=%.2f)\n", pos.x);
			}

			if (ShouldTrigger)
			{
				// 이미 삭제 예정인 볼인지 확인
				auto iter = std::find(m_BallsToDelete.begin(), m_BallsToDelete.end(), PinBall);
				if (iter == m_BallsToDelete.end()) // 아직 삭제 목록에 없으면
				{
					// 슈터에 볼 트리거 알림
					Shooter->OnBallTrigger();

					// 삭제할 볼 목록에 추가 (다음 프레임에서 삭제)
					m_BallsToDelete.push_back(PinBall);
					DEBUG_PRINT("[Ball Trigger] Ball marked for deletion\n");
				}
				// 스페이스바를 뗐을 때 발사
			}
		}
	}
}

void GameScene::RenderProcess()
{
    URenderer* Renderer = URenderer::GetInstance();
    //Renderer->Prepare();
    //Renderer->PrepareShader();

	for (int i = 0; i < m_TotalPrimitives; ++i)
	{
		UPrimitive* Primitive = (*m_PrimitiveList)[i];

		if (UPinBall* Ball = dynamic_cast<UPinBall*>((*m_PrimitiveList)[i]))
		{
			if (!IsMarkedForDeletion(Ball))
			{
				Renderer->UpdateConstant(Ball->GetLocation(), Ball->GetShape()->GetRadius());
				Renderer->RenderPrimitive();
			}
		}
		else if (URectangle* Rectangle = dynamic_cast<URectangle*>(Primitive))
		{
			Renderer->UpdateConstantForRectangle(Rectangle->Location, Rectangle->Width, Rectangle->Height, Rectangle->Rotation);
			Renderer->RenderRectangle();
		}
		else if (UTriangle* Triangle = dynamic_cast<UTriangle*>(Primitive))
		{
			Renderer->UpdateConstantForTriangle(Triangle->Location, Triangle->Base, Triangle->Height, Triangle->Rotation,
										 Triangle->Radius);
			Renderer->RenderTriangle();
		}
	}

    // Shooter 렌더링 추가
    if (Shooter && Shooter->GetShape())
    {
        Renderer->UpdateConstantForRectangle(Shooter->GetLocation(),
                                           Shooter->GetShape()->GetWidth(),
                                           Shooter->GetShape()->GetHeight(),0.0f);
        Renderer->RenderRectangle();
    }

	// PadPair 렌더링
	m_PadPair->Render(*Renderer);
}

void GameScene::CollisionProcess()
{
	if (m_ActorBall == nullptr || pause)
	{
		return;
	}

	HandlePadPairCollisions();
	HandleRectangleCollisions();
	HandleTriangleCollisions();
}

/**
 * @brief 공과 사각형 간의 충돌을 감지하고 처리하는 함수
 * @param PinBall 충돌을 검사할 핀볼 객체
 * @param Rect 충돌을 검사할 사각형 객체
 */
void GameScene::ResolveRectangleCollsion(UPinBall* PinBall, const URectangle* Rect)
{
	const float HalfW = Rect->Width * 0.5f;
	const float HalfH = Rect->Height * 0.5f;

	// 1) 월드 좌표에서 공-사각형 중심까지 벡터
	const FVector3 ballPos = PinBall->GetLocation();
	const FVector3 rectPos = Rect->Location;
	const FVector3 deltaWorld = ballPos - rectPos; // z는 무시 (2D)

	// 2) 직사각형의 회전 축(월드 기준 로컬 축) 구성
	//    로컬 x축: (cosθ, sinθ), 로컬 y축: (-sinθ, cosθ)  (CCW 기준)
	const float c = cosf(Rect->Rotation);
	const float s = sinf(Rect->Rotation);
	const FVector3 axisX(c, s, 0.f);   // 로컬 x축이 월드에서 가리키는 방향
	const FVector3 axisY(-s, c, 0.f);   // 로컬 y축이 월드에서 가리키는 방향

	// 3) 월드→로컬 (축으로 투영)
	//    로컬 좌표 = [dot(delta, axisX), dot(delta, axisY)]
	const float localX = Dot(deltaWorld, axisX);
	const float localY = Dot(deltaWorld, axisY);

	// 4) 로컬에서 AABB 최근접점 계산 (기존 로직 그대로)
	const float clampedX = Clamp(localX, -HalfW, HalfW);
	const float clampedY = Clamp(localY, -HalfH, HalfH);

	// 5) 최근접점의 월드 좌표 복원: rectPos + axisX*clampedX + axisY*clampedY
	const FVector3 closestWorld = rectPos + axisX * clampedX + axisY * clampedY;

	// 6) 월드에서 거리/법선 계산
	FVector3 diffWorld = ballPos - closestWorld;
	float distSq = diffWorld.LengthSquare();
	const float radius = PinBall->GetShape()->GetRadius();

	if (distSq > radius * radius)
		return; // 미충돌

	FVector3 normalWorld;
	float dist = sqrtf(distSq);

	if (dist > 1e-5f)
	{
		// 표면에서 바깥쪽을 향하는 월드 법선
		normalWorld = diffWorld / dist;
	}
	else
	{
		// 공의 중심이 꼭짓점에 딱 붙었거나 내부에 수치적으로 들어간 경우
		// 로컬 공간에서 더 얕은 침투 축을 찾아 축 법선을 선택
		const float penX = HalfW - fabsf(localX);
		const float penY = HalfH - fabsf(localY);

		if (penX < penY)
		{
			// 로컬 +X 또는 -X 면에 충돌 -> 월드 법선은 axisX 방향 부호만 반영
			const float sign = (localX >= 0.f) ? 1.f : -1.f;
			normalWorld = axisX * sign;
			dist = radius - penX;
		}
		else
		{
			// 로컬 +Y 또는 -Y 면
			const float sign = (localY >= 0.f) ? 1.f : -1.f;
			normalWorld = axisY * sign;
			dist = radius - penY;
		}
		// axisX/axisY는 이미 정규화(단위벡터)이므로 normalWorld는 단위길이
	}

	const float penetration = radius - dist;
	if (penetration < 0.f)
		return;

	// 7) 위치 보정 (월드에서)
	PinBall->GetLocation() += normalWorld * penetration;

	// 8) 속도 반사 (월드 법선 사용)
	float vn = Dot(PinBall->GetVelocity(), normalWorld);
	if (vn < 0.f)
	{
		const float restitution = 1.0f; // 필요에 맞게 조정
		PinBall->GetVelocity() -= normalWorld * (1.f + restitution) * vn;
	}
}

void GameScene::ResolveTriangleCollision(UPinBall* PinBall, const UTriangle* Triangle)
{
    if (!PinBall || !Triangle)
        return;

    const float a = Triangle->Base;
    const float b = Triangle->Height;
    const float r = Triangle->Radius;

    FVector3 v0(-r, -r, 0.0f);
    FVector3 v1(-r, b - r, 0.0f);
    FVector3 v2(a - r, -r, 0.0f);

    const float c = std::cos(Triangle->Rotation);
    const float s = std::sin(Triangle->Rotation);

    auto Rotate = [&](const FVector3& L) -> FVector3
    {
        return FVector3(L.x * c - L.y * s, L.x * s + L.y * c, 0.0f);
    };

    FVector3 w0 = Rotate(v0) + Triangle->Location;
    FVector3 w1 = Rotate(v1) + Triangle->Location;
    FVector3 w2 = Rotate(v2) + Triangle->Location;

    auto ClosestPointOnSegment = [](const FVector3& A, const FVector3& B, const FVector3& P) -> FVector3
    {
        FVector3 AB = B - A;
        float abLenSq = AB.LengthSquare();
        if (abLenSq <= 1e-12f) return A;
        float t = Dot(P - A, AB) / abLenSq;
        if (t < 0.0f) t = 0.0f;
        else if (t > 1.0f) t = 1.0f;
        return A + AB * t;
    };

    const FVector3 C = PinBall->GetLocation();

    FVector3 candidates[3];
    candidates[0] = ClosestPointOnSegment(w0, w1, C);
    candidates[1] = ClosestPointOnSegment(w1, w2, C);
    candidates[2] = ClosestPointOnSegment(w2, w0, C);

    float bestDistSq = FLT_MAX;
    FVector3 closest;
    for (int i = 0; i < 3; ++i)
    {
        FVector3 d = C - candidates[i];
        float dsq = d.LengthSquare();
        if (dsq < bestDistSq)
        {
            bestDistSq = dsq;
            closest = candidates[i];
        }
    }

    float dist = std::sqrtf(bestDistSq);
    if (dist > PinBall->GetShape()->GetRadius())
        return;

    FVector3 normal;
    if (dist > 1e-6f)
    {
        normal = (C - closest) / dist;
    }
    else
    {
        normal = (C - Triangle->Location);
        if (normal.LengthSquare() < 1e-8f)
            normal = FVector3(1.f, 0.f, 0.f);
        else
            normal.Normalize();
    }

    float penetration = PinBall->GetShape()->GetRadius() - dist;
    if (penetration > 0.f)
    {
		PinBall->GetLocation() += normal * penetration;
    }

    float vn = Dot(PinBall->GetVelocity(), normal);
    if (vn < 0.f)
    {
        const float Restitution = 1.0f;
		PinBall->GetVelocity() -= normal * (1.f + Restitution) * vn;
    }
}

void GameScene::HandlePadPairCollisions()
{
	if (!IsMarkedForDeletion(m_ActorBall))
	{
		ResolveTriangleCollision(m_ActorBall, m_PadPair->Left().GetShape());
		ResolveTriangleCollision(m_ActorBall, m_PadPair->Right().GetShape());
	}
}

void GameScene::HandleRectangleCollisions()
{
	// ActorBall과 다른 모든 Rectangle 간의 충돌 처리
	for (int i = 0; i < static_cast<int>(m_PrimitiveList->size()); ++i)
	{
		URectangle* rect = dynamic_cast<URectangle*>((*m_PrimitiveList)[i]);
		if (rect != nullptr)
		{
			ResolveRectangleCollsion(m_ActorBall, rect);
		}
	}
}

void GameScene::HandleTriangleCollisions()
{
	// ActorBall과 다른 모든 Triangle 간의 충돌 처리
	for (int i = 0; i < static_cast<int>(m_PrimitiveList->size()); ++i)
	{
		UTriangle* tri = dynamic_cast<UTriangle*>((*m_PrimitiveList)[i]);
		if (tri != nullptr)
		{
			ResolveTriangleCollision(m_ActorBall, tri);
		}
	}
}

void GameScene::AddNewBall()
{

}

void GameScene::AddNewRectangle(FVector3 location, float rotation, float width, float height,bool autoRotation)
{
	// Create the rectangle
	URectangle* NewRectangle = new URectangle();
	NewRectangle->Location = location;
	NewRectangle->Rotation = rotation;
	NewRectangle->Width = width;
	NewRectangle->Height = height;
	NewRectangle->Rotation = rotation;
	NewRectangle->Mass = width * height;
	NewRectangle->Velocity = FVector3(0.0f, 0.0f, 0.0f);
	NewRectangle->AutoRotation = autoRotation;

	FSceneManager& FCM = FSceneManager::GetInstance();
	Scene* currentScene = FCM.GetCurrentScene();
	// Add to the vector
	currentScene->GetScenePrimitives()->push_back(NewRectangle);

	// Update total count
	++m_TotalPrimitives;
}

void GameScene::AddNewTriangle(FVector3 location, float rotation, float base, float height)
{
	UTriangle* NewTriangle = new UTriangle();
	NewTriangle->Location = location;
	NewTriangle->Rotation = rotation;
	NewTriangle->Base = base;
	NewTriangle->Height = height;

	FSceneManager& FCM = FSceneManager::GetInstance();
	Scene* currentScene = FCM.GetCurrentScene();
	// Add to the vector
	currentScene->GetScenePrimitives()->push_back(NewTriangle);

	// Update total count
	++m_TotalPrimitives;
}

bool GameScene::isGameOver()
{
	UPinBall* Ball = m_ActorBall;
	if (Ball != nullptr)
	{

		if (Ball->GetLocation().y < -1.0f)
		{
			return true;
		}
	}

	return false;
}

void GameScene::ProcessDelayedDeletions()
{
	if (m_BallsToDelete.empty())
		return;

	DEBUG_PRINT("[Delayed Deletion] Processing %d balls for deletion\n", static_cast<int>(m_BallsToDelete.size()));

	FSceneManager& SceneManager = FSceneManager::GetInstance();

	// 삭제할 볼들을 순회하면서 처리
	for (UPinBall* BallToDelete : m_BallsToDelete)
	{
		// SceneManager를 통해 씨너의 프리미티브 리스트에서 제거
		SceneManager.RemovePrimitiveFromScene(BallToDelete);
		DEBUG_PRINT("[Delayed Deletion] Ball removed from scene primitives\n");

		// 메모리에서 삭제
		delete BallToDelete;
		DEBUG_PRINT("[Delayed Deletion] Ball deleted from memory\n");
	}

	// 삭제 목록 비우기
	m_BallsToDelete.clear();
	DEBUG_PRINT("[Delayed Deletion] Deletion process completed\n");
}

bool GameScene::IsMarkedForDeletion(UPinBall* InBall) const
{
	return std::find(m_BallsToDelete.begin(), m_BallsToDelete.end(), InBall) != m_BallsToDelete.end();
}
