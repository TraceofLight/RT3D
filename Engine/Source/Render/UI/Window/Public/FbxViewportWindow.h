#pragma once
#include "Render/UI/Window/Public/UIWindow.h"

/**
 * @brief Placeholder popup window for the FBX viewport viewer.
 *        Rendering is stubbed for now until the viewport manager hookup is ready.
 */
UCLASS()
class UFbxViewportWindow : public UUIWindow
{
	GENERATED_BODY()
	DECLARE_CLASS(UFbxViewportWindow, UUIWindow)

public:
	UFbxViewportWindow();
	void Initialize() override;

protected:
	void OnPostRenderWindow() override;

private:
	void RenderPlaceholderViewport() const;
};
