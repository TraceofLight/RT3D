#include "pch.h"
#include "Render/UI/Window/Public/PreviewScene.h"

#include "Runtime/CoreUObject/Public/NewObject.h"
#include "Level/Public/World.h"
#include "Level/Public/Level.h"
#include "Actor/Public/Actor.h"
#include "Actor/Public/SkeletalMeshActor.h"
#include "Component/Mesh/Public/SkeletalMeshComponent.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Component/Public/DirectionalLightComponent.h"
#include "Editor/Public/EditorEngine.h"
#include "Editor/Public/Gizmo.h"
#include "Editor/Public/BatchLines.h"

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
    PreviewWorld->CreateNewLevel("FBX", false);

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

	PreviewSkeletalActor  = PreviewWorld->SpawnActor(ASkeletalMeshActor::StaticClass());
	if (!PreviewSkeletalActor)
	{
		UE_LOG_ERROR("PreviewScene: failed to spawn preview fbx model.");
		return;
	}
	else
	{
		PreviewSkeletalActor->SetActorLocation(FVector(0,0,2));
		PreviewSkeletalActor->SetActorScale3D(FVector(0.2f, 0.2f, 0.2f));
	}

	PreviewSkeletal = Cast<USkeletalMeshComponent>(PreviewSkeletalActor->GetRootComponent());
	if (!PreviewSkeletal)
	{
		UE_LOG_ERROR("PreviewScene: skeletal mesh actor does not have a valid skeletal component.");
		return;
	}

	PreviewSkeletal->SetVisibility(true);

    PreviewBackgroundActor = PreviewWorld->SpawnActor(AActor::StaticClass());
    if (!PreviewBackgroundActor)
    {
        UE_LOG_ERROR("PreviewScene: failed to spawn preview actor.");
        return;
    }

    PreviewMesh = Cast<UStaticMeshComponent>(PreviewBackgroundActor->AddComponent(UStaticMeshComponent::StaticClass()));
    if (PreviewMesh)
    {
        PreviewMesh->SetVisibility(true);
    	PreviewMesh->SetStaticMesh("Data/Shapes/Cube.obj");
    	PreviewMesh->SetRelativeScale3D(FVector(100, 100, 2));
    	FVector Location = PreviewMesh->GetRelativeLocation();
    	UE_LOG("UStaticMeshComponent : %f, %f, %f", Location.X, Location.Y, Location.Z);
    }

	PreviewDirectional = Cast<UDirectionalLightComponent>(PreviewBackgroundActor->AddComponent(UDirectionalLightComponent::StaticClass()));
	if (PreviewDirectional)
	{
		PreviewDirectional->SetLightEnabled(true);
		PreviewDirectional->SetVisible(true);
		PreviewDirectional->SetLightColor(FVector(1.0, 1.0, 1.0));
		PreviewDirectional->SetRelativeRotation(FQuat::FromEuler(FVector( 0.f, -50.f, 0.f)));
		PreviewDirectional->SetRelativeLocation(FVector(0, 0, 10000));
		PreviewDirectional->SetIntensity(1.0);
	}

	// PreviewGizmo 생성 (메인 에디터와 독립)
	if (!PreviewGizmo)
	{
		PreviewGizmo = NewObject<UGizmo>(Outer);
	}

	// PreviewBatchLines 생성 (UObject이므로 Level에 등록하지 않음)
	if (!PreviewBatchLines)
	{
		PreviewBatchLines = new UBatchLines();
	}

    bContentInjected = true;
}

void FPreviewScene::RemoveInjectedContent()
{
    if (!bContentInjected || !PreviewWorld)
    {
        return;
    }

    if (PreviewBackgroundActor)
    {
        PreviewWorld->DestroyActor(PreviewBackgroundActor);
    }

    PreviewBackgroundActor = nullptr;
    PreviewMesh = nullptr;

    if (PreviewGizmo)
    {
        SafeDelete(PreviewGizmo);
        PreviewGizmo = nullptr;
    }

    if (PreviewBatchLines)
    {
        delete PreviewBatchLines;
        PreviewBatchLines = nullptr;
    }

    bContentInjected = false;
}


