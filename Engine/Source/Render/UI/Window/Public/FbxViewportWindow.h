#pragma once
#include "Render/UI/Window/Public/UIWindow.h"
#include "Render/UI/Viewport/Public/Viewport.h"

/**
 * @brief Popup window that renders the shared editor world with an independent viewport client.
 *        Currently mirrors the main viewport output inside an ImGui window.
 */
class FPreviewScene;

UCLASS()
class UFbxViewportWindow : public UUIWindow
{
	GENERATED_BODY()
	DECLARE_CLASS(UFbxViewportWindow, UUIWindow)

public:
	UFbxViewportWindow();
	void Initialize() override;
	void Release() override;
	void Tick(float DeltaTime) override;

	void LoadFbxFile(const path& File);

protected:
	void OnPostRenderWindow() override;

private:
	ComPtr<ID3D11Texture2D>          ColorRT;
	ComPtr<ID3D11RenderTargetView>   RTV;
	ComPtr<ID3D11ShaderResourceView> SRV;
	ComPtr<ID3D11Texture2D>          DepthTex;
	ComPtr<ID3D11DepthStencilView>   DSV;
	ImVec2 CachedSize = ImVec2(0,0);

	void EnsureRenderTargets(const ImVec2& size);
	FViewport*       PreviewViewport   = nullptr;
	FViewportClient* PreviewClient     = nullptr;
	FPreviewScene*   PreviewScene      = nullptr;

	bool bHovered = false;
};
