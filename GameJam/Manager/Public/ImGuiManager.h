#pragma once
#include "Render/Public/Renderer.h"

class FImGuiManager
{
public:
	static void InitializeImGui(HWND InWindowHandle, const URenderer& InRenderer);
	static void ReleaseImGui();
	static void RenderImGui();
};
