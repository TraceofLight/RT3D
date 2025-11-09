#pragma once

class UConsoleWindow;
class UControlPanelWindow;
class UExperimentalFeatureWindow;
class UPerformanceWindow;
class UOutlinerWindow;
class UCameraPanelWindow;
class UDetailWindow;
class UMainMenuWindow;
class UEditorWindow;
class UViewportClientWindow;
class ULevelTabBarWindow;
class UStatusBarWidget;
class UCurveEditorWindow;
class UFbxViewportWindow;

/**
 * @brief UI 윈도우 도킹 방향
 */
enum class EUIDockDirection : uint8_t
{
	None,
	Left,
	Right,
	Top,
	Bottom,
	BottomLeft,
	Center,
};

/**
 * @brief UI 윈도우들을 쉽게 생성하기 위한 팩토리 클래스
 */
class UUIWindowFactory
{
public:
	static void CreateDefaultUILayout();
	static UMainMenuWindow& CreateMainMenuWindow();
	static UStatusBarWidget& CreateStatusBarWidget();
	static UConsoleWindow* CreateConsoleWindow(EUIDockDirection InDockDirection = EUIDockDirection::BottomLeft);
	static UControlPanelWindow* CreateControlPanelWindow(EUIDockDirection InDockDirection = EUIDockDirection::Left);
	static UOutlinerWindow* CreateOutlinerWindow(EUIDockDirection InDockDirection = EUIDockDirection::Center);
	static ULevelTabBarWindow* CreateLevelTabBarWindow();
	static UDetailWindow* CreateDetailWindow(EUIDockDirection InDockDirection = EUIDockDirection::Right);
	static UExperimentalFeatureWindow* CreateExperimentalFeatureWindow(EUIDockDirection InDockDirection = EUIDockDirection::Right);
	static UEditorWindow* CreateEditorWindow(EUIDockDirection InDockDirection = EUIDockDirection::None);
	static UViewportClientWindow* CreateViewportClientWindow(EUIDockDirection InDockDirection = EUIDockDirection::None);
	static UCurveEditorWindow* CreateCurveEditorWindow(EUIDockDirection InDockDirection = EUIDockDirection::None);
	static UFbxViewportWindow* CreateFbxViewportWindow(EUIDockDirection InDockDirection = EUIDockDirection::None);
};
