#include "pch.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

#include "Core/Public/Primitive.h"
#include "Asset/Sphere.h"
#include "Asset/Rectangle.h"
#include "Asset/Triangle.h"
#include "Mesh/Public/UBall.h"
#include "Mesh/Public/URectangle.h"
#include "Mesh/Public/UTriangle.h"
#include "Manager/Public/ImGuiManager.h"
#include "Manager/Public/InputManager.h"
#include "Manager/Public/ScoreManager.h"
#include "Render/Public/Renderer.h"
#include "Scenes/SceneManager.h"
#include "Scenes/GameScene.h"

static void HandleMouseClick(int InX, int InY, bool InIsLeftClick);
static void RemoveSpecificBall(int IndexToRemove);
static void SetGravityCenter(int IndexToSet);
static void HandleCollisions();
static void HandleBallRectangleCollisions();
static void ResolveBallRectangle(UBall* Ball, const URectangle* Rect);
static void HandleBallTriangleCollisions();
static void ResolveBallTriangle(UBall* Ball, const UTriangle* Triangle);

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static void AddNewBall();
static void RemoveRandomBall();

// Static
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static HWND GlobalWindowHandle = nullptr;

// Global variables definition
int TotalPrimitives = 0;
UPrimitive** PrimitiveList = nullptr;
static int frameCount = 0;
static const int minFramesBeforeFirstBall = 5;

void RenderProcess(const URenderer& InRenderer);
void InputProcess(bool& InExitFlag);

/**
 * @brief 매 프레임 반복되는 Logic을 처리하는 함수
 */
static void MainLoop(URenderer& InRenderer)
{
	FTimeManager* TimeManager = FTimeManager::GetInstance();
	FInputManager* KeyManager = FInputManager::GetInstance();
	FScoreManager* ScoreManager = FScoreManager::GetInstance();

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
		// 시간 기반 공 생성 제어 (프레임 기반에서 시간 기반으로 변경)
		if (TimeManager->GetGameTime())
		{
			// 공 개수 자동 조절
			if (TotalPrimitives < UBall::TotalNumBalls)
			{
				AddNewBall();
				OutputDebugStringA("[MAINLOOP] Ball added\n");
			}
			else if (TotalPrimitives > UBall::TotalNumBalls)
			{
				RemoveRandomBall();
				OutputDebugStringA("[MAINLOOP] Ball removed\n");
			}
		}

		// 공들의 물리 시뮬레이션 업데이트
		for (int i = 0; i < TotalPrimitives; ++i)
		{
			UBall* Ball = static_cast<UBall*>(PrimitiveList[i]);
			Ball->Move(); // 이 함수 내부에서 DT 매크로 사용 가능
		}

		// 사각형 물리 업데이트
		GRectangle.Move(); // 이 함수 내부에서도 DT 매크로 사용 가능

		// === 충돌 처리 ===
		HandleCollisions();
		HandleBallRectangleCollisions();
		HandleBallTriangleCollisions();

		// Rendering
		RenderProcess(InRenderer);
	}
}

