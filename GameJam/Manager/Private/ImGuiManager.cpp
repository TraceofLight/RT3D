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

	static bool bShowCredits = false;

	ImGui::Begin("Jungle Property Window");

	/** 여기서부터 ImGui에 필요한 UI 작성 **/

	ImGui::Text("Hello Jungle World!");

	ImGui::Checkbox("Gravity", &bPinballGravity);

	if (ImGui::InputInt("Number of Balls", &UBall::TotalNumBalls, 1))
	{
		UBall::TotalNumBalls = max(0, UBall::TotalNumBalls);
	}

	if (GravityCenterBall)
	{
		ImGui::Text("Gravity Center Is Active.");
	}
	else
	{
		ImGui::Text("Right-Click A Ball To Set Gravity Center.");
	}

	// ----- Triangle UI 추가 시작 -----
	ImGui::Separator();
	ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "Triangle (Incenter Based)");

	// 라디안 -> 도 단위 변환
	float rotationDeg = GTriangle.Rotation * 180.0f / 3.14159265358979323846f;
	if (ImGui::SliderFloat("Rotation (deg)", &rotationDeg, 0.0f, 360.0f, "%.1f"))
	{
		// 도 -> 라디안
		GTriangle.Rotation = rotationDeg * (3.14159265358979323846f / 180.0f);
		// 0~2π 정규화 (선택)
		const float TwoPi = 6.2831853071795864769f;
		if (GTriangle.Rotation >= TwoPi || GTriangle.Rotation < 0.0f)
		{
			GTriangle.Rotation = fmodf(GTriangle.Rotation, TwoPi);
			if (GTriangle.Rotation < 0.0f) GTriangle.Rotation += TwoPi;
		}
	}

	// (선택 표시) 현재 라디안 값
	ImGui::Text("Rotation(rad): %.4f", GTriangle.Rotation);
	// ----- Triangle UI 추가 끝 -----

	/** 여기까지 ImGui에 필요한 UI 작성 **/

	ImGui::End();

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
		ImGui::Text("Space: Add Ball");
		ImGui::Text("Delete: Remove Ball");
		ImGui::Text("Left Click: Remove Target Ball");
		ImGui::Text("Right Click: Set Gravity Ball / Release");
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

	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::Begin("Options");
	if (ImGui::Button("Credits"))
		bShowCredits = true;
	ImGui::End();

	if (bShowCredits)
	{
		ImGui::Begin("Credits", &bShowCredits, ImGuiWindowFlags_NoCollapse);
		ImGui::Text("Team 5");
		ImGui::Text("Kim HeeJun, Lee HoJin,");
		ImGui::Text("Jung SeYeon, Heo Jun");
		ImGui::End();
	}


	// Render ImGui
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
