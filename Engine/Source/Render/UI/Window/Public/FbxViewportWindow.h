#pragma once
#include "Render/UI/Window/Public/UIWindow.h"
#include "Render/UI/Viewport/Public/Viewport.h"

/**
 * @brief Popup window that renders the shared editor world with an independent viewport client.
 *        Currently clears the off-screen RT/DSV using the FBX viewport camera settings.
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
	// --- 전용 뷰포트/클라이언트 (GWorld 공유) ---
	FViewport*       PreviewViewport   = nullptr;
	FViewportClient* PreviewClient     = nullptr;

	// --- RT/DSV & SRV ---
	ComPtr<ID3D11Texture2D>          ColorRT;
	ComPtr<ID3D11RenderTargetView>   RTV;
	ComPtr<ID3D11ShaderResourceView> SRV;
	ComPtr<ID3D11Texture2D>          DepthTex;
	ComPtr<ID3D11DepthStencilView>   DSV;

	ImVec2 CachedViewportSize = ImVec2(0,0); // 윈도우 내부 사이즈 캐시
	bool   bHovered = false;                 // 입력 라우팅용

	void EnsureRenderTargets(const ImVec2& Size);
	void RenderPreview(); // URenderer 호출
	void RouteInputToClient(); // 창 위에 있을 때만 입력 라우팅

	void RenderPlaceholderViewport() const;
};
