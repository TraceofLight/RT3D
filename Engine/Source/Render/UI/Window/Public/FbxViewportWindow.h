#pragma once
#include "Render/UI/Window/Public/UIWindow.h"
#include "Render/UI/Viewport/Public/Viewport.h"

/**
 * @brief Popup window that renders the shared editor world with an independent viewport client.
 *        Currently mirrors the main viewport output inside an ImGui window.
 */
class FPreviewScene;
class FPreviewViewportClient;
class USkeletalMeshComponentWidget;
class USkeletalMesh;
class UBatchLines;
class UViewportControlWidget;

enum class EEditMode : uint8
{
	None,
	BoneEdit
};

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

	// Edit Mode
	void SetEditMode(EEditMode InMode) { CurrentEditMode = InMode; }
	EEditMode GetEditMode() const { return CurrentEditMode; }

	// Bone Selection (PreviewScene 전용)
	void SelectBone(int32 BoneIndex);
	void DeselectBone();
	int32 GetSelectedBoneIndex() const { return SelectedBoneIndex; }

	// Viewport State
	bool IsViewportHovered() const { return bHovered; }

protected:
	void OnPostRenderWindow() override;

private:
	ComPtr<ID3D11Texture2D>          ColorRT;
	ComPtr<ID3D11RenderTargetView>   RTV;
	ComPtr<ID3D11ShaderResourceView> SRV;
	ComPtr<ID3D11Texture2D>          DepthTex;
	ComPtr<ID3D11DepthStencilView>   DSV;
	ImVec2 CachedSize = ImVec2(0,0);

	// HitProxy용 RenderTarget
	ComPtr<ID3D11Texture2D>          HitProxyRT;
	ComPtr<ID3D11RenderTargetView>   HitProxyRTV;
	ComPtr<ID3D11Texture2D>          HitProxyStagingTex;

	void EnsureRenderTargets(const ImVec2& size);
	void EnsurePreviewInfrastructure();
	void RenderPreviewViewport(const ImVec2& InSize);
	void RenderSkeletalInspector(const ImVec2& InSize);
	void UpdateSkeletalWidgetTargets();
	void HandleMouseClick(const ImVec2& LocalMousePos);
	void UpdatePreviewCamera(float DeltaTime);

	// Material Instancing
	void CreateMaterialInstances();
	void CleanupMaterialInstances();

	FViewport*       PreviewViewport   = nullptr;
	FPreviewViewportClient* PreviewClient     = nullptr;
	FPreviewScene*   PreviewScene      = nullptr;
	USkeletalMeshComponentWidget* SkeletalWidget = nullptr;
	UViewportControlWidget* ViewportControlWidget = nullptr;

	// Material Instancing
	TArray<class UMaterial*> MaterialInstances;

	bool bPreviewReady = false;
	bool bHovered = false;

	// Edit Mode
	EEditMode CurrentEditMode = EEditMode::None;

	// Bone Selection (메인 에디터와 독립)
	int32 SelectedBoneIndex = -1;
	class UBoneTransformProxy* BoneTransformProxy = nullptr;
private:
	// --- Splitter/Inspector state ---
	float InspectorWidth = 320.0f;     // 현재 인스펙터 폭(px)
	float InspectorMinWidth = 220.0f;  // 최소 폭
	float InspectorMaxWidth = 800.0f;  // 최대 폭(윈도 폭에 따라 클램프됨)
	float InspectorPrevWidth = 320.0f; // 토글 복구용
	bool  bInspectorVisible = true;    // 인스펙터 표시 여부
	float SplitterThickness = 6.0f;    // 스플리터 핸들 두께(px)
};

