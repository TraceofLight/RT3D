#include "pch.h"

#include "Actor/Public/PadPair.h"
#include "Actor/Public/Shooter.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

#include "Manager/Public/ImGuiManager.h"
#include "Manager/Public/InputManager.h"
#include "Manager/Public/ScoreManager.h"
#include "Render/Public/Renderer.h"
#include "Manager/Public/UIManager.h"
#include "Manager/Public/SceneManager.h"
#include "Scene/Public/GameScene.h"
#include "Scene/Public/LobbyScene.h"
#include "Asset/Sphere.h"
#include "Asset/Rectangle.h"
#include "Asset/Triangle.h"
#include "Mesh/Public/UBall.h"
#include "Mesh/Public/UTriangle.h"

// 외부 터미널 출력 전역 변수
bool bShowExternalTerminal = true;

// 외부 터미널 초기화 함수
static bool bExternalTerminalInitialized = false;
static void InitializeExternalTerminal();

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static void AddNewRectangle(FVector3 location, float rotation, float width, float height);
static void AddNewTriangle(FVector3 location, float rotation, float base, float height);
static void RemoveRandomBall();
static void InitSceneTemp();

// Static
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void RenderProcess(const URenderer& InRenderer);
static HWND GlobalWindowHandle = nullptr;

// Global variables definition
int TotalPrimitives = 0;
UPrimitive** PrimitiveList = nullptr;
static int frameCount = 0;
static const int minFramesBeforeFirstBall = 5;

//void RenderProcess(const URenderer& InRenderer, const UShooter* Shooter = nullptr);
void InputProcess(bool& InExitFlag);

/**
 * @brief 외부 터미널을 초기화하는 함수
 * @details bShowExternalTerminal이 true이면 새로운 콘솔 창을 할당하고 출력을 리다이렉트합니다.
 */
static void InitializeExternalTerminal()
{
	if (bShowExternalTerminal && !bExternalTerminalInitialized)
	{
		// 새로운 콘솔 창 할당
		if (AllocConsole())
		{
			// 콘솔 창 제목 설정
			SetConsoleTitle(L"Game Debug Console");

			// 표준 입출력을 콘솔로 리다이렉트
			FILE* pCout;
			FILE* pCin;
			FILE* pCerr;

			// stdout, stdin, stderr를 콘솔로 리다이렉트
			(void)freopen_s(&pCout, "CONOUT$", "w", stdout);
			(void)freopen_s(&pCin, "CONIN$", "r", stdin);
			(void)freopen_s(&pCerr, "CONOUT$", "w", stderr);

			// iostream 동기화
			std::ios::sync_with_stdio(true);

			bExternalTerminalInitialized = true;

			// 초기화 메시지 출력
			printf("[CONSOLE] External Terminal initialized successfully.\n");
		}
		else
		{
			OutputDebugStringA("[ERROR] Failed to allocate console.\n");
		}
	}
}

/**
 * @brief 매 프레임 반복되는 Logic을 처리하는 함수
 */
static void MainLoop(URenderer& InRenderer)
{
	FTimeManager* TimeManager = FTimeManager::GetInstance();
	FInputManager* KeyManager = FInputManager::GetInstance();
	FSceneManager* SceneManager = &FSceneManager::GetInstance();

	// UShooter Shooter;
	// Shooter.SetLocation({0.3f, -0.8f, 0.0f});
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

	GPadPair.Init(PadCfg);

	bool bIsExit = false;
	while (!bIsExit)
	{
		// Update TimeManager

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
		TimeManager->Update();

		KeyManager->Update();
		InputProcess(bIsExit);

		RenderProcess(InRenderer);

		Scene* CurrentScene = SceneManager->GetCurrentScene();
		if (CurrentScene)
		{
			CurrentScene->Update(TimeManager->GetDeltaTime());
			if (CurrentScene == SceneManager->GetCurrentScene())
			{
				CurrentScene->Render();
			}
		}
	}
}

