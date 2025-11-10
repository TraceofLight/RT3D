#pragma once
#include "Render/UI/Widget/Public/Widget.h"

class UTexture;
class USkeletalMeshComponent;
class UMaterial;
class FSkeleton;

UCLASS()
class USkeletalMeshComponentWidget : public UWidget
{
	GENERATED_BODY()
	DECLARE_CLASS(USkeletalMeshComponentWidget, UWidget)

public:
	void Initialize() override {}
	void Update() override {}
	void RenderWidget() override;

private:
	USkeletalMeshComponent* SkeletalMeshComponent{};

	// 섹션별 렌더링을 위한 헬퍼 함수
	void RenderSkeletalMeshSelector() const;
	void RenderMaterialSections();
	void RenderAvailableMaterials(int32 TargetSlotIndex) const;

	void DrawSkeletalBone(FSkeleton* Skeleton, int idx) const;
	void RenderBoneHierachy(FSkeleton* Skeleton) const;

	// 유틸리티 함수
	static FString GetMaterialDisplayName(UMaterial* Material);
	static UTexture* GetPreviewTextureForMaterial(const UMaterial* Material);
};
