#include "pch.h"
#include "Manager/Public/ImGuiManager.h"

#include "Actor/Public/UBall.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"
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

	/** 여기까지 ImGui에 필요한 UI 작성 **/

	ImGui::End();

	// Render ImGui
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
