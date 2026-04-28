#include "pch.h"
#include "Scene/Public/GameScene.h"
#include "Manager/Public/InputManager.h"
#include "Manager/Public/TimeManager.h"
#include "Manager/Public/SceneManager.h"
#include "Manager/Public/ScoreManager.h"
#include "Render/Public/Renderer.h"
#include "Actor/Public/PinBall.h"
#include "Actor/Public/Shooter.h"
#include "Mesh/Public/UBall.h"
#include "Mesh/Public/URectangle.h"
#include "Mesh/Public/UTriangle.h"
#include "Mesh/Public/UConcaveCircle.h"
#include "Actor/Public/FlipperPair.h"

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
	bIsPause = false;
	m_Rectangle = new URectangle();
	m_Shooted = false;
	m_ActorBall = nullptr;
	Shooter = new UShooter();
	Shooter->SetLocation({0.85f, -0.9f, 0.0f});

	// ScoreManager 초기화
	m_ScoreManager = FScoreManager::GetInstance();
	m_ScoreManager->ResetCurrentScore();

	FSceneManager& SceneMgr = FSceneManager::GetInstance();
	m_PrimitiveList = SceneMgr.GetAllScenePrimivites();

	GenerateObstacles();

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
	PadCfg.RotationSpeedDeg = 270.f; // CCD 문제 방지를 위한 속도 조정
	PadCfg.KeyLeft = EKeyInput::A;
	PadCfg.KeyRight = EKeyInput::D;

	m_PadPair = new UFlipperPair();
	m_PadPair->Init(PadCfg);

	// 상단 범퍼 벽
	m_TopBumperWall = new UConcaveCircle(
		FVector3(0.09f, 0.15f, 0.0f), // 상단 중앙
		0.85f, // 반지름
		0.35f, // 시작 각도 (0도)
		PI - 0.35f // 끝 각도 (180도)
	);

	// 중앙 범퍼
	m_CenterBumperBall = new UBall();
	m_CenterBumperBall->SetLocation(FVector3(0.0f, -0.3f, 0.0f));
	m_CenterBumperBall->SetRadius(0.15f);
	m_CenterBumperBall->SetVelocity(FVector3(0.0f, 0.0f, 0.0f)); // 정지 상태

	// 위 범퍼
	m_TopBumperBall1 = new UBall();
	m_TopBumperBall1->SetLocation(FVector3(-0.2f, 0.4f, 0.0f));
	m_TopBumperBall1->SetRadius(0.11f);
	m_TopBumperBall1->SetVelocity(FVector3(0.0f, 0.0f, 0.0f)); // 정지 상태

	m_TopBumperBall2 = new UBall();
	m_TopBumperBall2->SetLocation(FVector3(0.25f, 0.6f, 0.0f));
	m_TopBumperBall2->SetRadius(0.09f);
	m_TopBumperBall2->SetVelocity(FVector3(0.0f, 0.0f, 0.0f)); // 정지 상태

	m_TopBumperBall3 = new UBall();
	m_TopBumperBall3->SetLocation(FVector3(0.2f, 0.25f, 0.0f));
	m_TopBumperBall3->SetRadius(0.095f);
	m_TopBumperBall3->SetVelocity(FVector3(0.0f, 0.0f, 0.0f)); // 정지 상태
}

