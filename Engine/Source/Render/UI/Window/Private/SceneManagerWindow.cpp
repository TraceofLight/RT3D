#include "pch.h"
#include "Render/UI/Window/Public/SceneManagerWindow.h"

#include "Render/UI/Widget/Public/SceneHierarchyWidget.h"

/**
 * @brief SceneManager Constructor
 * Scene 관리를 위한 적절한 크기의 윈도우 제공
 */
USceneManagerWindow::USceneManagerWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Scene Manager";
	Config.DefaultSize = ImVec2(300, 360);
	Config.DefaultPosition = ImVec2(10, 670);
	Config.MinSize = ImVec2(250, 300);
	Config.DockDirection = EUIDockDirection::Right;
	Config.Priority = 20;
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;

	Config.UpdateWindowFlags();
	SetConfig(Config);

	// SceneHierarchyWidget 추가
	AddWidget(new USceneHierarchyWidget());
}

/**
 * @brief 초기화 함수
 */
void USceneManagerWindow::Initialize()
{
	UE_LOG("SceneManagerWindow: Initialized");
}
