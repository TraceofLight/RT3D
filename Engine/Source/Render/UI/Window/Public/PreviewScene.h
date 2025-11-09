#pragma once

#include "Global/Types.h"

class AActor;
class ADirectionalLight;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class ULightComponent;

/**
 * @brief Lightweight scene container used by the FBX viewport preview.
 *        Owns temporary actors/components created for inspection and exposes
 *        raw arrays that can be consumed by the renderer when the feature is
 *        fully implemented.
 */
class FPreviewScene
{
public:
	FPreviewScene() = default;
	~FPreviewScene();

	void Reset();
	bool HasRenderableContent() const;

	AActor* SkeletalActor = nullptr;
	AActor* GroundActor = nullptr;
	ADirectionalLight* Sun = nullptr;

	TArray<UStaticMeshComponent*> StaticComps;
	TArray<USkeletalMeshComponent*> SkelComps;
	TArray<ULightComponent*> Lights;
};
