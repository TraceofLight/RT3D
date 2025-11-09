#include "pch.h"
#include "Render/UI/Window/Public/PreviewScene.h"
#include "Actor/Public/Actor.h"
#include "Actor/Public/DirectionalLight.h"

FPreviewScene::~FPreviewScene()
{
	Reset();
}

void FPreviewScene::Reset()
{
	StaticComps.Empty();
	SkelComps.Empty();
	Lights.Empty();

	SafeDelete(SkeletalActor);
	SafeDelete(GroundActor);
	SafeDelete(Sun);

	SkeletalActor = nullptr;
	GroundActor = nullptr;
	Sun = nullptr;
}

bool FPreviewScene::HasRenderableContent() const
{
	return !StaticComps.IsEmpty() || !SkelComps.IsEmpty() || !Lights.IsEmpty();
}