void GameScene::GenerateObstacles()
{
	// right outer wall
	AddNewRectangle(FVector3(0.92f, -0.3f, 0.0f), 0.0f, 0.08f, 1.5f);
	// right inner wall
	AddNewRectangle(FVector3(0.735f, -0.3f, 0.0f), 0.0f, 0.12f, 1.5f);
	// left wall
	AddNewRectangle(FVector3(-0.88f, -0.3f, 0.0f), 0.0f, 0.35f, 1.5f);

	// left floor
	AddNewRectangle(FVector3(-0.728f, -0.65f, 0.0f), -0.6f, 0.95f, 0.2f);
	// right floor
	AddNewRectangle(FVector3(0.526f, -0.792f, 0.0f), 0.6f, 0.5f, 0.2f);

	// bottom left triangle
	AddNewTriangle(FVector3(-0.4f, -0.52f, 0.0f), 2.0f, 0.3f, 0.1f);
	// bottom right triangle
	AddNewTriangle(FVector3(0.44f, -0.52f, 0.0f), -2.0f, 0.3f, 0.1f);

	// triangle obstacle 1
	// AddNewTriangle(FVector3(0.58f, 0.2f, 0.0f), -2.3f, 0.45f, 0.25f);

	// right inner triangular wall
	AddNewTriangle(FVector3(0.64f, -0.0f, 0.0f), 1.57f, 0.85f, 0.15f);
	// left inner triangular wall
	AddNewTriangle(FVector3(-0.68f, -0.0f, 0.0f), -1.57f, 0.85f, 0.15f);

	// rectangle obstacle 1
	AddNewRectangle(FVector3(-0.32f, -0.f, 0.0f), 1.0f, 0.06f, 0.25f);

	// bottom fillings
	AddNewRectangle(FVector3(0.58f, -0.91f, 0.0f), -0.0f, 0.4f, 0.28f);
	AddNewRectangle(FVector3(-0.58f, -0.9f, 0.0f), -0.0f, 0.36f, 0.28f);

	//Above Shooter
	// AddNewRectangle(FVector3(0.8f, 0.8f, 0.0f), 0.3f, 0.1f, 0.4f,true);

	// rotating rectangles
	// AddNewRectangle(FVector3(0.1f, 0.3f, 0.0f), 0.3f, 0.05f, 0.2f, true,RectRotType::CLOCK);
	// AddNewRectangle(FVector3(0.285f, 0.0f, 0.0f), 0.3f, 0.05f, 0.2f, true,RectRotType::REVCLOCK);
	// AddNewRectangle(FVector3(0.285f, 0.65f, 0.0f), 0.3f, 0.07f, 0.35f, true,RectRotType::REVCLOCK);
	AddNewRectangle(FVector3(-0.1f, 0.75f, 0.0f), 0.3f, 0.06f, 0.26f, true, RectRotType::CLOCK);

	// temp shooter top
	// AddNewRectangle(FVector3(0.9f, 0.9f, 0.0f), -0.6f, 0.2f, 0.2f);
}

