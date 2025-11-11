#pragma once

#include "Global/Types.h"

class USkeletalMeshComponent;
class UObject;
class UWorld;
class AActor;
class UStaticMeshComponent;
class UAmbientLightComponent;
class UDirectionalLightComponent;
class UGizmo;

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

	UWorld* GetWorld() const { return PreviewWorld; }
	USkeletalMeshComponent* GetPreviewSkeletalComponent() const { return PreviewSkeletal; }
	UGizmo* GetPreviewGizmo() const { return PreviewGizmo; }

private:
	void CreatePreviewWorld();
	void DestroyPreviewWorld();
	void InjectDefaultContent();
	void RemoveInjectedContent();

	UObject* Outer = nullptr;
	UWorld* PreviewWorld = nullptr;

	// Fbx
	AActor* PreviewSkeletalActor = nullptr;
	USkeletalMeshComponent* PreviewSkeletal = nullptr;

	// Background Props
	AActor* PreviewBackgroundActor = nullptr;
	UStaticMeshComponent* PreviewMesh = nullptr;
	UDirectionalLightComponent* PreviewDirectional = nullptr;
	bool bContentInjected = false;
	bool bWorldRegistered = false;

	// Gizmo (메인 에디터와 독립)
	UGizmo* PreviewGizmo = nullptr;
};

