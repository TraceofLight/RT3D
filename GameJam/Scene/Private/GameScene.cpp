#include "pch.h"
#include "Scene/Public/GameScene.h"
#include "Manager/Public/InputManager.h"
#include "Manager/Public/TimeManager.h"
#include "Manager/Public/SceneManager.h"
#include "Render/Public/Renderer.h"
#include "Actor/Public/PinBall.h"
#include "Mesh/Public/UBall.h"
#include "Mesh/Public/URectangle.h"
#include "Mesh/Public/UTriangle.h"

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
    m_Rectangle = new URectangle();
    m_Triangle = new UTriangle();
}

void GameScene::Update(float deltaTime)
{
    FTimeManager* TimeManager = FTimeManager::GetInstance();
    FInputManager* KeyManager = FInputManager::GetInstance();
	SceneManager& SceneMgr = SceneManager::GetInstance();

	m_PrimitiveList = SceneMgr.GetAllScenePrimivites();

    InputProcess();

    m_Triangle->UpdateRotation(KeyManager, TimeManager->GetDeltaTime());

    for (int i = 0; i < m_TotalPrimitives; ++i)
    {
        UPinBall* Ball = static_cast<UPinBall*>(m_PrimitiveList[i]);
        Ball->Move();
    }

    m_Rectangle->Move();

    // HandleCollisions();
    HandleBallRectangleCollisions();
    HandleBallTriangleCollisions();
}

void GameScene::Render()
{
    RenderProcess();
}

void GameScene::Cleanup()
{
    for (int i = 0; i < m_TotalPrimitives; ++i)
    {
        delete m_PrimitiveList[i];
    }

    if (m_PrimitiveList)
    {
        delete[] m_PrimitiveList;
        m_PrimitiveList = nullptr;
    }

    if(m_Rectangle)
    {
        delete m_Rectangle;
        m_Rectangle = nullptr;
    }

    if(m_Triangle)
    {
        delete m_Triangle;
        m_Triangle = nullptr;
    }

    m_TotalPrimitives = 0;
}

void GameScene::InputProcess()
{
    FInputManager* KeyManager = FInputManager::GetInstance();

    if (KeyManager->IsKeyPressed(EKeyInput::Delete))
    {
        RemoveRandomBall();
    }

	if (KeyManager->IsKeyPressed(EKeyInput::End))
	{
		SceneManager::GetInstance().LoadScene("LOBBY");
	}
}

void GameScene::RenderProcess()
{
    URenderer* Renderer = URenderer::GetInstance();
    //Renderer->Prepare();
    //Renderer->PrepareShader();

    for (int i = 0; i < m_TotalPrimitives; ++i)
    {
        UPinBall* Ball = static_cast<UPinBall*>(m_PrimitiveList[i]);
        Renderer->UpdateConstant(Ball->GetLocation(), Ball->GetShape()->GetRadius());
        Renderer->RenderPrimitive();
    }

    Renderer->UpdateConstantForRectangle(m_Rectangle->Location, m_Rectangle->Width, m_Rectangle->Height);
    Renderer->RenderRectangle();

    Renderer->UpdateConstantForTriangle(m_Triangle->Location, m_Triangle->Base, m_Triangle->Height, m_Triangle->Rotation, m_Triangle->Radius);
    Renderer->RenderTriangle();

    //FImGuiManager::RenderImGui();
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

void GameScene::HandleBallTriangleCollisions()
{
    for (int i = 0; i < m_TotalPrimitives; ++i)
    {
        UPinBall* Ball = static_cast<UPinBall*>(m_PrimitiveList[i]);
        ResolveBallTriangle(Ball, m_Triangle);
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

void GameScene::HandleBallRectangleCollisions()
{
	for (int i = 0; i < static_cast<int>(m_TotalPrimitives.size()); ++i)
	{
		UPinBall* Ball = static_cast<UPinBall*>(m_TotalPrimitives[i]);
		ResolveBallRectangle(Ball, &GRectangle);
	}
}
