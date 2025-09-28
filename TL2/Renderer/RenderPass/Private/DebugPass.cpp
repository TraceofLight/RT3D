#include "pch.h"
#include "../Public/DebugPass.h"

#include "CameraActor.h"
#include "FViewport.h"

// RenderCommand 헤더들
#include "SelectionManager.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/SceneView/Public/SceneView.h"
#include "Renderer\RenderCommand\Public\UpdateConstantBufferCommand.h"
#include "Renderer\RenderCommand\Public\UpdateHighlightBufferCommand.h"
#include "Renderer\RenderCommand\Public\SetViewModeCommand.h"
#include "Renderer\RenderCommand\Public\SetDepthStencilStateCommand.h"
#include "Renderer\RenderCommand\Public\SetBlendStateCommand.h"
#include "LineComponent.h"


FDebugPass::~FDebugPass()
{
    // Debug Pass 소멸자 - 필요한 경우 리소스 정리
}

void FDebugPass::Initialize()
{
    // DebugPass 초기화 작업
}

void FDebugPass::Cleanup()
{
    // DebugPass 정리 작업
}

void FDebugPass::Execute(const FSceneView* View, FSceneRenderer* SceneRenderer)
{
    if (!View || !SceneRenderer) return;

    UWorld* World = View->GetWorld();
    if (!World) return;

    URHIDevice* RHI = FSceneRenderer::GetGlobalRHI();
    if (!RHI) return;

    // 뷰 매트릭스 계산
    ACameraActor* Camera = View->GetCamera();
    FViewport* Viewport = View->GetViewport();

    if (!Camera || !Viewport) return;

    float ViewportAspectRatio = static_cast<float>(Viewport->GetSizeX()) / static_cast<float>(
        Viewport->GetSizeY());
    if (Viewport->GetSizeY() == 0) ViewportAspectRatio = 1.0f;

    FMatrix ViewMatrix = Camera->GetViewMatrix();
    FMatrix ProjectionMatrix = Camera->GetProjectionMatrix(ViewportAspectRatio, Viewport);

    // === 라인 배치 시작 ===
    BeginLineBatch();

    // === 그리드 렌더링 ===
    if (World->IsShowFlagEnabled(EEngineShowFlags::SF_Grid))
    {
        RenderGrid(View, SceneRenderer);
    }

    // === 기즈모 렌더링 ===
    // TODO: 기즈모는 별도 처리가 필요할 수 있음 (FViewportClient에서 별도 렌더링)
    RenderGizmos(View, SceneRenderer);

    // === 라인 배치 종료 ===
    EndLineBatch(FMatrix::Identity(), ViewMatrix, ProjectionMatrix);
}

void FDebugPass::BeginLineBatch()
{
    // TODO: RHI를 통한 라인 배치 시작
    // URenderer의 BeginLineBatch() 로직을 RHI로 이식
    URHIDevice* RHI = FSceneRenderer::GetGlobalRHI();
    if (!RHI) return;

    // 라인 배치 초기화 작업
}

void FDebugPass::EndLineBatch(const FMatrix& ModelMatrix, const FMatrix& ViewMatrix,
                              const FMatrix& ProjectionMatrix)
{
    // TODO: RHI를 통한 라인 배치 종료 및 렌더링
    // URenderer의 EndLineBatch() 로직을 RHI로 이식
    URHIDevice* RHI = FSceneRenderer::GetGlobalRHI();
    if (!RHI) return;

    // 모든 라인을 GPU로 전송하여 렌더링
}

void FDebugPass::RenderGrid(const FSceneView* View, FSceneRenderer* SceneRenderer)
{
    if (!View) return;

    UWorld* World = View->GetWorld();
    if (!World) return;

    URHIDevice* RHI = FSceneRenderer::GetGlobalRHI();
    if (!RHI) return;

    // 그리드 액터 찾기
    AGridActor* GridActor = World->GetGridActor();
    if (GridActor)
    {
        // GridActor의 LineComponent들을 처리
        RenderActorLines(GridActor, View, SceneRenderer);
    }

    // 모든 액터들의 LineComponent들을 처리
    const TArray<AActor*>& Actors = World->GetActors();
    for (AActor* Actor : Actors)
    {
        if (Actor && !Actor->GetActorHiddenInGame())
        {
            RenderActorLines(Actor, View, SceneRenderer);
        }
    }
}

