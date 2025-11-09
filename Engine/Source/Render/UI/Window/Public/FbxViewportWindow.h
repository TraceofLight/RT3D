#pragma once
#include "Render/UI/Window/Public/UIWindow.h"
#include "Render/UI/Viewport/Public/Viewport.h"

/**
 * @brief Popup window that renders the shared editor world with an independent viewport client.
 *        Currently mirrors the main viewport output inside an ImGui window.
 */
UCLASS()
class UFbxViewportWindow : public UUIWindow
{
	GENERATED_BODY()
	DECLARE_CLASS(UFbxViewportWindow, UUIWindow)

public:
	UFbxViewportWindow();
	void Initialize() override;
	void Release();
	void Tick(float DeltaTime);

	void LoadFbxFile(const path& File);

protected:
	void OnPostRenderWindow() override;

private:
	Microsoft::WRL::ComPtr<ID3D11Texture2D>          ColorRT;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   RTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV;
	Microsoft::WRL::ComPtr<ID3D11Texture2D>          DepthTex;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>   DSV;
	ImVec2 CachedSize = ImVec2(0,0);

	void EnsureRenderTargets(const ImVec2& size);
	FViewport*       PreviewViewport   = nullptr;
	FViewportClient* PreviewClient     = nullptr;

	bool bHovered = false;

	void RouteInputToClient();
};
