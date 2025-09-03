#include "pch.h"
#include "UI/Public/Button.h"
#include "Manager/Public/UIManager.h"
#include "Render/Public/Renderer.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

Button::Button(const std::string& buttonText, std::function<void()> callback)
{
	name = buttonText;
	text.SetText(buttonText);
	onClick = callback;
}

void Button::Update(float deltaTime)
{
}

void Button::Render()
{
	//FConstants fc;
	//fc.Offset = position;
	//fc.ScaleX = size.x;
	//fc.ScaleY = size.y;
	//URenderer::GetInstance()->UpdateConstant(position, size.x);
	//URenderer::GetInstance()->RenderRectangle();
	if (ImGui::Button(name.c_str()))
	{
		onClick();
	}
}

bool Button::OnMouseClick(FVector3 pos)
{
	if (IsCursorInside(pos))
	{
		if (onClick)
		{
			onClick();
		}
		return true;
	}
	return false;
}
