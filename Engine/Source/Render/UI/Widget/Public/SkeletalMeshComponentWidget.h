#pragma once
#include "Render/UI/Widget/Public/Widget.h"

class UTexture;
class USkeletalMeshComponent;
class UDirectionalLightComponent;
class UMaterial;
class UWorld;
class UFbxViewportWindow;
class USkeletalMesh;

struct FSkeleton;

UCLASS()
class USkeletalMeshComponentWidget : public UWidget
{
	GENERATED_BODY()
	DECLARE_CLASS(USkeletalMeshComponentWidget, UWidget)

public:
	void Initialize() override;
	void Update() override {}
	void RenderWidget() override;

	void SetTargetWorld(UWorld* InWorld);
	void SetTargetComponent(USkeletalMeshComponent* InComponent);
	void SetPreviewViewportClient(FViewportClient* InClient) { PreviewClient = InClient; }

	// FbxViewportWindow 연동 (양방향 본 선택)
	void SetOwningFbxViewportWindow(UFbxViewportWindow* InWindow) { OwningFbxViewportWindow = InWindow; }

	// FbxViewportWindow 전용 하이라이팅 (전역 선택과 독립)
	void SetHighlightedBoneIndex(int32 BoneIdx) { HighlightedBoneIndex = BoneIdx; }
	int32 GetHighlightedBoneIndex() const { return HighlightedBoneIndex; }

	UDirectionalLightComponent*  FindFirstDirectional(UWorld* TargetWorld) const;

	void RenderPreviewTopControls(UWorld* TargetWorld, USkeletalMeshComponent* TargetComponent);
	void RenderBoneHierachy(USkeletalMeshComponent* SkeletalMeshComponent);
private:
	USkeletalMeshComponent* SkeletalMeshComponent{};
	USkeletalMeshComponent* OverrideTargetComponent = nullptr;
	FName SelectedBoneName;
	uint32 SelectedBoneIdx = -1;

	// FbxViewportWindow 전용 하이라이팅 (전역 선택과 독립)
	int32 HighlightedBoneIndex = -1;

	UWorld* World = nullptr;
	FViewportClient* PreviewClient = nullptr; /* FBX Viewport의 카메라 속도 */
	UFbxViewportWindow* OwningFbxViewportWindow = nullptr; /* 양방향 본 선택 연동용 */

	// 섹션별 렌더링을 위한 헬퍼 함수
	void RenderSkeletalMeshSelector();
	void RenderMaterialSections();
	void RenderAvailableMaterials(int32 TargetSlotIndex) const;

	void DrawSkeletalBone(FSkeleton* Skeleton, int idx);
	void OpenFbxPreviewViewport(USkeletalMesh* SkeletalMesh);

	// 유틸리티 함수
	static FString GetMaterialDisplayName(UMaterial* Material);
	static UTexture* GetPreviewTextureForMaterial(const UMaterial* Material);
};
