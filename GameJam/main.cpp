#include "pch.h"

#include "Actor/Public/Pad.h"
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

// Static
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void RenderProcess(const URenderer& InRenderer);
static HWND GlobalWindowHandle = nullptr;

void RenderProcess(const URenderer& InRenderer, const UShooter* Shooter = nullptr);
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

	UShooter Shooter;
	Shooter.SetLocation({0.3f, -0.8f, 0.0f});

	// Pad 설정
	GLeftPad.ConfigueLeftPad();
	GRightPad.ConfigueRightPad();

	bool bIsExit = false;
	while (!bIsExit)
	{
		// Update TimeManager
		TimeManager->Update();

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

		KeyManager->Update();
		if (KeyManager->IsKeyPressed(EKeyInput::Esc))
		{
			PostMessage(GlobalWindowHandle, WM_CLOSE, 0, 0);
			bIsExit = true;
			continue;
		}
		// KeyManager 업데이트
		KeyManager->Update();
		InputProcess(bIsExit);

		// === 게임 로직 업데이트 ===
		// Charge
		if (KeyManager->IsKeyDown(EKeyInput::Space))
		{
			Shooter.Charging();
			DEBUG_PRINT("[MAINLOOP] Shooter Charging...\n");
		}
		// Shoot
		else if (KeyManager->IsKeyReleased(EKeyInput::Space))
		{
			Shooter.Shoot();
			DEBUG_PRINT("[MAINLOOP] Shooter Fire!\n");
		}

		// SceneManager에서 현재 씬의 Primitives 가져오기
		FSceneManager& SceneManager = FSceneManager::GetInstance();
		vector<UPrimitive*> ScenePrimitives = SceneManager.GetAllScenePrimivites();

		// Pad 회전 업데이트
		GLeftPad.HandleInput(KeyManager, TimeManager->GetDeltaTime());
		GRightPad.HandleInput(KeyManager, TimeManager->GetDeltaTime());

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

		// Rendering
		RenderProcess(InRenderer, &Shooter);
	}
}

void RenderProcess(const URenderer& InRenderer, const UShooter* Shooter)
{
	InRenderer.Prepare();
	InRenderer.PrepareShader();
	Scene* CurrentScene = FSceneManager::GetInstance().GetCurrentScene();
	Scene* PrevScene = nullptr;
	if (CurrentScene)
	{
		PrevScene = CurrentScene;
		CurrentScene->Update(FTimeManager::GetInstance()->GetDeltaTime());
		if (PrevScene == FSceneManager::GetInstance().GetCurrentScene())
		{
			CurrentScene->Render();
		}
	}

	//RenderProcess(*URenderer::GetInstance());

	// 사각형 렌더링
	InRenderer.UpdateConstantForRectangle(GRectangle.Location, GRectangle.Width, GRectangle.Height);
	InRenderer.RenderRectangle();

	// Pad Render
	GLeftPad.Render(InRenderer);
	GRightPad.Render(InRenderer);

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
	SceneManager.LoadScene("GAME");
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

void RenderProcess(const URenderer& InRenderer)
{
	InRenderer.Prepare();
	InRenderer.PrepareShader();


	// ImGui 렌더링 (TimeManager 정보 표시 가능)
	FImGuiManager::RenderImGui();

	// 백버퍼 스왑
	InRenderer.SwapBuffer();
}
