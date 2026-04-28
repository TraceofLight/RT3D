#include "pch.h"
#include "Renderer/RenderPass/Public/DepthPrePass.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/SceneView/Public/SceneView.h"
#include "World.h"
#include "Actor.h"
#include "PrimitiveComponent.h"
#include "CameraActor.h"
#include "FViewport.h"

void FDepthPrePass::Execute(const FSceneView* View, FSceneRenderer* SceneRenderer)
{
    if (!View || !SceneRenderer) return;
    
    URHIDevice* RHI = FSceneRenderer::GetGlobalRHI();
    if (!RHI) return;
    
    UWorld* World = View->GetWorld();
    if (!World) return;
    
    ACameraActor* Camera = View->GetCamera();
    FViewport* Viewport = View->GetViewport();
    if (!Camera || !Viewport) return;
    
    // Depth Pre-Pass 설정
    // TODO: Depth Write/Color Write 설정 구현 필요
    // RHI->OMSetDepthWriteEnabled(true);
    // RHI->OMSetColorWriteEnabled(false);
    
    // 뷰 행렬과 투영 행렬 계산
    float ViewportAspectRatio = static_cast<float>(Viewport->GetSizeX()) / static_cast<float>(Viewport->GetSizeY());
    if (Viewport->GetSizeY() == 0) ViewportAspectRatio = 1.0f;
    
    FMatrix ViewMatrix = Camera->GetViewMatrix();
    FMatrix ProjectionMatrix = Camera->GetProjectionMatrix(ViewportAspectRatio, Viewport);
    
    // 모든 액터들의 깊이 정보만 렌더링
    const TArray<AActor*>& Actors = World->GetActors();
    for (AActor* Actor : Actors)
    {
        if (Actor && !Actor->GetActorHiddenInGame())
        {
            RenderActorDepth(Actor, View, SceneRenderer);
        }
    }
    
    // Depth Pre-Pass 완료 후 컬러 쓰기 다시 활성화
    // TODO: Color Write 재활성화 구현 필요
    // RHI->OMSetColorWriteEnabled(true);
}

void FDepthPrePass::RenderActorDepth(AActor* Actor, const FSceneView* View, FSceneRenderer* SceneRenderer)
{
    if (!Actor || !View || !SceneRenderer) return;
    
    URHIDevice* RHI = FSceneRenderer::GetGlobalRHI();
    if (!RHI) return;
    
    // 액터의 프리미티브 컴포넌트들을 깊이만 렌더링
    for (USceneComponent* Component : Actor->GetComponents())
    {
        if (!Component) continue;
        
        if (UActorComponent* ActorComp = Cast<UActorComponent>(Component))
        {
            if (!ActorComp->IsActive()) continue;
        }
        
        if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
        {
            // 깊이만 렌더링하도록 설정된 상태에서 프리미티브 렌더링
            // TODO: Primitive 컴포넌트의 깊이 전용 렌더링 구현
            // Primitive->RenderDepthOnly(RHI, ViewMatrix, ProjectionMatrix);
        }
    }
}
