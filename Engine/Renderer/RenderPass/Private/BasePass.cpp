#include "pch.h"
#include "../Public/BasePass.h"

#include "CameraActor.h"
#include "FViewport.h"
#include "../Public/RenderPass.h"
#include "../../SceneView/Public/SceneView.h"
#include "../../SceneRenderer.h"
#include "../../RenderCommand/Public/RHICommandList.h"
#include "../../../World.h"
#include "../../../RHIDevice.h"
#include "../../../SelectionManager.h"

// Cast<> 템플릿을 위한 필요한 클래스 헤더들
#include "../../../StaticMeshActor.h"
#include "../../../GridActor.h"
#include "../../../TextRenderComponent.h"
#include "../../../AABoundingBoxComponent.h"
#include "../../../ActorComponent.h"
#include "../../../PrimitiveComponent.h"

FBasePass::~FBasePass()
{
}

void FBasePass::Initialize()
{
    // BasePass 초기화 작업
}

void FBasePass::Cleanup()
{
    // BasePass 정리 작업
}

void FBasePass::Execute(const FSceneView* View, FSceneRenderer* SceneRenderer)
{
    if (!View || !SceneRenderer) return;

    UWorld* World = View->GetWorld();
    if (!World) {
        printf("[BasePass] World is null!\n");
        return;
    }

    FRHICommandList* RHICmdList = SceneRenderer->GetCommandList();
    if (!RHICmdList) return;

    // 뷰 매트릭스 계산
    ACameraActor* Camera = View->GetCamera();
    FViewport* Viewport = View->GetViewport();

    if (!Camera || !Viewport) return;

    float ViewportAspectRatio = static_cast<float>(Viewport->GetSizeX()) / static_cast<float>(
        Viewport->GetSizeY());
    if (Viewport->GetSizeY() == 0) ViewportAspectRatio = 1.0f;

    FMatrix ViewMatrix = Camera->GetViewMatrix();
    FMatrix ProjectionMatrix = Camera->GetProjectionMatrix(ViewportAspectRatio, Viewport);

    // 뷰 모드 설정 (CommandList로 처리)
    EViewModeIndex ViewModeIndex = View->GetViewModeIndex();
    // TODO: ViewMode Command 추가 예정
    
    FVector HighlightColor(1.0f, 1.0f, 1.0f);

    // === 라인 배치 시작 ===
    // TODO: 라인 배치는 별도 RenderPass로 분리할 예정

    // === 일반 액터들 렌더링 ===
    // Primitives Show Flag 체크
    if (World->IsShowFlagEnabled(EEngineShowFlags::SF_Primitives))
    {
        const TArray<AActor*>& Actors = World->GetActors();
        for (AActor* Actor : Actors)
        {
            if (!Actor) continue;
            if (Actor->GetActorHiddenInGame()) continue;

            // StaticMesh Show Flag 체크
            if (Cast<AStaticMeshActor>(Actor) && !World->IsShowFlagEnabled(
                EEngineShowFlags::SF_StaticMeshes))
                continue;

            RenderActor(Actor, View, RHICmdList, ViewMatrix, ProjectionMatrix, HighlightColor);
        }
    }

    // === 엔진 액터들 (그리드 등) 렌더링 ===
    const TArray<AActor*>& EngineActors = World->GetEngineActors();
    for (AActor* EngineActor : EngineActors)
    {
        if (!EngineActor) continue;
        if (EngineActor->GetActorHiddenInGame()) continue;

        // Grid Show Flag 체크
        if (Cast<AGridActor>(EngineActor) && !World->IsShowFlagEnabled(EEngineShowFlags::SF_Grid))
        {
            continue;
        }

        RenderActor(EngineActor, View, RHICmdList, ViewMatrix, ProjectionMatrix, HighlightColor);
    }
}

void FBasePass::RenderActor(AActor* Actor, const FSceneView* View, FRHICommandList* RHICmdList,
                            const FMatrix& ViewMatrix, const FMatrix& ProjectionMatrix,
                            const FVector& HighlightColor)
{
    if (!Actor || !View || !RHICmdList)
    {
        return;
    }

    UWorld* World = View->GetWorld();

    // 선택 상태 확인
    bool bIsSelected = USelectionManager::GetInstance().IsActorSelected(Actor);

    // 하이라이트 상수 버퍼 업데이트 (CommandList로 처리)
    // TODO: UpdateHighlightBuffers Command 추가

    // 액터의 모든 컴포넌트 렌더링
    for (USceneComponent* Component : Actor->GetComponents())
    {
        if (!Component) continue;

        if (UActorComponent* ActorComp = Cast<UActorComponent>(Component))
        {
            if (!ActorComp->IsActive()) continue;
        }

        // Text Render Component Show Flag 체크
        if (Cast<UTextRenderComponent>(Component) && !World->IsShowFlagEnabled(
            EEngineShowFlags::SF_BillboardText))
            continue;

        // Bounding Box Show Flag 체크  
        if (Cast<UAABoundingBoxComponent>(Component) && !World->IsShowFlagEnabled(
            EEngineShowFlags::SF_BoundingBoxes))
            continue;

        if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
        {
            RenderPrimitiveComponent(Primitive, View, RHICmdList, ViewMatrix, ProjectionMatrix);
        }
    }

    // 블렌드 스테이트 종료 (CommandList로 처리)
    RHICmdList->SetBlendState(false);
}

void FBasePass::RenderPrimitiveComponent(UPrimitiveComponent* Component, const FSceneView* View,
                                         FRHICommandList* RHICmdList, const FMatrix& ViewMatrix,
                                         const FMatrix& ProjectionMatrix)
{
    if (!Component || !View || !RHICmdList)
    {
        return;
    }

    // 뷰 모드 설정 (CommandList로 처리)
    // TODO: ViewMode Command 추가

    // Primitive Component 렌더링 (CommandList로 처리)
    RHICmdList->DrawIndexedPrimitive(Component, ViewMatrix, ProjectionMatrix);

    // 깊이 스텐실 상태 복원 (CommandList로 처리)
    RHICmdList->SetDepthStencilState(EComparisonFunc::LessEqual);
}
