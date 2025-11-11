#pragma once
#include "Render/UI/Window/Public/UIWindow.h"
#include "Render/UI/Viewport/Public/Viewport.h"

/**
 * @brief Popup window that renders the shared editor world with an independent viewport client.
 *        Currently mirrors the main viewport output inside an ImGui window.
 */
class FPreviewScene;
class USkeletalMeshComponentWidget;
class USkeletalMesh;

UCLASS()
class UFbxViewportWindow : public UUIWindow
{
	GENERATED_BODY()
	DECLARE_CLASS(UFbxViewportWindow, UUIWindow)

public:
	UFbxViewportWindow();
	void Initialize() override;
	void Cleanup() override;

	void LoadFbxFile(const path& File);
	void SetPreviewSkeletalMesh(USkeletalMesh* SkeletalMesh);

	// Bone Selection (PreviewScene 전용)
	void SelectBone(int32 BoneIndex);
	void DeselectBone();
	int32 GetSelectedBoneIndex() const { return SelectedBoneIndex; }

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
	void EnsurePreviewInfrastructure();
	void RenderPreviewViewport(const ImVec2& InSize);
	void RenderSkeletalInspector(const ImVec2& InSize);
	void UpdateSkeletalWidgetTargets();

	FViewport*       PreviewViewport   = nullptr;
	FViewportClient* PreviewClient     = nullptr;
	FPreviewScene*   PreviewScene      = nullptr;
	USkeletalMeshComponentWidget* SkeletalWidget = nullptr;

	bool bPreviewReady = false;
	bool bHovered = false;

	// Bone Selection (메인 에디터와 독립)
	int32 SelectedBoneIndex = -1;
	class UBoneTransformProxy* BoneTransformProxy = nullptr;
};

