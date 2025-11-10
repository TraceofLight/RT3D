#include "pch.h"
#include "Render/UI/Window/Public/PreviewScene.h"
#include "Core/Public/NewObject.h"
#include "Level/Public/World.h"
#include "Level/Public/Level.h"
#include "Actor/Public/StaticMeshActor.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Public/AmbientLightComponent.h"
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
    	PreviewMesh->SetStaticMesh("Data/Capsule.obj");
    	PreviewMesh->SetRelativeScale3D(FVector(10.0, 10.0, 10.0));
    	FVector Location = PreviewMesh->GetRelativeLocation();
    	UE_LOG("UStaticMeshComponent : %f, %f, %f", Location.X, Location.Y, Location.Z);
    }

    PreviewAmbient = Cast<UAmbientLightComponent>(PreviewActor->AddComponent(UAmbientLightComponent::StaticClass()));
    if (PreviewAmbient)
    {
        PreviewAmbient->SetLightEnabled(true);
        PreviewAmbient->SetVisible(true);
        PreviewAmbient->SetLightColor(FVector(1.0, 1.0, 1.0));

    	FVector Location = PreviewAmbient->GetRelativeLocation();
    	UE_LOG("Ambient : %f, %f, %f", Location.X, Location.Y, Location.Z);
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
    PreviewAmbient = nullptr;
    bContentInjected = false;
}