void GameScene::Update(float deltaTime)
{
	FTimeManager* TimeManager = FTimeManager::GetInstance();
	FInputManager* KeyManager = FInputManager::GetInstance();
	FSceneManager& SceneMgr = FSceneManager::GetInstance();

	// 지연 삭제 처리 (프레임 시작 시)
	ProcessDelayedDeletions();

	// ScoreManager 업데이트 (시간 기반 점수 처리)
	// 공이 발사된 후에만 시간 기반 점수 가산
	if (m_ScoreManager && !bIsPause && m_Shooted)
	{
		m_ScoreManager->Update();
	}

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
		SetPause();
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
	if (m_Rectangle)
	{
		delete m_Rectangle;
		m_Rectangle = nullptr;
	}

	delete Shooter;
	delete m_PadPair;

	if (m_TopBumperWall)
	{
		delete m_TopBumperWall;
		m_TopBumperWall = nullptr;
	}
	if (m_CenterBumperBall)
	{
		delete m_CenterBumperBall;
		m_CenterBumperBall = nullptr;
	}
	if (m_TopBumperBall1)
	{
		delete m_TopBumperBall1;
		m_TopBumperBall1 = nullptr;
	}
	if (m_TopBumperBall2)
	{
		delete m_TopBumperBall2;
		m_TopBumperBall2 = nullptr;
	}
	if (m_TopBumperBall3)
	{
		delete m_TopBumperBall3;
		m_TopBumperBall3 = nullptr;
	}

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
			Renderer->UpdateConstantForRectangle(Rectangle->Location, Rectangle->Width, Rectangle->Height,
			                                     Rectangle->Rotation);
			Renderer->RenderRectangle();
		}
		else if (UTriangle* Triangle = dynamic_cast<UTriangle*>(Primitive))
		{
			Renderer->UpdateConstantForTriangle(Triangle->Location, Triangle->Base, Triangle->Height,
			                                    Triangle->Rotation,
			                                    Triangle->Radius);
			Renderer->RenderTriangle();
		}
		else if (UConcaveCircle* ConcaveCircle = dynamic_cast<UConcaveCircle*>(Primitive))
		{
			Renderer->UpdateConstant(ConcaveCircle->GetCenter(), ConcaveCircle->GetRadius());
			Renderer->RenderPrimitive();
		}
	}

	// Shooter 렌더링 추가
	if (Shooter && Shooter->GetShape())
	{
		Renderer->UpdateConstantForRectangle(Shooter->GetLocation(),
		                                     Shooter->GetShape()->GetWidth(),
		                                     Shooter->GetShape()->GetHeight(), 0.0f);
		Renderer->RenderRectangle();
	}

	// PadPair 렌더링
	m_PadPair->Render(*Renderer);

	// ConcaveCircle 렌더링
	auto RenderConcave = [&](const UConcaveCircle* CC, float cr, float cg, float cb, float ca)
	{
		if (!CC) return;
		const FVector3 C = CC->GetCenter();
		const float R = CC->GetRadius();
		const bool bFull = CC->IsFullCircle();
		const float Start = CC->GetArcAngleStart();
		const float End = CC->GetArcAngleEnd();
		const int Segments = 64;
		FVertexSimple verts[128];
		int count = 0;
		auto append_segment = [&](const FVector3& A, const FVector3& B)
		{
			if (count + 2 > 128) return;
			verts[count++] = {A.x, A.y, A.z, cr, cg, cb, ca};
			verts[count++] = {B.x, B.y, B.z, cr, cg, cb, ca};
		};

		auto build_ring = [&](float radius, bool closeLoop)
		{
			if (bFull)
			{
				FVector3 prev = {C.x + radius, C.y + 0.0f, C.z};
				for (int i = 1; i <= Segments; ++i)
				{
					float t = (float)i / (float)Segments;
					float ang = t * 6.28318530718f;
					FVector3 curr = {C.x + radius * cosf(ang), C.y + radius * sinf(ang), C.z};
					append_segment(prev, curr);
					prev = curr;
				}
			}
			else
			{
				float span = End - Start;
				float step = span / (float)Segments;
				FVector3 prev = {C.x + radius * cosf(Start), C.y + radius * sinf(Start), C.z};
				for (int i = 1; i <= Segments; ++i)
				{
					float ang = Start + step * i;
					FVector3 curr = {C.x + radius * cosf(ang), C.y + radius * sinf(ang), C.z};
					append_segment(prev, curr);
					prev = curr;
				}
			}
		};

		// 상수버퍼 업데이트 (기본값으로 리셋)
		Renderer->UpdateConstant({0.f, 0.f, 0.f}, 1.0f);
		build_ring(R, bFull);
		if (CC->GetInnerRadius() > 0.0f)
		{
			build_ring(CC->GetInnerRadius(), bFull);
		}
		if (count > 1)
		{
			Renderer->RenderLines(verts, (UINT)count);
		}
	};

	RenderConcave(m_TopBumperWall, 0.2f, 0.8f, 1.f, 1.f); // 파랑빛 상단 범퍼
	// RenderConcave(m_MainBoundary, 1.f, 0.6f, 0.1f, 1.f); // 오렌지빛 메인 바운더리
	// RenderConcave(m_LeftFlipperGuide, 0.6f, 1.f, 0.2f, 1.f); // 연두빛 좌측 가이드
	// RenderConcave(m_RightFlipperGuide, 1.f, 0.2f, 0.6f, 1.f); // 자주빛 우측 가이드
	// RenderConcave(m_LeftSideGuide, 0.8f, 0.8f, 0.8f, 1.f); // 회색 좌측 사이드
	// RenderConcave(m_RightSideGuide, 0.8f, 0.8f, 0.8f, 1.f); // 회색 우측 사이드

	// ConvexCircle (볼록 충돌체) 렌더링
	if (m_CenterBumperBall)
	{
		Renderer->UpdateConstant(m_CenterBumperBall->GetLocation(), m_CenterBumperBall->GetRadius());
		Renderer->RenderPrimitive(); // 노랑빛 중앙 범퍼 (볼록)
	}
	if (m_TopBumperBall1)
	{
		Renderer->UpdateConstant(m_TopBumperBall1->GetLocation(), m_TopBumperBall1->GetRadius());
		Renderer->RenderPrimitive();
	}
	if (m_TopBumperBall2)
	{
		Renderer->UpdateConstant(m_TopBumperBall2->GetLocation(), m_TopBumperBall2->GetRadius());
		Renderer->RenderPrimitive();
	}
	if (m_TopBumperBall3)
	{
		Renderer->UpdateConstant(m_TopBumperBall3->GetLocation(), m_TopBumperBall3->GetRadius());
		Renderer->RenderPrimitive();
	}
}

