#include "pch.h"
#include "UI/Public/Button.h"
#include "Manager/Public/UIManager.h"
#include "Render/Public/Renderer.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

Button::Button(const string& InButtonText, function<void()> InCallback)
{
	name = InButtonText;
	ButtonText.SetText(InButtonText);
	OnClickFunction = InCallback;
}

void Button::Update(float InDeltaTime)
{
}

void Button::Render()
{
	float ButtonWidth = 150.0f;
	ImGuiStyle& Style = ImGui::GetStyle();
	float WindowWidth = ImGui::GetWindowSize().x;
	float CursorPositionX = (WindowWidth - ButtonWidth) * 0.5f;

	// 버튼 중앙 정렬
	ImGui::SetCursorPosX(CursorPositionX);

	if (ImGui::Button(name.c_str(), ImVec2(ButtonWidth, 0)))
	{
		OnClickFunction();
	}
}

bool Button::OnMouseClick(FVector3 InPosition)
{
	if (IsCursorInside(InPosition))
	{
		if (OnClickFunction)
		{
			OnClickFunction();
		}

		return true;
	}

	return false;
}
