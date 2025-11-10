#include "pch.h"
#include "Render/UI/Window/Public/PreviewScene.h"
#include "Core/Public/NewObject.h"
#include "Level/Public/World.h"
#include "Level/Public/Level.h"
#include "Actor/Public/StaticMeshActor.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Public/AmbientLightComponent.h"
#include "Component/Public/DirectionalLightComponent.h"
#include "Editor/Public/EditorEngine.h"

FPreviewScene::~FPreviewScene()
{
}

bool FPreviewScene::Initialize(UObject* InOuter)
{
    Outer = InOuter;
    if (!PreviewWorld)
    {
        CreatePreviewWorld();
    }

    if (!PreviewWorld)
    {
        return false;
    }

    InjectDefaultContent();
    return true;
}

void FPreviewScene::Shutdown()
{
    RemoveInjectedContent();
    DestroyPreviewWorld();
}

void FPreviewScene::CreatePreviewWorld()
{
    if (PreviewWorld)
    {
        return;
    }

    if (!Outer)
    {
        UE_LOG_ERROR("PreviewScene: cannot create preview world without a valid outer.");
        return;
    }

    PreviewWorld = NewObject<UWorld>(Outer);
    if (!PreviewWorld)
    {
        UE_LOG_ERROR("PreviewScene: failed to allocate preview world.");
        return;
    }

    PreviewWorld->SetWorldType(EWorldType::EditorPreview);
    PreviewWorld->CreateNewLevel();

    if (GEditor && !bWorldRegistered)
    {
        GEditor->RegisterPreviewWorld(PreviewWorld);
        bWorldRegistered = true;
    }
}

void FPreviewScene::DestroyPreviewWorld()
{
    if (!PreviewWorld)
    {
        return;
    }

    if (GEditor && bWorldRegistered)
    {
        GEditor->UnregisterPreviewWorld(PreviewWorld);
        bWorldRegistered = false;
        PreviewWorld = nullptr;
        return;
    }

    PreviewWorld->EndPlay();
    SafeDelete(PreviewWorld);
    PreviewWorld = nullptr;
}

void FPreviewScene::InjectDefaultContent()
{
    if (bContentInjected || !PreviewWorld)
    {
        return;
    }

    PreviewActor = PreviewWorld->SpawnActor(AStaticMeshActor::StaticClass());
    if (!PreviewActor)
    {
        UE_LOG_ERROR("PreviewScene: failed to spawn preview actor.");
        return;
    }

    PreviewMesh = Cast<UStaticMeshComponent>(PreviewActor->AddComponent(UStaticMeshComponent::StaticClass()));
    if (PreviewMesh)
    {
        PreviewMesh->SetVisibility(true);
    	PreviewMesh->SetStaticMesh("Data/Shapes/Cube.obj");
    	PreviewMesh->SetRelativeScale3D(FVector(100.0, 100.0, 1.0));
    	FVector Location = PreviewMesh->GetRelativeLocation();
    	UE_LOG("UStaticMeshComponent : %f, %f, %f", Location.X, Location.Y, Location.Z);
    }

	PreviewDirectional = Cast<UDirectionalLightComponent>(PreviewActor->AddComponent(UDirectionalLightComponent::StaticClass()));
	if (PreviewDirectional)
	{
		PreviewDirectional->SetLightEnabled(true);
		PreviewDirectional->SetVisible(true);
		PreviewDirectional->SetLightColor(FVector(1.0, 1.0, 1.0));
		PreviewDirectional->SetRelativeRotation(FQuaternion::FromEuler(FVector( 0.f, -20.f, 0.f)));

		FVector Location = PreviewDirectional->GetRelativeLocation();
		UE_LOG("Directional : %f, %f, %f", Location.X, Location.Y, Location.Z);
	}

    bContentInjected = true;
}

void FPreviewScene::RemoveInjectedContent()
{
    if (!bContentInjected || !PreviewWorld)
    {
        return;
    }

    if (PreviewActor)
    {
        PreviewWorld->DestroyActor(PreviewActor);
    }

    PreviewActor = nullptr;
    PreviewMesh = nullptr;
    bContentInjected = false;
}
