#pragma once
#include "Render/Public/Renderer.h"

// FIXME(KHJ): Singleton 패턴화해서 구현되면 적용할 것
class FImGuiManager
{
public:
	static void InitializeImGui(HWND InWindowHandle, const URenderer& InRenderer);
	static void ReleaseImGui();
	static void RenderLobbyGui();
	static void RenderImGui();
	static void RenderGameGui();
};
