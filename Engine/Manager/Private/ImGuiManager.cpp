#include "pch.h"
#include "Manager/Public/ImGuiManager.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

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
	ImGui::NewFrame();

	if (FSceneManager::GetInstance().GetCurrentScene()->GetName() == "LOBBY")
	{
		RenderLobbyGui();
	}

	if (FSceneManager::GetInstance().GetCurrentScene()->GetName() == "GAME")
	{
		// Game Over 상태일 때는 게임 UI를 렌더링하지 않음
		if (!FSceneManager::GetInstance().GetCurrentScene()->IsPause())
		{
			RenderGameGui();
		}

		if (FSceneManager::GetInstance().GetCurrentScene()->IsPause())
		{
			// 플레이어 이름 입력을 위한 정적 변수
			static char PlayerName[32] = "Player";
			static bool ScoreSubmitted = false;

			// 화면 중앙 좌표 가져오기
			const ImGuiViewport* Viewport = ImGui::GetMainViewport();
			ImVec2 Center = Viewport->GetCenter();

			ImGui::SetNextWindowPos(Center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize(ImVec2(400, 350), ImGuiCond_Always);

			// 창 스타일 플래그 설정
			ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

			// "GameOver!" 창 그리기 시작
			ImGui::Begin(" ", nullptr, WindowFlags);

			// GameOver 텍스트를 크게 중앙에 표시
			ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("GAME OVER").x) * 0.5f);
			ImGui::Text("GAME OVER!");

			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();

			// 최종 점수 표시
			FScoreManager* ScoreManager = FScoreManager::GetInstance();
			if (ScoreManager)
			{
				// 실제 점수 텍스트 크기 계산
				char ScoreText[64];
				snprintf(ScoreText, sizeof(ScoreText), "Final Score: %d", ScoreManager->GetCurrentScore());
				ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(ScoreText).x) * 0.5f);
				ImGui::Text("%s", ScoreText);
			}

			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();

			// 플레이어 이름 입력 필드 (중앙 정렬)
			ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Enter Your Name:").x) * 0.5f);
			ImGui::Text("Enter Your Name:");
			ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 200.0f) * 0.5f);
			ImGui::SetNextItemWidth(200.0f);
			ImGui::InputText("##PlayerName", PlayerName, sizeof(PlayerName));

			ImGui::Spacing();
			ImGui::Spacing();

			// 점수 제출 버튼
			if (!ScoreSubmitted)
			{
				ImGui::Spacing();
				float SubmitButtonWidth = 150;
				ImGui::SetCursorPosX((ImGui::GetWindowSize().x - SubmitButtonWidth) * 0.5f);

				bool NameValid = strlen(PlayerName) > 0 && strlen(PlayerName) < sizeof(PlayerName) - 1;

				// 이름이 유효하지 않으면 버튼 비활성화
				if (!NameValid)
				{
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
				}

				if (ImGui::Button("SUBMIT SCORE", ImVec2(SubmitButtonWidth, 30)) && NameValid)
				{
					if (ScoreManager)
					{
						ScoreManager->SubmitScore(string(PlayerName));
						ScoreSubmitted = true;
					}
				}

				if (!NameValid)
				{
					ImGui::PopStyleVar();
					// 경고 메시지 표시
					if (strlen(PlayerName) == 0)
					{
						ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Name Cannot Be Empty").x) * 0.5f);
						ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Name Cannot Be Empty");
					}
					else
					{
						ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Name Too Long (max 30 chars)").x) * 0.5f);
						ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Name Too Long (max 30 chars)");
					}
				}
				else
				{
					ImGui::Spacing();
					ImGui::Spacing();
				}
			}
			else
			{
				ImGui::Spacing();
				ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Score Submitted!").x) * 0.5f);
				ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Score Submitted!");
			}

			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();

			// RESTART 버튼 중앙 정렬 및 그리기
			float RestartButtonWidth = 150;
			ImGui::SetCursorPosX((ImGui::GetWindowSize().x - RestartButtonWidth) * 0.5f);
			if (ImGui::Button("RESTART", ImVec2(RestartButtonWidth, 40)))
			{
				// 게임 재시작 시 점수 제출 상태 리셋
				ScoreSubmitted = false;
				strcpy_s(PlayerName, sizeof(PlayerName), "Player"); // 기본 이름으로 리셋
				FSceneManager::GetInstance().LoadScene("GAME");
			}

			ImGui::Spacing();

			// EXIT TO LOBBY 버튼 중앙 정렬 및 그리기
			float exitButtonWidth = 150;
			ImGui::SetCursorPosX((ImGui::GetWindowSize().x - exitButtonWidth) * 0.5f);
			if (ImGui::Button("EXIT TO LOBBY", ImVec2(exitButtonWidth, 40)))
			{
				// 로비로 나갈 때도 점수 제출 상태 리셋
				ScoreSubmitted = false;
				strcpy_s(PlayerName, sizeof(PlayerName), "Player"); // 기본 이름으로 리셋
				FSceneManager::GetInstance().LoadScene("LOBBY");
			}

			ImGui::End();
		}
	}

	// Render ImGui
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void FImGuiManager::RenderLobbyGui()
{
	static bool bShowCredits = false;

	// 화면 크기 정보 가져오기
	ImGuiIO& IO = ImGui::GetIO();
	float ScreenWidth = IO.DisplaySize.x;
	float ScreenHeight = IO.DisplaySize.y;

	// 메인 메뉴 위도우 크기 및 위치 설정
	float WindowWidth = 300.0f;
	float WindowHeight = 280.f;
	float WindowX = (ScreenWidth - WindowWidth) * 0.5f;
	float WindowY = (ScreenHeight - WindowHeight) * 0.5f;

	ImGui::SetNextWindowPos(ImVec2(WindowX, WindowY));
	ImGui::SetNextWindowSize(ImVec2(WindowWidth, WindowHeight));
	ImGui::Begin("PinBall Game", nullptr,
	             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

	// 제목 위 패딩
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();

	// 제목 텍스트 중앙 정렬
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 15));

	// 제목 스타일
	ImGuiStyle& Style = ImGui::GetStyle();
	float OriginalFontSize = ImGui::GetFont()->Scale;
	ImGui::SetWindowFontScale(2.0f);

	const char* Title = "PinBall";
	float TitleWidth = ImGui::CalcTextSize(Title).x;
	ImGui::SetCursorPosX((WindowWidth - TitleWidth) * 0.5f);
	ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", Title);

	ImGui::SetWindowFontScale(1.0f);
	ImGui::PopStyleVar();

	// 제목과 구분선 사이 패딩
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();

	// Game Start Button
	UIManager::GetInstance().Render();

	// 버튼과 구분선 사이 패딩
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();
	ImGui::Spacing();

	// Credits 버튼 중앙 정렬
	float buttonWidth = 120.0f;
	ImGui::SetCursorPosX((WindowWidth - buttonWidth) * 0.5f);
	if (ImGui::Button("Credits", ImVec2(buttonWidth, 0)))
	{
		bShowCredits = !bShowCredits;
	}

	ImGui::Spacing();

	// Credits 정보 표시
	if (bShowCredits)
	{
		ImGui::Spacing();

		// 팀 정보 중앙 정렬
		ImGui::SetCursorPosX((WindowWidth - ImGui::CalcTextSize("Team 5").x) * 0.5f);
		ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Team 5");

		ImGui::SetCursorPosX((WindowWidth - ImGui::CalcTextSize("Kim HeeJun, Lee HoJin,").x) * 0.5f);
		ImGui::Text("Kim HeeJun, Lee HoJin,");

		ImGui::SetCursorPosX((WindowWidth - ImGui::CalcTextSize("Jung SeYeon, Heo Jun").x) * 0.5f);
		ImGui::Text("Jung SeYeon, Heo Jun");
	}

	ImGui::End();
}

