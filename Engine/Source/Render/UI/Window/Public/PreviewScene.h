#pragma once

#include "Global/Types.h"

class UObject;
class UWorld;
class AActor;
class UStaticMeshComponent;
class UAmbientLightComponent;

/**
 * @brief Lightweight helper that owns the mini preview world used by FBX viewport windows.
 *        Handles world lifecycle plus any temporary actors that should live only inside that preview world.
 */
class FPreviewScene
{
public:
	FPreviewScene() = default;
	~FPreviewScene();

	bool Initialize(UObject* InOuter);
	void Shutdown();
	void Tick(float DeltaTime);

	UWorld* GetWorld() const { return PreviewWorld; }

private:
	void CreatePreviewWorld();
	void DestroyPreviewWorld();
	void InjectDefaultContent();
	void RemoveInjectedContent();

	UObject* Outer = nullptr;
	UWorld* PreviewWorld = nullptr;
	AActor* PreviewActor = nullptr;
	UStaticMeshComponent* PreviewMesh = nullptr;
	UAmbientLightComponent* PreviewAmbient = nullptr;
	bool bContentInjected = false;
};
