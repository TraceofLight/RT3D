#include "pch.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

#include "Core/Public/Primitive.h"
#include "Asset/Sphere.h"
#include "Asset/Rectangle.h"
#include "Asset/Triangle.h"
#include "Actor/Public/PinBall.h"
#include "Mesh/Public/URectangle.h"
#include "Mesh/Public/UTriangle.h"
#include "Manager/Public/ImGuiManager.h"
#include "Manager/Public/InputManager.h"
#include "Manager/Public/ScoreManager.h"
#include "Render/Public/Renderer.h"
#include "Manager/Public/UIManager.h"
#include "Manager/Public/SceneManager.h"
#include "Scene/Public/GameScene.h"
#include "Scene/Public/LobbyScene.h"
#include "Actor/Public/Shooter.h"
#include "Mesh/Public/UBall.h"

static void HandleCollisions();
static void HandleBallRectangleCollisions();
static void ResolveBallRectangle(UPinBall* Ball, const URectangle* Rect);
static void HandleBallTriangleCollisions();
static void ResolveBallTriangle(UPinBall* Ball, const UTriangle* Triangle);

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// Static
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static HWND GlobalWindowHandle = nullptr;

// Global variables definition
static std::vector<UPinBall*> PinBalls;

void RenderProcess(const URenderer& InRenderer, const UShooter* Shooter = nullptr);
void InputProcess(bool& InExitFlag);

/**
 * @brief 매 프레임 반복되는 Logic을 처리하는 함수
 */
static void MainLoop(URenderer& InRenderer)
{
	FTimeManager* TimeManager = FTimeManager::GetInstance();
	FInputManager* KeyManager = FInputManager::GetInstance();
	FScoreManager* ScoreManager = FScoreManager::GetInstance();

	UShooter Shooter;
	Shooter.SetLocation({0.3f, -0.8f, 0.0f});

	bool bIsExit = false;
	while (!bIsExit)
	{
		// Update TimeManager
		TimeManager->Update();

		// 윈도우 메시지 처리
		MSG Message;
		while (PeekMessage(&Message, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&Message);
			DispatchMessage(&Message);

			if (Message.message == WM_QUIT)
			{
				bIsExit = true;
				break;
			}
		}

		if (bIsExit)
		{
			break;
		}

		// KeyManager 업데이트
		KeyManager->Update();
		InputProcess(bIsExit);

		// === 게임 로직 업데이트 ===
		// Charge
		if (KeyManager->IsKeyDown(EKeyInput::Space))
		{
			Shooter.Charging();
			OutputDebugStringA("[MAINLOOP] Shooter Charging...\n");
		}
		// Shoot
		else if (KeyManager->IsKeyReleased(EKeyInput::Space))
		{
			Shooter.Shoot();
			OutputDebugStringA("[MAINLOOP] Shooter Fire!\n");
		}

		// SceneManager에서 현재 씬의 Primitives 가져오기
		FSceneManager& SceneManager = FSceneManager::GetInstance();
		vector<UPrimitive*> ScenePrimitives = SceneManager.GetAllScenePrimivites();

		// Triangle 회전 업데이트
		GTriangle.UpdateRotation(KeyManager, TimeManager->GetDeltaTime());

		// 공들의 물리 시뮬레이션 업데이트 (SceneManager에서 가져온 ball들)
		for (UPrimitive* Primitive : ScenePrimitives)
		{
			UPinBall* Ball = static_cast<UPinBall*>(Primitive);
			if (Ball)
			{
				Ball->Move(); // PinBall의 물리 업데이트 처리
			}
		}

		// 사각형 물리 업데이트
		GRectangle.Move(); // 이 함수 내부에서도 DT 매크로 사용 가능

		// === 충돌 처리 ===
		HandleCollisions();
		HandleBallRectangleCollisions();
		HandleBallTriangleCollisions();

		// Rendering
		RenderProcess(InRenderer, &Shooter);
	}
}