void RenderProcess(const URenderer& InRenderer)
{
	InRenderer.Prepare();
	InRenderer.PrepareShader();

	// 공들 렌더링
	for (int i = 0; i < TotalPrimitives; ++i)
	{
		UBall* Ball = static_cast<UBall*>(PrimitiveList[i]);
		InRenderer.UpdateConstant(Ball->Location, Ball->Radius);
		InRenderer.RenderPrimitive();
	}

	// 사각형 렌더링
	InRenderer.UpdateConstantForRectangle(GRectangle.Location, GRectangle.Width, GRectangle.Height);
	InRenderer.RenderRectangle();

	// Triangle Render
	InRenderer.UpdateConstantForTriangle(GTriangle.Location, GTriangle.Base, GTriangle.Height, GTriangle.Rotation, GTriangle.Radius);
	InRenderer.RenderTriangle();

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

	// Space키로 공 추가
	if (KeyManager->IsKeyPressed(EKeyInput::Space))
	{
		AddNewBall();
	}

	// Delete키로 공 제거
	if (KeyManager->IsKeyPressed(EKeyInput::Delete))
	{
		RemoveRandomBall();
	}

	// 마우스 클릭 처리
	if (KeyManager->IsKeyDown(EKeyInput::MouseLeft))
	{
		// 마우스 위치 가져오기
		FVector2 MousePosition = KeyManager->GetMousePosition();
		HandleMouseClick(static_cast<int>(MousePosition.x), static_cast<int>(MousePosition.y), true);
	}

	if (KeyManager->IsKeyDown(EKeyInput::MouseRight))
	{
		// 마우스 위치 가져오기
		FVector2 MousePosition = KeyManager->GetMousePosition();
		HandleMouseClick(static_cast<int>(MousePosition.x), static_cast<int>(MousePosition.y), false);
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
}

static void InitSceneTemp()
{
	// step 1. spawn a rectangle
	// URectangle* Rectangle = new URectangle();
	// Rectangle->Location = FVector3(1.0f, 1.0f, 1.0f);
	// PrimitiveList[0] = Rectangle;
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
	URenderer Renderer;

	InitEngine(WindowHandle, Renderer);

	// Scene* gameScene = new GameScene();
	// SceneManager::GetInstance().RegisterScene("GAME SCENE", gameScene);
	// SceneManager::GetInstance().LoadScene("GAME SCENE");
	// InitSceneTemp();

	MainLoop(Renderer);

	// Release Balls
	for (int i = 0; i < TotalPrimitives; ++i)
	{
		delete PrimitiveList[i];
	}

	// Release & Remove Dangling Pointer
	if (PrimitiveList)
	{
		delete[] PrimitiveList;
		PrimitiveList = nullptr;
	}

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

	Renderer.TotalShutDown();

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
 * @brief 새로운 공을 추가하는 함수
 */
void AddNewBall()
{
	// Make New List
	UPrimitive** NewList = new UPrimitive*[TotalPrimitives + 1];

	// Copy
	for (int i = 0; i < TotalPrimitives; ++i)
	{
		NewList[i] = PrimitiveList[i];
	}

	// Add New Ball
	NewList[TotalPrimitives] = new UBall();

	// Release
	if (PrimitiveList != nullptr)
	{
		delete[] PrimitiveList;
	}

	// Swap List
	PrimitiveList = NewList;

	++TotalPrimitives;
}

/**
 * @brief 임의의 공을 제거하는 함수
 */
void RemoveRandomBall()
{
	if (TotalPrimitives <= 0)
	{
		return;
	}

	// Select Index
	int IndexToRemove = rand() % TotalPrimitives;

	// If Gravity Center, Make Null First
	if (PrimitiveList[IndexToRemove] == GravityCenterBall)
	{
		GravityCenterBall = nullptr;
	}

	// Remove Object
	delete PrimitiveList[IndexToRemove];

	// Make New List
	UPrimitive** NewList = nullptr;
	if (TotalPrimitives - 1 > 0)
	{
		NewList = new UPrimitive*[TotalPrimitives - 1];
	}

	// Copy
	int NewIndex = 0;
	for (int i = 0; i < TotalPrimitives; ++i)
	{
		if (i == IndexToRemove)
		{
			continue;
		}
		NewList[NewIndex] = PrimitiveList[i];
		NewIndex++;
	}

	// Release
	delete[] PrimitiveList;

	// Swap List
	PrimitiveList = NewList;

	--TotalPrimitives;
}

/**
 * @brief 공들 간의 충돌을 감지하고 처리하는 함수
 */
void HandleCollisions()
{
	for (int i = 0; i < TotalPrimitives; ++i)
	{
		for (int j = i + 1; j < TotalPrimitives; ++j)
		{
			UBall* Ball1 = static_cast<UBall*>(PrimitiveList[i]);
			UBall* Ball2 = static_cast<UBall*>(PrimitiveList[j]);

			FVector3 Delta = Ball1->Location - Ball2->Location;
			float DistanceSq = Delta.LengthSquare();
			float CombinedRadius = Ball1->Radius + Ball2->Radius;

			if (DistanceSq < CombinedRadius * CombinedRadius && DistanceSq > 0.0f)
			{
				// 충돌 발생
				float Distance = sqrtf(DistanceSq);
				FVector3 Normal = Delta / Distance;

				// 겹침 해결
				float Overlap = 0.5f * (CombinedRadius - Distance);
				Ball1->Location += Normal * Overlap;
				Ball2->Location -= Normal * Overlap;

				// 탄성 충돌 계산
				FVector3 relativeVelocity = Ball1->Velocity - Ball2->Velocity;
				float VelocityAlongNormal = Dot(relativeVelocity, Normal);

				if (VelocityAlongNormal < 0)
				{
					float Restitution = 1.0f; // 완전 탄성 충돌
					float ImpulseScalar = -(1.0f + Restitution) * VelocityAlongNormal;
					ImpulseScalar /= (1.0f / Ball1->Mass) + (1.0f / Ball2->Mass);

					FVector3 impulse = Normal * ImpulseScalar;
					Ball1->Velocity += impulse * (1.0f / Ball1->Mass);
					Ball2->Velocity -= impulse * (1.0f / Ball2->Mass);
				}
			}
		}
	}
}

/**
 * @brief 공과 사각형 간의 충돌을 감지하고 처리하는 함수
 * @param Ball 충돌을 검사할 공 객체
 * @param Rect 충돌을 검사할 사각형 객체
 */
void ResolveBallRectangle(UBall* Ball, const URectangle* Rect)
{
	float HalfW = Rect->Width * 0.5f;
	float HalfH = Rect->Height * 0.5f;

	// 볼 중심에서 사각형 중심으로의 벡터 (사각형 로컬 좌표)
	FVector3 Delta = Ball->Location - Rect->Location;

	// 사각형 안에서 가장 가까운 점 (로컬)
	float ClampedX = Clamp(Delta.x, -HalfW, HalfW);
	float ClampedY = Clamp(Delta.y, -HalfH, HalfH);

	// 월드 좌표의 가장 가까운 점
	FVector3 Closest(Rect->Location.x + ClampedX,
	                 Rect->Location.y + ClampedY,
	                 Rect->Location.z);

	FVector3 Diff = Ball->Location - Closest;
	float DistSq = Diff.LengthSquare();
	float Radius = Ball->Radius;

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
	Ball->Location += Normal * Penetration;

	// 속도 반사
	float Vn = Dot(Ball->Velocity, Normal);
	if (Vn < 0.f)
	{
		float Restitution = 1.0f; // 필요시 조정
		Ball->Velocity -= Normal * (1.f + Restitution) * Vn;
	}
}

void HandleBallTriangleCollisions()
{
	for (int i = 0; i < TotalPrimitives; ++i)
	{
		UBall* Ball = static_cast<UBall*>(PrimitiveList[i]);
		ResolveBallTriangle(Ball, &GTriangle);
	}
}

void ResolveBallTriangle(UBall* Ball, const UTriangle* Triangle)
{
	if (!Ball || !Triangle)
		return;

	// 삼각형(직각, Incenter 기준 회전) 로컬 꼭짓점 구성
	const float a = Triangle->Base;    // X방향 직각변
	const float b = Triangle->Height;  // Y방향 직각변
	const float r = Triangle->Radius;  // Inradius (이미 UTriangle 내부에서 계산됨)

	// 로컬(Incenter = 원점) 좌표: (0,0)-(0,b)-(a,0)에서 (r,r)만큼 이동 제거
	FVector3 v0(-r, -r, 0.0f);      // 직각 꼭짓점
	FVector3 v1(-r, b - r, 0.0f);      // +Y
	FVector3 v2(a - r, -r, 0.0f);      // +X

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

	const FVector3 C = Ball->Location;

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
	if (dist > Ball->Radius)
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
	float penetration = Ball->Radius - dist;
	if (penetration > 0.f)
	{
		Ball->Location += normal * penetration;
	}

	// 속도 반사 (삼각형은 정적)
	float vn = Dot(Ball->Velocity, normal);
	if (vn < 0.f)
	{
		const float Restitution = 1.0f; // 필요 시 조정
		Ball->Velocity -= normal * (1.f + Restitution) * vn;
	}
}

/**
 * @brief 공과 사각형 간의 충돌을 감지하고 처리하는 함수
 */
void HandleBallRectangleCollisions()
{
	for (int i = 0; i < TotalPrimitives; ++i)
	{
		UBall* Ball = static_cast<UBall*>(PrimitiveList[i]);
		ResolveBallRectangle(Ball, &GRectangle);
	}
}

/**
 * @brief 특정 index의 공을 제거하는 함수
 * @param IndexToRemove 제거할 공의 index
 */
void RemoveSpecificBall(int IndexToRemove)
{
	if (TotalPrimitives <= 0 || IndexToRemove < 0 || IndexToRemove >= TotalPrimitives)
	{
		return;
	}

	// Make GravityCenter Null If Selected
	if (PrimitiveList[IndexToRemove] == GravityCenterBall)
	{
		GravityCenterBall = nullptr;
	}

	// Release Object
	delete PrimitiveList[IndexToRemove];

	// Make New List
	UPrimitive** NewList = nullptr;
	if (TotalPrimitives - 1 > 0)
	{
		NewList = new UPrimitive*[TotalPrimitives - 1];
	}

	// Copy
	int NewIndex = 0;
	for (int i = 0; i < TotalPrimitives; ++i)
	{
		if (i == IndexToRemove)
		{
			continue;
		}
		NewList[NewIndex] = PrimitiveList[i];
		NewIndex++;
	}

	// Release Array
	delete[] PrimitiveList;

	// Swap List
	PrimitiveList = NewList;

	// Count Refresh
	--TotalPrimitives;
	--UBall::TotalNumBalls;

	assert(UBall::TotalNumBalls >= 0);
}

/**
 * @brief 특정 공을 중력 중심으로 설정하거나 해제하는 함수
 * @param IndexToSet 중력 중심으로 설정할 공의 인덱스
 */
void SetGravityCenter(int IndexToSet)
{
	if (IndexToSet < 0 || IndexToSet >= TotalPrimitives)
	{
		return;
	}

	UBall* SelectedBall = static_cast<UBall*>(PrimitiveList[IndexToSet]);

	// 이미 중력 중심으로 설정된 공을 다시 클릭하면 중력 효과를 해제
	if (GravityCenterBall == SelectedBall)
	{
		GravityCenterBall = nullptr;
	}
	else
	{
		// 새로운 공을 중력 중심으로 설정, 중력 중심이 된 공은 정지
		GravityCenterBall = SelectedBall;
		GravityCenterBall->Velocity = FVector3(0.f, 0.f, 0.f);
	}
}

/**
 * @brief 마우스 클릭 이벤트를 받아 공 선택 및 관련 로직을 처리하는 함수
 * @param InX 마우스 x 좌표 (스크린 좌표)
 * @param InY 마우스 y 좌표 (스크린 좌표)
 * @param InIsLeftClick 왼쪽 클릭 여부
 */
void HandleMouseClick(int InX, int InY, bool InIsLeftClick)
{
	if (!GlobalWindowHandle)
	{
		return;
	}

	// Get Window Size
	RECT ClientRect;
	GetClientRect(GlobalWindowHandle, &ClientRect);
	float ClientWidth = ClientRect.right - ClientRect.left;
	float ClientHeight = ClientRect.bottom - ClientRect.top;

	// NDC Convert
	float ndc_x = (static_cast<float>(InX) / ClientWidth) * 2.0f - 1.0f;
	float ndc_y = -((static_cast<float>(InY) / ClientHeight) * 2.0f - 1.0f); // Y축은 방향이 반대

	FVector3 ClickPosition(ndc_x, ndc_y, 0.0f);

	// Find Clicked Ball
	int clickedBallIndex = -1;
	for (int i = TotalPrimitives - 1; i >= 0; --i)
	{
		UBall* Ball = static_cast<UBall*>(PrimitiveList[i]);
		FVector3 Delta = ClickPosition - Ball->Location;
		Delta.z = 0;

		if (Delta.LengthSquare() < Ball->Radius * Ball->Radius)
		{
			clickedBallIndex = i;
			break;
		}
	}

	// If Click, Execute Logic
	if (clickedBallIndex != -1)
	{
		if (InIsLeftClick)
		{
			// 왼쪽 클릭: 해당 공 제거
			RemoveSpecificBall(clickedBallIndex);
		}
		else
		{
			// 오른쪽 클릭: 해당 공을 중력 중심으로 설정 / 해제
			SetGravityCenter(clickedBallIndex);
		}
	}
}