void GameScene::CollisionProcess()
{
	if (m_ActorBall == nullptr || bIsPause)
	{
		return;
	}

	HandlePadPairCollisions();
	HandleRectangleCollisions();
	HandleTriangleCollisions();
	HandleConcaveCircleCollisions();
	HandleConvexCircleCollisions();
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
	const FVector3 axisX(c, s, 0.f); // 로컬 x축이 월드에서 가리키는 방향
	const FVector3 axisY(-s, c, 0.f); // 로컬 y축이 월드에서 가리키는 방향

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
		PinBall->GetVelocity() -= normalWorld * (1.f + Rect->Restitution) * vn;

		// 충돌 시 점수 추가
		if (m_ScoreManager)
		{
			m_ScoreManager->AddCollisionScore();
		}
	}
}

void GameScene::ResolveTriangleCollision(UPinBall* PinBall, const UTriangle* Triangle)
{
	if (!PinBall || !Triangle)
	{
		return;
	}

	const float B = Triangle->Base;
	const float H = Triangle->Height;
	const float r = Triangle->Radius;
	const float ballR = PinBall->GetShape()->GetRadius();

	// 이등변 삼각형 로컬(인센터 = (0,0)) 정점 (CCW)
	// 밑변: y = -r, 꼭짓점: y = H - r
	FVector3 v0(-B * 0.5f, -r, 0.0f); // Left base
	FVector3 v1(B * 0.5f, -r, 0.0f); // Right base
	FVector3 v2(0.0f, H - r, 0.0f); // Apex

	// 인센터 기준 회전
	const float c = std::cos(Triangle->Rotation);
	const float s = std::sin(Triangle->Rotation);
	auto Rotate = [&](const FVector3& L) -> FVector3
	{
		return FVector3(L.x * c - L.y * s, L.x * s + L.y * c, 0.0f);
	};

	const FVector3 center = Triangle->Location;
	FVector3 w0 = Rotate(v0) + center;
	FVector3 w1 = Rotate(v1) + center;
	FVector3 w2 = Rotate(v2) + center;

	// 점-선분 최근접점
	auto ClosestPointOnSegment = [](const FVector3& A, const FVector3& B, const FVector3& P) -> FVector3
	{
		FVector3 AB = B - A;
		float lenSq = AB.LengthSquare();
		if (lenSq <= 1e-12f)
		{
			return A;
		}

		float t = Dot(P - A, AB) / lenSq;
		t = (t < 0.f) ? 0.f : (t > 1.f ? 1.f : t);

		return A + AB * t;
	};

	const FVector3 C = PinBall->GetLocation();

	FVector3 candidates[3];
	candidates[0] = ClosestPointOnSegment(w0, w1, C);
	candidates[1] = ClosestPointOnSegment(w1, w2, C);
	candidates[2] = ClosestPointOnSegment(w2, w0, C);

	// 최소 거리 후보 선택
	float bestDistSq = FLT_MAX;
	FVector3 closest;
	for (int i = 0; i < 3; ++i)
	{
		float dsq = (C - candidates[i]).LengthSquare();
		if (dsq < bestDistSq)
		{
			bestDistSq = dsq;
			closest = candidates[i];
		}
	}

	float dist = std::sqrtf(bestDistSq);
	if (dist > ballR)
		return; // 충돌 없음

	// 법선
	FVector3 normal;
	if (dist > 1e-6f)
	{
		normal = (C - closest) / dist;
	}
	else
	{
		// 극도로 가까움: 인센터 방향 사용
		normal = (C - center);
		if (normal.LengthSquare() < 1e-8f)
			normal = FVector3(1.f, 0.f, 0.f);
		else
			normal.Normalize();
	}

	// 침투 보정
	float penetration = ballR - dist;
	if (penetration > 0.f)
	{
		PinBall->GetLocation() += normal * penetration;
	}

	// 속도 반사 (삼각형 정적)
	float vn = Dot(PinBall->GetVelocity(), normal);
	if (vn < 0.f)
	{
		PinBall->GetVelocity() -= normal * (1.f + Triangle->Restitution) * vn;

		// 충돌 시 점수 추가
		if (m_ScoreManager)
		{
			m_ScoreManager->AddCollisionScore();
		}
	}
}

