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

	Shooter = new UShooter();
	Shooter->SetLocation({0.3f, -0.8f, 0.0f});

	// PadPair 설정
	FPadPairConfig PadCfg;
	PadCfg.Base = 0.05f;
	PadCfg.Height = 0.3f;
	PadCfg.XOffset = 0.30f;
	PadCfg.YOffset = -0.65f;
	PadCfg.MidAngleDeg = 90.f;
	PadCfg.SweepHalfDeg = 30.f;
	PadCfg.RotationSpeedDeg = 360.f;
	PadCfg.KeyLeft = EKeyInput::A;
	PadCfg.KeyRight = EKeyInput::D;

	m_PadPair = new UPadPair();
	m_PadPair->Init(PadCfg);
}

void GameScene::Update(float deltaTime)
{
    FTimeManager* TimeManager = FTimeManager::GetInstance();
    FInputManager* KeyManager = FInputManager::GetInstance();
	FSceneManager& SceneMgr = FSceneManager::GetInstance();

	m_PrimitiveList = SceneMgr.GetAllScenePrimivites();

    InputProcess();

	if (FSceneManager::GetInstance().GetCurrentScene()->GetName() != "GAME")
		return;

	m_TotalPrimitives = static_cast<int>(m_PrimitiveList->size());

    // 모든 볼 객체의 움직임 업데이트
    for (int i = 0; i < m_TotalPrimitives; ++i)
    {
        UPinBall* Ball = static_cast<UPinBall*>((*m_PrimitiveList)[i]);
        Ball->Move();
    }

    m_Rectangle->Move();

	//m_PadPair.Update(KeyManager, TimeManager->GetDeltaTime());
	if (isGameOver())
	{
		pause = true;
	}
	m_PadPair->Update(KeyManager, TimeManager->GetDeltaTime());

    // HandleCollisions();
    // HandleBallRectangleCollisions();
	HandleBallPadPairCollisions();
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

	// 스페이스바 처리 - 차징 및 발사
	if (Shooter)
	{
		if (KeyManager->IsKeyDown(EKeyInput::Space))
		{
			// 스페이스바를 누르고 있는 동안 차징
			DEBUG_PRINT("[MAINLOOP] Shooter Charging...\n");
			Shooter->Charging();
		}
		else if (KeyManager->IsKeyReleased(EKeyInput::Space))
		{
			// 스페이스바를 뗐을 때 발사
			if (m_TotalPrimitives < 1)
			{
				Shooter->Shoot();
				DEBUG_PRINT("[MAINLOOP] Shooter Fire!\n");
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
        UPinBall* Ball = static_cast<UPinBall*>((*m_PrimitiveList)[i]);
        Renderer->UpdateConstant(Ball->GetLocation(), Ball->GetShape()->GetRadius());
        Renderer->RenderPrimitive();
    }

    // Shooter 렌더링 추가
    if (Shooter && Shooter->GetShape())
    {
        Renderer->UpdateConstantForRectangle(Shooter->GetLocation(), 
                                           Shooter->GetShape()->GetWidth(), 
                                           Shooter->GetShape()->GetHeight());
        Renderer->RenderRectangle();
    }

	// PadPair 렌더링
	m_PadPair->Render(*Renderer);
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

    FVector3 Delta = Ball->GetLocation() - Rect->Location;

    float ClampedX = Clamp(Delta.x, -HalfW, HalfW);
    float ClampedY = Clamp(Delta.y, -HalfH, HalfH);

    FVector3 Closest(Rect->Location.x + ClampedX, Rect->Location.y + ClampedY, Rect->Location.z);

    FVector3 Diff = Ball->GetLocation() - Closest;
    float DistSq = Diff.LengthSquare();
    float Radius = Ball->GetShape()->GetRadius();

    if (DistSq > Radius * Radius)
    {
        return;
    }

    FVector3 Normal;
    float Dist = sqrtf(DistSq);

    if (Dist > 0.00001f)
    {
        Normal = Diff / Dist;
    }
    else
    {
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

    float Penetration = Radius - Dist;
    if (Penetration < 0.f)
    {
        return;
    }

    Ball->GetLocation() += Normal * Penetration;

    float Vn = Dot(Ball->GetVelocity(), Normal);
    if (Vn < 0.f)
    {
        float Restitution = 1.0f;
        Ball->GetVelocity() -= Normal * (1.f + Restitution) * Vn;
    }
}

void GameScene::ResolveBallTriangle(UPinBall* Ball, const UTriangle* Triangle)
{
    if (!Ball || !Triangle)
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

    const FVector3 C = Ball->GetLocation();

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
    if (dist > Ball->GetShape()->GetRadius())
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

    float penetration = Ball->GetShape()->GetRadius() - dist;
    if (penetration > 0.f)
    {
        Ball->GetLocation() += normal * penetration;
    }

    float vn = Dot(Ball->GetVelocity(), normal);
    if (vn < 0.f)
    {
        const float Restitution = 1.0f;
        Ball->GetVelocity() -= normal * (1.f + Restitution) * vn;
    }
}

void GameScene::HandleBallPadPairCollisions()
{
	for (int i = 0; i < m_TotalPrimitives; ++i)
	{
		UPinBall* Ball = static_cast<UPinBall*>((*m_PrimitiveList)[i]);
		ResolveBallTriangle(Ball, m_PadPair->Left().GetShape());
		ResolveBallTriangle(Ball, m_PadPair->Right().GetShape());
	}
}


/*void GameScene::HandleBallRectangleCollisions()
{
	for (int i = 0; i < static_cast<int>(m_PrimitiveList->size()); ++i)
	{
		UPinBall* Ball = static_cast<UPinBall*>((*m_PrimitiveList)[i]);
		ResolveBallRectangle(Ball, &GRectangle);
	}
}*/
bool GameScene::isGameOver()
{
	if (m_TotalPrimitives > 0)
	{
		UPinBall* Ball = static_cast<UPinBall*>((*m_PrimitiveList)[0]);
		if (Ball->GetLocation().y < -1.0f)
		{
			return true;
		}
	}
	return false;
}
