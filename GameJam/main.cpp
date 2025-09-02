#include "pch.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

#include "Core/Public/Primitive.h"
#include "Asset/Sphere.h"
#include "Actor/Public/UBall.h"
#include "Manager/Public/ImGuiManager.h"
#include "Manager/Public/KeyManager.h"
#include "Render/Public/Renderer.h"

static void HandleMouseClick(int InX, int InY, bool InIsLeftClick);
static void RemoveSpecificBall(int IndexToRemove);
static void SetGravityCenter(int IndexToSet);
static void HandleCollisions();

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

/**
 * @brief 매 프레임 반복되는 Logic을 처리하는 함수
 */
static void MainLoop(URenderer& InRenderer)
{
	// Renderer와 Shader 생성 이후에 버텍스 버퍼를 생성합니다.
	UINT numVerticesSphere = sizeof(sphere_vertices) / sizeof(FVertexSimple);

	InRenderer.vertexBufferSphere = InRenderer.CreateVertexBuffer(
		sphere_vertices, sizeof(sphere_vertices));

	InRenderer.numVerticesSphere = numVerticesSphere;

	const int TargetFPS = 30;
	const double TargetFrameTime = 1000.0 / TargetFPS;

	LARGE_INTEGER Frequency;
	QueryPerformanceFrequency(&Frequency);

	LARGE_INTEGER StartTime, EndTime;
	double ElapsedTime = 0.0;
	bool bIsExit = false;

	// KeyManager 인스턴스 가져오기
	FKeyManager* KeyManager = FKeyManager::GetInstance();

	while (bIsExit == false)
	{
		QueryPerformanceCounter(&StartTime);

		MSG msg;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
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

		// 키 입력 처리 예시 - ESC키로 나가기
		if (KeyManager->IsKeyPressed(EKeyInput::Esc))
		{
			PostMessage(GlobalWindowHandle, WM_CLOSE, 0, 0);
			bIsExit = true;
		}

		// 키 입력 처리 예시 - Space키로 공 추가
		if (KeyManager->IsKeyPressed(EKeyInput::Space))
		{
			AddNewBall();
		}

		// 키 입력 처리 예시 - Delete키로 공 제거
		if (KeyManager->IsKeyPressed(EKeyInput::Delete))
		{
			RemoveRandomBall();
		}

		frameCount++;

		// 첫 몇 프레임 동안은 공 생성을 지연
		if (frameCount > minFramesBeforeFirstBall)
		{
			// 한 프레임에 하나씩만 공을 추가/제거하여 안정성 향상
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

		// 물리 업데이트
		for (int i = 0; i < TotalPrimitives; ++i)
		{
			UBall* Ball = static_cast<UBall*>(PrimitiveList[i]);
			Ball->Move();
		}

		// 물리 업데이트 후 충돌 처리
		HandleCollisions();

		// 렌더링
		InRenderer.Prepare();
		InRenderer.PrepareShader();

		for (int i = 0; i < TotalPrimitives; ++i)
		{
			UBall* ball = static_cast<UBall*>(PrimitiveList[i]);
			InRenderer.UpdateConstant(ball->Location, ball->Radius);
			InRenderer.RenderPrimitive();
		}

		FImGuiManager::RenderImGui();

		InRenderer.SwapBuffer();

		// 일정한 프레임 타임을 유지
		do
		{
			Sleep(0);
			QueryPerformanceCounter(&EndTime);
			ElapsedTime = (EndTime.QuadPart - StartTime.QuadPart) * 1000.0 / Frequency.QuadPart;
		}
		while (ElapsedTime < TargetFrameTime);
	}
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

	URenderer Renderer;

	Renderer.TotalInit(WindowHandle);

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

	// KeyManager 인스턴스 해제
	FKeyManager* KeyManager = FKeyManager::GetInstance();
	if (KeyManager)
	{
		delete KeyManager;
	}

	Renderer.TotalShutDown();

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// 각종 메시지를 처리할 함수
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

	// KeyManager에 메시지 전달 (옵션)
	FKeyManager* KeyManager = FKeyManager::GetInstance();
	if (KeyManager)
	{
		KeyManager->ProcessKeyMessage(message, wParam, lParam);
	}

	switch (message)
	{
	// 마우스 왼쪽 버튼 클릭
	case WM_LBUTTONDOWN:
		{
			HandleMouseClick(LOWORD(lParam), HIWORD(lParam), true);
			return 0;
		}
	// 마우스 오른쪽 버튼 클릭
	case WM_RBUTTONDOWN:
		{
			HandleMouseClick(LOWORD(lParam), HIWORD(lParam), false);
			return 0;
		}
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
	float clientWidth = static_cast<float>(ClientRect.right - ClientRect.left);
	float clientHeight = static_cast<float>(ClientRect.bottom - ClientRect.top);

	// NDC Convert
	float ndc_x = (static_cast<float>(InX) / clientWidth) * 2.0f - 1.0f;
	float ndc_y = -((static_cast<float>(InY) / clientHeight) * 2.0f - 1.0f); // Y축은 방향이 반대

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