/**
 * @brief 패드 전용 충돌 처리 함수
 * @param InBall 충돌을 검사할 공 객체
 * @param InFilpper 패드 삼각형 객체
 * @param bInIsLeftPad 좌측 패드인지 여부
 */
void GameScene::ResolvePadCollision(UPinBall* InBall, const UTriangle* InFilpper, bool bInIsLeftPad)
{
	if (!InBall || !InFilpper)
	{
		return;
	}

	const float B = InFilpper->Base;
	const float H = InFilpper->Height;
	const float r = InFilpper->Radius;
	const float ballR = InBall->GetShape()->GetRadius();

	// 이등변 삼각형 로컬(인센터 = (0,0)) 정점 (CCW)
	FVector3 v0(-B * 0.5f, -r, 0.0f); // Left base
	FVector3 v1(B * 0.5f, -r, 0.0f); // Right base
	FVector3 v2(0.0f, H - r, 0.0f); // Apex

	// 인센터 기준 회전
	const float c = std::cos(InFilpper->Rotation);
	const float s = std::sin(InFilpper->Rotation);
	auto Rotate = [&](const FVector3& L) -> FVector3
	{
		return FVector3(L.x * c - L.y * s, L.x * s + L.y * c, 0.0f);
	};

	const FVector3 Center = InFilpper->Location;
	FVector3 w0 = Rotate(v0) + Center;
	FVector3 w1 = Rotate(v1) + Center;
	FVector3 w2 = Rotate(v2) + Center;

	// 점-선분 최근접점
	auto ClosestPointOnSegment = [](const FVector3& A, const FVector3& B, const FVector3& P) -> FVector3
	{
		FVector3 AB = B - A;
		float lenSq = AB.LengthSquare();
		if (lenSq <= 1e-12f)
		{
			return A;
		}

		float t = Dot(P - A, AB) / lenSq;
		t = std::clamp(t, 0.f, 1.f);

		return A + AB * t;
	};

	const FVector3 C = InBall->GetLocation();

	FVector3 Candidates[3];
	Candidates[0] = ClosestPointOnSegment(w0, w1, C); // Base
	Candidates[1] = ClosestPointOnSegment(w1, w2, C); // Right side
	Candidates[2] = ClosestPointOnSegment(w2, w0, C); // Left side

	// 최소 거리 후보 선택
	float BestDistSq = FLT_MAX;
	FVector3 Closest;
	int ClosestSegment = 0;
	for (int i = 0; i < 3; ++i)
	{
		float dsq = (C - Candidates[i]).LengthSquare();
		if (dsq < BestDistSq)
		{
			BestDistSq = dsq;
			Closest = Candidates[i];
			ClosestSegment = i;
		}
	}

	float Dist = std::sqrtf(BestDistSq);
	if (Dist > ballR)
	{
		return; // 충돌 없음
	}

	// 법선 계산
	FVector3 Normal;
	if (Dist > 1e-6f)
	{
		Normal = (C - Closest) / Dist;
	}
	else
	{
		// 극도로 가까운 경우: 인센터 방향 사용
		Normal = (C - Center);
		if (Normal.LengthSquare() < 1e-8f)
		{
			Normal = FVector3(1.f, 0.f, 0.f);
		}
		else
		{
			Normal.Normalize();
		}
	}

	// CCD 문제 해결을 위한 특수 처리: 패드와 충돌 시 위쪽으로 편향
	FInputManager* KeyManager = FInputManager::GetInstance();
	bool leftKeyPressed = KeyManager && KeyManager->IsKeyDown(EKeyInput::A);
	bool rightKeyPressed = KeyManager && KeyManager->IsKeyDown(EKeyInput::D);

	// 해당 패드의 키가 눌린 경우 강제로 위쪽 방향으로 편향
	if ((bInIsLeftPad && leftKeyPressed) || (!bInIsLeftPad && rightKeyPressed))
	{
		// 패드가 활성화된 경우: 위쪽으로 강제 편향
		if (ClosestSegment == 0) // Base 세그먼트와 충돌한 경우
		{
			// 강제로 위쪽 방향 추가
			Normal.y = max(Normal.y, 0.3f); // 최소 30% 위쪽 성분 보장
			Normal.Normalize();
		}
	}

	// 침투 보정
	float Penetration = ballR - Dist;
	if (Penetration > 0.f)
	{
		InBall->GetLocation() += Normal * Penetration;
	}

	// 속도 반사
	float vn = Dot(InBall->GetVelocity(), Normal);
	if (vn < 0.f)
	{
		InBall->GetVelocity() -= Normal * (1.f + InFilpper->Restitution) * vn;

		// 패드 키가 눌린 경우 추가 위쪽 속도 부여 (CCD 보완)
		if ((bInIsLeftPad && leftKeyPressed) || (!bInIsLeftPad && rightKeyPressed))
		{
			// 위쪽 속도 성분이 부족하면 보정
			if (InBall->GetVelocity().y < 1.0f)
			{
				InBall->GetVelocity().y += 0.5f; // 위쪽으로 추가 속도
			}
		}

		// 충돌 시 점수 추가
		if (m_ScoreManager)
		{
			m_ScoreManager->AddCollisionScore();
		}
	}
}