void RenderProcess(const URenderer& InRenderer)
{
	InRenderer.Prepare();
	InRenderer.PrepareShader();

	for (int i = 0; i < TotalPrimitives; ++i)
	{
		UPrimitive* Primitive = PrimitiveList[i];

		if (URectangle* Rectangle = dynamic_cast<URectangle*>(Primitive))
		{
			InRenderer.UpdateConstantForRectangle(Rectangle->Location, Rectangle->Width, Rectangle->Height, Rectangle->Rotation);
			InRenderer.RenderRectangle();
		}
		else if (UTriangle* Triangle = dynamic_cast<UTriangle*>(Primitive))
		{
			InRenderer.UpdateConstantForTriangle(Triangle->Location, Triangle->Base, Triangle->Height, Triangle->Rotation,
										 Triangle->Radius);
			InRenderer.RenderTriangle();
		}
	}

	// 사각형 렌더링
	// InRenderer.UpdateConstantForRectangle(GRectangle.Location, GRectangle.Width, GRectangle.Height);
	// InRenderer.RenderRectangle();

	// Triangle Render
	InRenderer.UpdateConstantForTriangle(GTriangle.Location, GTriangle.Base, GTriangle.Height, GTriangle.Rotation,
										 GTriangle.Radius);
	InRenderer.RenderTriangle();

	// ImGui 렌더링 (TimeManager 정보 표시 가능)
	FImGuiManager::RenderImGui();

	// 백버퍼 스왑
	InRenderer.SwapBuffer();
}

void InputProcess(bool& InExitFlag)
{
	// Deprecated
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
	SceneManager.LoadScene("LOBBY");
}

static void InitSceneTemp()
{
	FVector3 randomLocation = FVector3(-0.8f + (rand() / static_cast<float>(RAND_MAX)) * 1.6f,
		-0.8f + (rand() / static_cast<float>(RAND_MAX)) * 1.6f, 1.0f);

	float randomRotation = -0.8f + (rand() / static_cast<float>(RAND_MAX)) * 1.6f;

	// AddNewTriangle(randomLocation, randomRotation, 0.3f, 0.9f);
	AddNewRectangle(randomLocation, randomRotation, 0.1f, 0.1f);
	// AddNewRectangle(FVector3(), 1.0f, 1.0f, 1.0f);
	// AddNewBall();

	// right wall
	AddNewRectangle(FVector3(0.8f, -0.2f, 0.0f), 0.0f, 0.1f, 1.6f);
	// left wall
	AddNewRectangle(FVector3(-0.8f, -0.2f, 0.0f), 0.0f, 0.1f, 1.6f);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	// 외부 터미널 초기화
	InitializeExternalTerminal();

	// 난수 시드 초기화
	srand(static_cast<unsigned int>(GetTickCount()));

	WCHAR WindowClass[] = L"JungleWindowClass";
	WCHAR Title[] = L"Game Tech Lab";

	WNDCLASSW wndclass = {0, WndProc, 0, 0, 0, 0, 0, 0, 0, WindowClass};
	RegisterClassW(&wndclass);

	HWND WindowHandle = CreateWindowExW(0, WindowClass, Title,
	                                    WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
	                                    CW_USEDEFAULT, CW_USEDEFAULT, 1024, 1024,
	                                    nullptr, nullptr, hInstance, nullptr);
	GlobalWindowHandle = WindowHandle;
	InitEngine(WindowHandle, *(URenderer::GetInstance()));

	InitSceneTemp();
	//SceneManager::GetInstance().LoadScene("GAME SCENE");

	MainLoop(*URenderer::GetInstance());

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

	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}


void AddNewRectangle(FVector3 location, float rotation, float width, float height)
{
	// Make New List
	UPrimitive** NewList = new UPrimitive*[TotalPrimitives + 1];

	// Copy
	for (int i = 0; i < TotalPrimitives; ++i)
	{
		NewList[i] = PrimitiveList[i];
	}

	// Add New Rectangle
	URectangle* NewRectangle = new URectangle();
	NewRectangle->Location = location;
	NewRectangle->Rotation = rotation;
	NewRectangle->Width = width;
	NewRectangle->Height = height;
	NewRectangle->Mass = width * height;
	NewRectangle->Velocity = FVector3(0.0f, 0.0f, 0.0f);
	NewList[TotalPrimitives] = NewRectangle;

	// Release
	if (PrimitiveList != nullptr)
	{
		delete[] PrimitiveList;
	}

	// Swap List
	PrimitiveList = NewList;

	++TotalPrimitives;
}

void AddNewTriangle(FVector3 location, float rotation, float base, float height)
{
	// Make New List
	UPrimitive** NewList = new UPrimitive*[TotalPrimitives + 1];

	// Copy
	for (int i = 0; i < TotalPrimitives; ++i)
	{
		NewList[i] = PrimitiveList[i];
	}

	// Add New Rectangle
	UTriangle* NewTriangle = new UTriangle();
	NewTriangle->Location = location;
	NewTriangle->Rotation = rotation;
	NewTriangle->Base = base;
	NewTriangle->Height = height;
	NewList[TotalPrimitives] = NewTriangle;

	// Release
	if (PrimitiveList != nullptr)
	{
		delete[] PrimitiveList;
	}

	// Swap List
	PrimitiveList = NewList;

	++TotalPrimitives;
}