void RenderProcess(const URenderer& InRenderer, const UShooter* Shooter)
{
	InRenderer.Prepare();
	InRenderer.PrepareShader();

	// SceneManager에서 공들 가져와서 렌더링
	FSceneManager& SceneManager = FSceneManager::GetInstance();
	vector<UPrimitive*> ScenePrimitives = SceneManager.GetAllScenePrimivites();

	for (UPrimitive* Primitive : ScenePrimitives)
	{
		UPinBall* Ball = static_cast<UPinBall*>(Primitive);
		if (Ball)
		{
			// PinBall 렌더링
			InRenderer.UpdateConstant(Ball->GetLocation(), Ball->GetShape()->GetRadius());
			InRenderer.RenderPrimitive();
		}
	}

	// 사각형 렌더링
	InRenderer.UpdateConstantForRectangle(GRectangle.Location, GRectangle.Width, GRectangle.Height);
	InRenderer.RenderRectangle();

	// Triangle Render
	InRenderer.UpdateConstantForTriangle(GTriangle.Location, GTriangle.Base, GTriangle.Height, GTriangle.Rotation,
	                                     GTriangle.Radius);
	InRenderer.RenderTriangle();

	// Shooter 렌더링 (사각형으로 표시)
	if (Shooter)
	{
		// Shooter를 작은 사각형으로 렌더링
		float ShooterWidth = 0.05f;
		float ShooterHeight = 0.2f;
		InRenderer.UpdateConstantForRectangle(Shooter->GetLocation(), ShooterWidth, ShooterHeight);
		InRenderer.RenderRectangle();
	}

	// ImGui 렌더링 (TimeManager 정보 표시 가능)
	FImGuiManager::RenderImGui();

	// 백버퍼 스왑
	InRenderer.SwapBuffer();
}

void InputProcess(bool& InExitFlag)
{
	FInputManager* KeyManager = FInputManager::GetInstance();

	// ESC키로 종료
	if (KeyManager->IsKeyPressed(EKeyInput::Esc))
	{
		PostMessage(GlobalWindowHandle, WM_CLOSE, 0, 0);
		InExitFlag = true;
		return;
	}

	if (KeyManager->IsKeyPressed(EKeyInput::Delete))
	{
		if (!PinBalls.empty())
		{
			int indexToRemove = rand() % PinBalls.size();
			delete PinBalls[indexToRemove];
			PinBalls.erase(PinBalls.begin() + indexToRemove);
		}
	}
}

static void InitEngine(HWND InWindowHandle, URenderer& InRenderer)
{
	// Renderer Initialize
	InRenderer.TotalInit(InWindowHandle);

	// Sphere 버텍스 버퍼
	UINT numVerticesSphere = sizeof(sphere_vertices) / sizeof(FVertexSimple);
	InRenderer.vertexBufferSphere = InRenderer.CreateVertexBuffer(
		sphere_vertices, sizeof(sphere_vertices));
	InRenderer.numVerticesSphere = numVerticesSphere;

	// Rectangle 버텍스/인덱스 버퍼
	InRenderer.vertexBufferRectangle = InRenderer.CreateVertexBuffer(
		rectangle_vertices, sizeof(rectangle_vertices));
	InRenderer.indexBufferRectangle = InRenderer.CreateIndexBuffer(
		rectangle_indices, sizeof(rectangle_indices));
	InRenderer.numIndicesRectangle = _countof(rectangle_indices);

	// Triangle 버텍스 버퍼
	InRenderer.vertexBufferTriangle = InRenderer.CreateVertexBuffer(
		triangle_vertices, sizeof(triangle_vertices));

	// Initialize Managers
	FTimeManager::GetInstance();
	FInputManager::GetInstance();
	UIManager::GetInstance();

	// 씨너 등록
	FSceneManager& SceneManager = FSceneManager::GetInstance();
	SceneManager.RegisterScene("LOBBY", new LobbyScene());
	SceneManager.RegisterScene("GAME", new GameScene());

	// 게임 씨너를 기본으로 로드
	SceneManager.LoadScene("GAME");
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	// 난수 시드 초기화
	srand(static_cast<unsigned int>(GetTickCount()));

	// 윈도우 클래스 이름
	WCHAR WindowClass[] = L"JungleWindowClass";

	// 윈도우 타이틀바에 표시될 이름
	WCHAR Title[] = L"Game Tech Lab";

	// 각종 메시지를 처리할 함수인 WndProc의 함수 포인터를 WindowClass 구조체에 넣는다.
	WNDCLASSW wndclass = {0, WndProc, 0, 0, 0, 0, 0, 0, 0, WindowClass};

	// 윈도우 클래스 등록
	RegisterClassW(&wndclass);

	// 1024 x 1024 크기에 윈도우 생성
	HWND WindowHandle = CreateWindowExW(0, WindowClass, Title,
	                                    WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
	                                    CW_USEDEFAULT, CW_USEDEFAULT, 1024, 1024,
	                                    nullptr, nullptr, hInstance, nullptr);

	// Make Window Handle Global
	GlobalWindowHandle = WindowHandle;
	// Make Renderer

	InitEngine(WindowHandle, *(URenderer::GetInstance()));
	MainLoop(*URenderer::GetInstance());

	// Release PinBalls
	for (UPinBall* Ball : PinBalls)
	{
		delete Ball;
	}
	PinBalls.clear();

	FInputManager* KeyManager = FInputManager::GetInstance();
	if (KeyManager)
	{
		delete KeyManager;
	}

	FTimeManager* TimeManager = FTimeManager::GetInstance();
	if (TimeManager)
	{
		delete TimeManager;
	}

	URenderer::GetInstance()->TotalShutDown();

	return 0;
}