void GameScene::HandlePadPairCollisions()
{
	if (!IsMarkedForDeletion(m_ActorBall))
	{
		// 패드 전용 충돌 처리 사용
		ResolvePadCollision(m_ActorBall, m_PadPair->Left().GetShape(), true);
		ResolvePadCollision(m_ActorBall, m_PadPair->Right().GetShape(), false);
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

void GameScene::AddNewRectangle(FVector3 location, float rotation, float width, float height, bool autoRotation,
                                RectRotType type)
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
	NewRectangle->Restitution = OBSTACLE_RESTITUTION;

	switch (type)
	{
	case RectRotType::CLOCK:
		NewRectangle->clockwise = true;
		break;
	case RectRotType::REVCLOCK:
		NewRectangle->clockwise = false;
		break;
	default:
		break;
	}

	FSceneManager& FCM = FSceneManager::GetInstance();
	Scene* currentScene = FCM.GetCurrentScene();
	// Add to the vector
	currentScene->GetScenePrimitives()->push_back(NewRectangle);
}

void GameScene::AddNewTriangle(FVector3 location, float rotation, float base, float height)
{
	UTriangle* NewTriangle = new UTriangle();
	NewTriangle->Location = location;
	NewTriangle->Rotation = rotation;
	NewTriangle->Base = base;
	NewTriangle->Height = height;
	NewTriangle->Restitution = OBSTACLE_RESTITUTION;

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

		// m_ActorBall이 제거된 상태일 것이므로 Dangling Pointer 제거
		// XXX(KHJ): Shared Pointer를 사용했더라면...
		m_ActorBall = nullptr;
	}

	// 삭제 목록 비우기
	m_BallsToDelete.clear();
	DEBUG_PRINT("[Delayed Deletion] Deletion process completed\n");

	// 종료 조건을 만족했기 때문에 Delayed 처리 이후 Pause 추가
	SetPause();
}

bool GameScene::IsMarkedForDeletion(UPinBall* InBall) const
{
	return std::find(m_BallsToDelete.begin(), m_BallsToDelete.end(), InBall) != m_BallsToDelete.end();
}

