#pragma once
#include "Render/UI/Viewport/Public/ViewportClient.h"

class UGizmo;
class FPreviewScene;

/**
 * @brief FViewportClient for preview windows (FBX Viewer, Material Editor, etc.)
 * Provides gizmo support and object manipulation for standalone preview viewports
 */
class FPreviewViewportClient : public FViewportClient
{
public:
	FPreviewViewportClient();
	~FPreviewViewportClient();

	// Gizmo Access
	UGizmo* GetGizmo() override { return Gizmo; }
	void SetGizmo(UGizmo* InGizmo) { Gizmo = InGizmo; }
	bool UsesTransformGizmo() const override { return Gizmo != nullptr; }

	// Input Handlers
	bool InputKey(EKeyInput Key, bool bPressed) override;
	bool HandleClick(int32 MouseX, int32 MouseY) override;
	bool ProcessGizmoDrag(const FVector2& MouseDelta) override;

	// Preview Scene
	void SetPreviewScene(FPreviewScene* InScene) { PreviewScene = InScene; }
	FPreviewScene* GetPreviewScene() const { return PreviewScene; }

private:
	UGizmo* Gizmo = nullptr;
	FPreviewScene* PreviewScene = nullptr;

	// Drag state
	bool bIsDragging = false;
	FVector DragStartMouseLocation;
};