void FImGuiManager::RenderGameGui()
{
	// Get Screen Info
	ImGuiIO& IO = ImGui::GetIO();
	float ScreenWidth = IO.DisplaySize.x;
	float ScreenHeight = IO.DisplaySize.y;

	// Panel Setting
	float RightPanelWidth = 300.0f;
	float CurrentY = 10.0f;
	float WindowPadding = 10.0f;
	float RightPanelX = ScreenWidth - RightPanelWidth - WindowPadding;

	// Pinball Title
	ImGui::SetNextWindowPos(ImVec2(RightPanelX, CurrentY));
	ImGui::SetNextWindowSize(ImVec2(RightPanelWidth, 70.0f));
	ImGui::Begin(" ", nullptr,
	             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

	ImGui::SetWindowFontScale(2.0f);
	const char* Title = "Pinball";
	float TitleWidth = ImGui::CalcTextSize(Title).x;
	ImGui::SetCursorPosX((RightPanelWidth - TitleWidth) * 0.5f);
	ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", Title);

	// Reset Font Size
	ImGui::SetWindowFontScale(1.0f);

	ImGui::End();
	CurrentY += 80.0f;

	// Current Score
	ImGui::SetNextWindowPos(ImVec2(RightPanelX, CurrentY));
	ImGui::SetNextWindowSize(ImVec2(RightPanelWidth, 90.0f));
	ImGui::Begin("Current Score", nullptr,
	             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

	FScoreManager* ScoreManager = FScoreManager::GetInstance();
	if (ScoreManager)
	{
		ImGui::SetWindowFontScale(1.5f);
		ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Score: %d", ScoreManager->GetCurrentScore());
		ImGui::SetWindowFontScale(1.0f);
	}

	ImGui::End();
	CurrentY += 100.0f;

	// Leaderboard
	ImGui::SetNextWindowPos(ImVec2(RightPanelX, CurrentY));
	ImGui::SetNextWindowSize(ImVec2(RightPanelWidth, 210.0f));
	ImGui::Begin("Leaderboard", nullptr,
	             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

	if (ScoreManager)
	{
		const vector<FScoreEntry>& leaderboard = ScoreManager->GetLeaderboard();
		ImGui::Text("Top 10 Scores:");
		ImGui::Separator();

		if (leaderboard.empty())
		{
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "None");
		}
		else
		{
			for (size_t i = 0; i < leaderboard.size(); ++i)
			{
				ImVec4 TextColor = (i < 3) ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
				ImGui::TextColored(TextColor, "%d. %s - %d",
				                   static_cast<int>(i + 1),
				                   leaderboard[i].PlayerName.c_str(),
				                   leaderboard[i].Score);
			}
		}
	}

	ImGui::End();
	CurrentY += 220.0f;

	// Frame Performance Info
	ImGui::SetNextWindowPos(ImVec2(RightPanelX, CurrentY));
	ImGui::SetNextWindowSize(ImVec2(RightPanelWidth, 120.0f));
	ImGui::Begin("Frame Performance Info", nullptr,
	             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

	FTimeManager* TimeManager = FTimeManager::GetInstance();
	if (TimeManager)
	{
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

		ImGui::TextColored(FPSColor, "FPS: %.1f", CurrentFPS);
		ImGui::Text("Delta Time: %.2f ms", TimeManager->GetDeltaTime() * 1000.0f);
		ImGui::Text("Game Time: %.1f s", TimeManager->GetGameTime());

		ImGui::Separator();

		if (TimeManager->IsPaused())
		{
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Game Paused");
		}
		else
		{
			ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Game Resumed");
		}
	}
	else
	{
		ImGui::Text("TimeManager를 찾을 수 없습니다.");
	}

	ImGui::End();
	CurrentY += 130.0f;

	// Key Input Status
	float KeyInputHeight = ScreenHeight - CurrentY - WindowPadding;
	ImGui::SetNextWindowPos(ImVec2(RightPanelX, CurrentY));
	ImGui::SetNextWindowSize(ImVec2(RightPanelWidth, KeyInputHeight));
	ImGui::Begin("Key Input Status", nullptr,
	             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

	// KeyManager에서 현재 눌린 키들 가져오기
	FInputManager* KeyManager = FInputManager::GetInstance();
	if (KeyManager)
	{
		vector<EKeyInput> PressedKeys = KeyManager->GetPressedKeys();

		ImGui::Text("Pressed Keys:");
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

		// 마우스 위치 표시
		FVector2 MousePosition = KeyManager->GetMousePosition();
		ImGui::Text("Mouse: (%.0f, %.0f)", MousePosition.x, MousePosition.y);
		ImGui::Text("Total Keys: %d", static_cast<int>(PressedKeys.size()));

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Controls:");
		ImGui::Text("ESC: Exit");
		ImGui::Text("Space: Shooter (Hold / Release)");
		ImGui::Text("A: Move Left Pad");
		ImGui::Text("D: Move Right Pad");
	}

	ImGui::End();
}
