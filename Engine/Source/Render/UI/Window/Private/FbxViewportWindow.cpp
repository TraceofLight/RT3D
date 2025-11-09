#include "pch.h"
#include "Render/UI/Window/Public/FbxViewportWindow.h"
#include "ImGui/imgui.h"

IMPLEMENT_CLASS(UFbxViewportWindow, UUIWindow)

UFbxViewportWindow::UFbxViewportWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "FBX Viewport";
	Config.DefaultSize = ImVec2(720, 480);
	Config.MinSize = ImVec2(360, 240);
	Config.DefaultPosition = ImVec2(140, 120);
	Config.InitialState = EUIWindowState::Hidden;
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;
	Config.Priority = 25;
	Config.UpdateWindowFlags();

	SetConfig(Config);
	SetWindowState(EUIWindowState::Hidden);
}

void UFbxViewportWindow::Initialize()
{
	UE_LOG("FbxViewportWindow: Window initialized");
}

void UFbxViewportWindow::OnPostRenderWindow()
{
	ImGui::TextColored(ImVec4(0.6f, 0.85f, 1.0f, 1.0f), "FBX Viewport Preview");
	ImGui::Separator();
	ImGui::TextWrapped("Viewport Manager integration will send a live FBX scene to this popup later.");
	ImGui::Spacing();

	RenderPlaceholderViewport();
}

void UFbxViewportWindow::RenderPlaceholderViewport() const
{
	const ImVec2 CanvasSize = ImGui::GetContentRegionAvail();
	if (CanvasSize.x <= 1.0f || CanvasSize.y <= 1.0f)
	{
		return;
	}

	const ImVec2 CanvasMin = ImGui::GetCursorScreenPos();
	const ImVec2 CanvasMax(CanvasMin.x + CanvasSize.x, CanvasMin.y + CanvasSize.y);

	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	DrawList->AddRectFilled(CanvasMin, CanvasMax, IM_COL32(20, 20, 20, 255));
	DrawList->AddRect(CanvasMin, CanvasMax, IM_COL32(95, 95, 95, 255), 4.0f, 0, 2.0f);

	static constexpr const char* PlaceholderText = "FBX viewport rendering is not wired yet.";
	const ImVec2 TextSize = ImGui::CalcTextSize(PlaceholderText);
	const ImVec2 TextPos(CanvasMin.x + (CanvasSize.x - TextSize.x) * 0.5f,
		CanvasMin.y + (CanvasSize.y - TextSize.y) * 0.5f);
	DrawList->AddText(TextPos, IM_COL32(200, 200, 200, 255), PlaceholderText);

	ImGui::InvisibleButton("FbxViewportPlaceholder", CanvasSize);
}