void FDebugPass::RenderGizmos(const FSceneView* View, FSceneRenderer* SceneRenderer)
{
    if (!View) return;
    
    UWorld* World = View->GetWorld();
    if (!World) return;
    
    // 기즈모 액터 찾기
    AGizmoActor* GizmoActor = World->GetGizmoActor();
    if (!GizmoActor) return;
    
    // 선택된 액터가 있을 때만 기즈모 렌더링
    if (!USelectionManager::GetInstance().HasSelection()) return;
    
    URHIDevice* RHI = FSceneRenderer::GetGlobalRHI();
    if (!RHI) return;
    
    ACameraActor* Camera = View->GetCamera();
    FViewport* Viewport = View->GetViewport();
    if (!Camera || !Viewport) return;
    
    // 기즈모 컴포넌트들 가져오기
    TArray<USceneComponent*>* Components = GizmoActor->GetGizmoComponents();
    if (!Components) return;
    
    float ViewportAspectRatio = static_cast<float>(Viewport->GetSizeX()) / static_cast<float>(Viewport->GetSizeY());
    if (Viewport->GetSizeY() == 0) ViewportAspectRatio = 1.0f;
    
    FMatrix ViewMatrix = Camera->GetViewMatrix();
    FMatrix ProjectionMatrix = Camera->GetProjectionMatrix(ViewportAspectRatio, Viewport);
    EViewModeIndex ViewModeIndex = World->GetViewModeIndex();
    
    // 기즈모 컴포넌트들 렌더링
    for (int32 i = 0; i < Components->Num(); ++i)
    {
        USceneComponent* Component = (*Components)[i];
        if (!Component) continue;
        
        // 컴포넌트 활성 상태 확인
        if (UActorComponent* ActorComp = Cast<UActorComponent>(Component))
        {
            if (!ActorComp->IsActive()) continue;
        }
        
        FMatrix ModelMatrix = Component->GetWorldMatrix();
        FVector HighlightColor(1.0f, 1.0f, 1.0f);
        
        // RenderCommand를 통한 상수 버퍼 업데이트
        FRHIUpdateConstantBufferCommand UpdateConstantBufferCmd(RHI, ModelMatrix, ViewMatrix, ProjectionMatrix);
        UpdateConstantBufferCmd.Execute();
        
        // 기즈모 축 하이라이트 설정
        bool bIsHighlighted = (GizmoActor->GetGizmoAxis() == i + 1);
        FRHIUpdateHighlightBufferCommand UpdateHighlightCmd(RHI, bIsHighlighted, HighlightColor, i + 1);
        UpdateHighlightCmd.Execute();
        
        // 프리미티브 컴포넌트 렌더링
        if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
        {
            // 기즈모 렌더링을 위한 RHI 상태 설정 (RenderCommand 사용)
            FRHISetViewModeCommand SetViewModeCmd(RHI, EViewModeIndex::VMI_Unlit);
            SetViewModeCmd.Execute();
            
            FRHISetDepthStencilStateCommand SetDepthCmd(RHI, EComparisonFunc::Always);
            SetDepthCmd.Execute();
            
            FRHISetBlendStateCommand SetBlendCmd(RHI, true);
            SetBlendCmd.Execute();
            
            // TODO: Primitive 컴포넌트를 RHI로 렌더링
            // Primitive->RenderWithRHI(RHI, ViewMatrix, ProjectionMatrix);
            
            // 상태 복구 (RenderCommand 사용)
            FRHISetBlendStateCommand RestoreBlendCmd(RHI, false);
            RestoreBlendCmd.Execute();
            
            FRHISetDepthStencilStateCommand RestoreDepthCmd(RHI, EComparisonFunc::LessEqual);
            RestoreDepthCmd.Execute();
            
            FRHISetViewModeCommand RestoreViewModeCmd(RHI, ViewModeIndex);
            RestoreViewModeCmd.Execute();
        }
    }
    
    // 기즈모 렌더링 종료시 상태 설정 (RenderCommand 사용)
    FRHISetBlendStateCommand FinalBlendCmd(RHI, true); // 알파 블렌딩 유지
    FinalBlendCmd.Execute();
}

void FDebugPass::RenderActorLines(AActor* Actor, const FSceneView* View, FSceneRenderer* SceneRenderer)
{
    if (!Actor || !View || !SceneRenderer) return;
    
    URHIDevice* RHI = FSceneRenderer::GetGlobalRHI();
    if (!RHI) return;
    
    ACameraActor* Camera = View->GetCamera();
    FViewport* Viewport = View->GetViewport();
    if (!Camera || !Viewport) return;
    
    float ViewportAspectRatio = static_cast<float>(Viewport->GetSizeX()) / static_cast<float>(Viewport->GetSizeY());
    if (Viewport->GetSizeY() == 0) ViewportAspectRatio = 1.0f;
    
    FMatrix ViewMatrix = Camera->GetViewMatrix();
    FMatrix ProjectionMatrix = Camera->GetProjectionMatrix(ViewportAspectRatio, Viewport);
    
    // 액터의 LineComponent들을 찾아서 렌더링
    for (USceneComponent* Component : Actor->GetComponents())
    {
        if (!Component) continue;
        
        if (UActorComponent* ActorComp = Cast<UActorComponent>(Component))
        {
            if (!ActorComp->IsActive()) continue;
        }
        
        if (ULineComponent* LineComp = Cast<ULineComponent>(Component))
        {
            if (LineComp->HasVisibleLines())
            {
                // TODO: 라인 데이터를 가져와서 RHI로 렌더링
                // 현재는 GetWorldLineData()를 사용해서 라인 데이터 추출 가능
                TArray<FVector> StartPoints, EndPoints;
                TArray<FVector4> Colors;
                
                LineComp->GetWorldLineData(StartPoints, EndPoints, Colors);
                
                // TODO: RHI를 사용해서 라인들을 렌더링
                // 예: RHI->DrawLines(StartPoints, EndPoints, Colors, ViewMatrix, ProjectionMatrix);
            }
        }
    }
}
