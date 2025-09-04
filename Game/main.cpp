#include "pch.h"

#include "Actor/Public/FlipperPair.h"
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

// 외부 터미널 초기화 함수
static bool bExternalTerminalInitialized = false;
static void InitializeExternalTerminal();

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// Static
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void RenderProcess(const URenderer& InRenderer);
static HWND GlobalWindowHandle = nullptr;

//void RenderProcess(const URenderer& InRenderer, const UShooter* Shooter = nullptr);
void InputProcess(bool& InExitFlag);

/**
 * @brief 외부 터미널을 초기화하는 함수
 * @details bShowExternalTerminal이 true이면 새로운 콘솔 창을 할당하고 출력을 리다이렉트합니다.
 */
static void InitializeExternalTerminal()
{
	if (SHOW_EXTERNAL_TERMINAL && !bExternalTerminalInitialized)
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

		// 창이 포커스된 상태에서만 게임 로직 업데이트
		if (KeyManager->IsWindowFocused())
		{
			Scene* CurrentScene = SceneManager->GetCurrentScene();
			if (CurrentScene)
			{
				if (!CurrentScene->IsPause())
				{
					CurrentScene->Update(TimeManager->GetDeltaTime());
					if (CurrentScene == SceneManager->GetCurrentScene())
					{
						CurrentScene->Render();
					}
				}
			}
		}

		// Rendering
		InRenderer.SwapBuffer();
	}
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
	                                    CW_USEDEFAULT, CW_USEDEFAULT, SCREEN_WIDTH, SCREEN_HEIGHT,
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
	case WM_ACTIVATE:
		{
			// 창 활성화 상태 처리
			bool bIsActivated = (LOWORD(wParam) != WA_INACTIVE);
			if (KeyManager)
			{
				KeyManager->SetWindowFocus(bIsActivated);
			}

			// 디버그 로그 출력
			printf("[WINDOW] Window focus changed: %s\n", bIsActivated ? "FOCUSED" : "UNFOCUSED");
		}
		break;

	case WM_KILLFOCUS:
		{
			// 창이 포커스를 잃었을 때 처리 (추가 안전 장치)
			if (KeyManager)
			{
				KeyManager->SetWindowFocus(false);
			}
			printf("[WINDOW] Window lost focus (KILLFOCUS)\n");
		}
		break;

	case WM_SETFOCUS:
		{
			// 창이 포커스를 얻었을 때 처리
			if (KeyManager)
			{
				KeyManager->SetWindowFocus(true);
			}
			printf("[WINDOW] Window gained focus (SETFOCUS)\n");
		}
		break;

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
}