/**
 * @brief 공과 오목 원형 충돌체간의 충돌을 감지하고 처리하는 함수
 * @param Ball 충돌을 검사할 공 객체
 * @param ConcaveCircle 충돌을 검사할 오목 원형 충돌체 객체
 */
void GameScene::ResolveBallConcaveCircle(UPinBall* Ball, const UConcaveCircle* ConcaveCircle)
{
	if (!Ball || !ConcaveCircle)
		return;

	FVector3 BallPos = Ball->GetLocation();
	float BallRadius = Ball->GetShape()->GetRadius();

	// 공이 오목 원의 내부에 있는지 확인
	if (!ConcaveCircle->IsPointInside(BallPos))
	{
		return; // 밖에 있으면 충돌하지 않음
	}

	DEBUG_PRINT("[ConcaveCircle] Ball is inside concave circle!\n");

	// 공의 중심에서 원의 중심으로의 벡터
	FVector3 Diff = BallPos - ConcaveCircle->GetCenter();
	float DistFromCenter = Diff.Length();

	// 공이 원의 경계와 충돌하는지 확인 (공의 반지름을 고려)
	float CollisionRadius = ConcaveCircle->GetRadius() - BallRadius;

	DEBUG_PRINT("[ConcaveCircle] DistFromCenter: %.3f, CollisionRadius: %.3f\n", DistFromCenter, CollisionRadius);

	if (DistFromCenter >= CollisionRadius)
	{
		DEBUG_PRINT("[ConcaveCircle] COLLISION DETECTED! Resolving...\n");

		// 충돌 발생 - 공을 원의 안쪽으로 밀어냄
		FVector3 Normal;
		if (DistFromCenter > 0.00001f)
		{
			Normal = Diff / DistFromCenter; // 중심에서 바깥쪽으로의 단위벡터
		}
		else
		{
			// 공이 정확히 중심에 있는 경우 임의의 방향 설정
			Normal = FVector3(1.0f, 0.0f, 0.0f);
		}

		// 침투 깊이 계산 (음수여야 함 - 안쪽으로 밀어내기 위해)
		float Penetration = CollisionRadius - DistFromCenter;
		DEBUG_PRINT("[ConcaveCircle] Penetration: %.3f\n", Penetration);

		if (Penetration > 0.0f)
		{
			// 공을 안쪽으로 밀어냄 (Normal의 반대 방향)
			Ball->GetLocation() -= Normal * Penetration;
			DEBUG_PRINT("[ConcaveCircle] Ball position corrected\n");
		}

		// 속도 반사 처리
		float Vn = Dot(Ball->GetVelocity(), Normal);
		if (Vn > 0.f) // 밖으로 향하는 속도만 반사
		{
			float Restitution = 1.0f;
			Ball->GetVelocity() -= Normal * (1.f + Restitution) * Vn;
			DEBUG_PRINT("[ConcaveCircle] Ball velocity reflected\n");

			// 충돌 시 점수 추가
			if (m_ScoreManager)
			{
				m_ScoreManager->AddCollisionScore();
			}
		}
	}
}

void GameScene::HandleConcaveCircleCollisions()
{
	for (int i = 0; i < static_cast<int>(m_PrimitiveList->size()); ++i)
	{
		UPinBall* Ball = dynamic_cast<UPinBall*>((*m_PrimitiveList)[i]);
		if (Ball != nullptr && !IsMarkedForDeletion(Ball))
		{
			// 개별 ConcaveCircle 멤버들과의 충돌 처리
			if (m_TopBumperWall)
			{
				ResolveBallConcaveCircle(Ball, m_TopBumperWall);
			}
			// if (m_MainBoundary)
			// {
			// 	ResolveBallConcaveCircle(Ball, m_MainBoundary);
			// }
			// if (m_LeftFlipperGuide)
			// {
			// 	ResolveBallConcaveCircle(Ball, m_LeftFlipperGuide);
			// }
			// if (m_RightFlipperGuide)
			// {
			// 	ResolveBallConcaveCircle(Ball, m_RightFlipperGuide);
			// }
			// if (m_LeftSideGuide)
			// {
			// 	ResolveBallConcaveCircle(Ball, m_LeftSideGuide);
			// }
			// if (m_RightSideGuide)
			// {
			// 	ResolveBallConcaveCircle(Ball, m_RightSideGuide);
			// }
		}
	}
}

