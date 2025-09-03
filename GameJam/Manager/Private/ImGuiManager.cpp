#include "pch.h"
#include "Manager/Public/ImGuiManager.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

#include "Mesh/Public/UBall.h"
#include "Mesh/Public/UTriangle.h"
#include "Manager/Public/InputManager.h"
#include "Manager/Public/TimeManager.h"
#include "Manager/Public/ScoreManager.h"
#include "Render/Public/Renderer.h"
#include "Manager/Public/UIManager.h"
#include "Manager/Public/SceneManager.h"
#include "Scene/Public/Scene.h"
/**
 * @brief ImGui Initializer
 */
void FImGuiManager::InitializeImGui(HWND InWindowHandle, const URenderer& InRenderer)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(InWindowHandle);
	ImGui_ImplDX11_Init(InRenderer.Device, InRenderer.DeviceContext);
}

void FImGuiManager::ReleaseImGui()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

/**
 * @brief ImGui Process
 */
void FImGuiManager::RenderImGui()
{
	// Get New Frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	static bool bShowCredits = false;
	ImGui::NewFrame();


	if (FSceneManager::GetInstance().GetCurrentScene()->GetName() == "GAME")
	{
		RenderGameGui();
		if (FSceneManager::GetInstance().GetCurrentScene()->GetPause())
		{
			// 화면 중앙 좌표 가져오기
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImVec2 center = viewport->GetCenter();

			// 창 위치와 크기 설정
			ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_Always);

			// 창 스타일 플래그 설정
			ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

			// "GameOver!" 창 그리기 시작
			ImGui::Begin("GameOver!", nullptr, window_flags);

			// GameOver 텍스트를 크게 중앙에 표시
			ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("GAME OVER").x) * 0.5f);
			ImGui::Text("GAME OVER");

			ImGui::Spacing(); // 약간의 간격 추가
			ImGui::Separator(); // 구분선 추가
			ImGui::Spacing();
			ImGui::Spacing();

			// RESTART 버튼 중앙 정렬 및 그리기
			float restartButtonWidth = 150; // 버튼 크기를 고정하거나 계산
			ImGui::SetCursorPosX((ImGui::GetWindowSize().x - restartButtonWidth) * 0.5f);
			if (ImGui::Button("RESTART", ImVec2(restartButtonWidth, 40)))
			{
				FSceneManager::GetInstance().LoadScene("GAME");
			}

			ImGui::Spacing();

			// EXIT TO LOBBY 버튼 중앙 정렬 및 그리기
			float exitButtonWidth = 150;
			ImGui::SetCursorPosX((ImGui::GetWindowSize().x - exitButtonWidth) * 0.5f);
			if (ImGui::Button("EXIT TO LOBBY", ImVec2(exitButtonWidth, 40)))
			{
				FSceneManager::GetInstance().LoadScene("LOBBY");
			}

			ImGui::End();
		}
	}

	else
	{
		ImGui::Begin("Game UI");

		UIManager::GetInstance().Render();



		if (ImGui::Button("Credits"))
		{
			bShowCredits = !bShowCredits;
		}
		if(bShowCredits)
		{
			ImGui::Text("Team 5");
			ImGui::Text("Kim HeeJun, Lee HoJin,");
			ImGui::Text("Jung SeYeon, Heo Jun");
		}
		ImGui::End();
	}


	// Render ImGui
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void FImGuiManager::RenderGameGui()
{
	// 키 입력 상태를 표시하는 창 추가
	ImGui::Begin("Key Input Status");

	// KeyManager에서 현재 눌린 키들 가져오기
	FInputManager* KeyManager = FInputManager::GetInstance();
	if (KeyManager)
	{
		vector<EKeyInput> PressedKeys = KeyManager->GetPressedKeys();

		ImGui::Text("Pressed Keys: ");
		ImGui::Separator();

		if (PressedKeys.empty())
		{
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(Nothing Input)");
		}
		else
		{
			for (const EKeyInput& Key : PressedKeys)
			{
				const char* KeyString = FInputManager::KeyInputToString(Key);
				ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "- %s", KeyString);
			}
		}

		ImGui::Separator();
		ImGui::Separator();

		// 마우스 위치 표시
		FVector2 MousePosition = KeyManager->GetMousePosition();
		ImGui::Text("Mouse Position: (%.0f, %.0f)", MousePosition.x, MousePosition.y);

		ImGui::Text("Total %d Key Typing Now", static_cast<int>(PressedKeys.size()));

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Manual: ");
		ImGui::Text("ESC: Exit");
		ImGui::Text("Space: Charge Shooter (Hold / Release)");
		ImGui::Text("Delete: Remove Ball");
	}

	ImGui::End();

	ImGui::Begin("Frame Performance Info");

	FTimeManager* TimeManager = FTimeManager::GetInstance();

	if (TimeManager)
	{
		ImGui::Text("Current FPS: %.1f", TimeManager->GetFPS());
		ImGui::Text("Delta Time: %.4f ms", TimeManager->GetDeltaTime() * 1000.0f);
		ImGui::Text("Game Time: %.2f s", TimeManager->GetGameTime());

		ImGui::Separator();

		if (TimeManager->IsPaused())
		{
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Game Paused");
		}
		else
		{
			ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Game Resumed");
		}

		ImGui::Separator();

		float CurrentFPS = TimeManager->GetFPS();
		ImVec4 FPSColor;

		if (CurrentFPS >= 60.0f)
		{
			FPSColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // 녹색 (우수)
		}
		else if (CurrentFPS >= 30.0f)
		{
			FPSColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // 노란색 (보통)
		}
		else
		{
			FPSColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // 빨간색 (주의)
		}

		ImGui::TextColored(FPSColor, "Current FPS: %.1f", CurrentFPS);
	}
	else
	{
		ImGui::Text("TimeManager를 찾을 수 없습니다.");
	}

	ImGui::End();

	ImGui::Begin("Current Score");

	FScoreManager* ScoreManager = FScoreManager::GetInstance();
	if (ScoreManager)
	{
		ImGui::Text("Current Score: %d", ScoreManager->GetCurrentScore());
	}

	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::End();
}