/*************************/
/** 이하 Common Function **/
/*************************/

/**
 * @brief 각종 입력을 처리하는 함수
 */
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
	{
		// ImGui가 마우스 이벤트를 사용했다면, 게임 로직에서는 처리하지 않아야 한다.
		if (ImGui::GetIO().WantCaptureMouse)
		{
			return true;
		}
	}

	FInputManager* KeyManager = FInputManager::GetInstance();
	if (KeyManager)
	{
		KeyManager->ProcessKeyMessage(message, wParam, lParam);
	}

	// Destroy 제외한 나머지 입력은 InputManager에서 처리
	switch (message)
	{
	case WM_DESTROY:
		// Signal that the app should quit
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

/**
 * @brief 공들 간의 충돌을 감지하고 처리하는 함수
 */
void HandleCollisions()
{
	// TODO(KHJ): PinBall의 충돌 처리 로직 구현, 여기가 아닌 매니저 측에서의 관리 필요
}

/**
 * @brief 공과 사각형 간의 충돌을 감지하고 처리하는 함수
 * @param Ball 충돌을 검사할 공 객체
 * @param Rect 충돌을 검사할 사각형 객체
 */
void ResolveBallRectangle(UPinBall* Ball, const URectangle* Rect)
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

void HandleBallTriangleCollisions()
{
	// PinBall vector 사용하여 삼각형 충돌 처리
	// TODO: PinBall의 삼각형 충돌 처리 로직 구현
}

void ResolveBallTriangle(UPinBall* Ball, const UTriangle* Triangle)
{
	if (!Ball || !Triangle)
		return;

	// 삼각형(직각, Incenter 기준 회전) 로컬 꼭짓점 구성
	const float a = Triangle->Base; // X방향 직각변
	const float b = Triangle->Height; // Y방향 직각변
	const float r = Triangle->Radius; // Inradius (이미 UTriangle 내부에서 계산됨)

	// 로컬(Incenter = 원점) 좌표: (0,0)-(0,b)-(a,0)에서 (r,r)만큼 이동 제거
	FVector3 v0(-r, -r, 0.0f); // 직각 꼭짓점
	FVector3 v1(-r, b - r, 0.0f); // +Y
	FVector3 v2(a - r, -r, 0.0f); // +X

	// 회전
	const float c = std::cos(Triangle->Rotation);
	const float s = std::sin(Triangle->Rotation);

	auto Rotate = [&](const FVector3& L) -> FVector3
	{
		return FVector3(L.x * c - L.y * s, L.x * s + L.y * c, 0.0f);
	};

	// 월드 변환 (Incenter = Triangle->Location)
	FVector3 w0 = Rotate(v0) + Triangle->Location;
	FVector3 w1 = Rotate(v1) + Triangle->Location;
	FVector3 w2 = Rotate(v2) + Triangle->Location;

	// 가장 가까운 점 찾기 (원-삼각형 최소 거리)
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

	// 가장 가까운 점 선택
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
	// 충돌 검사 (원-삼각형)
	if (dist > Ball->GetShape()->GetRadius())
		return; // 충돌 없음

	// 법선 계산
	FVector3 normal;
	if (dist > 1e-6f)
	{
		normal = (C - closest) / dist;
	}
	else
	{
		// 중심이 거의 겹친 경우: 삼각형 인센터 방향 사용
		normal = (C - Triangle->Location);
		if (normal.LengthSquare() < 1e-8f)
			normal = FVector3(1.f, 0.f, 0.f);
		else
			normal.Normalize();
	}

	// 침투 보정
	float penetration = Ball->GetShape()->GetRadius() - dist;
	if (penetration > 0.f)
	{
		Ball->GetLocation() += normal * penetration;
	}

	// 속도 반사 (삼각형은 정적)
	float vn = Dot(Ball->GetVelocity(), normal);
	if (vn < 0.f)
	{
		const float Restitution = 1.0f; // 필요 시 조정
		Ball->GetVelocity() -= normal * (1.f + Restitution) * vn;
	}
}

/**
 * @brief 공과 사각형 간의 충돌을 감지하고 처리하는 함수
 */
void HandleBallRectangleCollisions()
{
	// SceneManager에서 공들 가져와서 사각형 충돌 처리
	FSceneManager& SceneManager = FSceneManager::GetInstance();
	vector<UPrimitive*> ScenePrimitives = SceneManager.GetAllScenePrimivites();

	for (UPrimitive* Primitive : ScenePrimitives)
	{
		UPinBall* Ball = static_cast<UPinBall*>(Primitive);
		if (Ball)
		{
			ResolveBallRectangle(Ball, &GRectangle);
		}
	}
}
