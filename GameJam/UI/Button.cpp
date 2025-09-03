#include "pch.h"
#include "Button.h"
#include "UIManager.h"
#include "../Render/Public/Renderer.h"

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
	FConstants fc;
	fc.Offset = position;
	fc.ScaleX = size.x;
	fc.ScaleY = size.y;
	URenderer::GetInstance().UpdateConstant(position, size.x);
	URenderer::GetInstance().RenderRectangle();
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