/**
 * @brief 공과 볼록 원형 충돌체간의 충돌을 감지하고 처리하는 함수
 * @param Ball 충돌을 검사할 공 객체
 * @param ConvexCircle 충돌을 검사할 볼록 원형 충돌체 객체
 */
void GameScene::ResolveBallConvexCircle(UPinBall* Ball, const UBall* ConvexCircle)
{
	if (!Ball || !ConvexCircle)
		return;

	FVector3 BallPos = Ball->GetLocation();
	FVector3 ConvexPos = ConvexCircle->GetLocation();
	float BallRadius = Ball->GetShape()->GetRadius();
	float ConvexRadius = ConvexCircle->GetRadius();

	// 두 원의 중심 간 거리 계산
	FVector3 Diff = BallPos - ConvexPos;
	float DistFromCenter = Diff.Length();
	float TotalRadius = BallRadius + ConvexRadius;

	// 충돌 감지: 중심 간 거리가 두 반지름의 합보다 작을 때
	if (DistFromCenter >= TotalRadius)
	{
		return; // 충돌하지 않음
	}

	DEBUG_PRINT("[ConvexCircle] Ball colliding with convex circle!\n");

	// 충돌 발생 - 공을 볼록 충돌체에서 밀어냄
	FVector3 Normal;
	if (DistFromCenter > 0.00001f)
	{
		Normal = Diff / DistFromCenter; // 볼록 충돌체에서 공 방향으로의 단위벡터
	}
	else
	{
		// 두 원이 정전히 겹친 경우 임의의 방향 설정
		Normal = FVector3(1.0f, 0.0f, 0.0f);
	}

	// 침투 깊이 계산
	float Penetration = TotalRadius - DistFromCenter;
	DEBUG_PRINT("[ConvexCircle] Penetration: %.3f\n", Penetration);

	if (Penetration > 0.0f)
	{
		// 공을 바깥쪽으로 밀어냄
		Ball->GetLocation() += Normal * Penetration;
		DEBUG_PRINT("[ConvexCircle] Ball position corrected\n");
	}

	// 속도 반사 처리
	float Vn = Dot(Ball->GetVelocity(), Normal);
	if (Vn < 0.f) // 볼록 충돌체로 향하는 속도만 반사
	{
		// float Restitution = 1.2f; // 볼록 충돌체는 약간 더 강하게 반사
		Ball->GetVelocity() -= Normal * (1.f + OBSTACLE_RESTITUTION) * Vn;
		DEBUG_PRINT("[ConvexCircle] Ball velocity reflected with restitution %.1f\n", OBSTACLE_RESTITUTION);

		// 충돌 시 점수 추가
		if (m_ScoreManager)
		{
			m_ScoreManager->AddCollisionScore();
		}
	}
}

void GameScene::HandleConvexCircleCollisions()
{
	for (int i = 0; i < static_cast<int>(m_PrimitiveList->size()); ++i)
	{
		UPinBall* Ball = dynamic_cast<UPinBall*>((*m_PrimitiveList)[i]);
		if (Ball != nullptr && !IsMarkedForDeletion(Ball))
		{
			// 개별 ConvexCircle 멤버들과의 충돌 처리
			if (m_CenterBumperBall)
			{
				ResolveBallConvexCircle(Ball, m_CenterBumperBall);
			}
			if (m_TopBumperBall1)
			{
				ResolveBallConvexCircle(Ball, m_TopBumperBall1);
			}
			if (m_TopBumperBall2)
			{
				ResolveBallConvexCircle(Ball, m_TopBumperBall2);
			}
			if (m_TopBumperBall3)
			{
				ResolveBallConvexCircle(Ball, m_TopBumperBall3);
			}
		}
	}
}
