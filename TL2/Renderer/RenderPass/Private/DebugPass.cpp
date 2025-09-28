#include "pch.h"
#include "../Public/DebugPass.h"

#include "CameraActor.h"
#include "FViewport.h"
#include "../Public/RenderPass.h"
#include "../../SceneView/Public/SceneView.h"
#include "../../SceneRenderer.h"
#include "../../../World.h"
#include "../../../RHIDevice.h"

FDebugPass::~FDebugPass()
{
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

    // 그리드 액터 찾기
    AGridActor* GridActor = World->GetGridActor();
    if (!GridActor) return;

    // 그리드 렌더링
    // TODO: GridActor의 렌더링 로직을 RHI 기반으로 수정 필요
    // 현재는 URenderer를 사용하므로 임시로 비워둠
}

void FDebugPass::RenderGizmos(const FSceneView* View, FSceneRenderer* SceneRenderer)
{
    if (!View) return;

    UWorld* World = View->GetWorld();
    if (!World) return;

    // 기즈모 액터 찾기
    AGizmoActor* GizmoActor = World->GetGizmoActor();
    if (!GizmoActor) return;

    // 기즈모는 현재 FViewportClient에서 별도로 렌더링하고 있음
    // 나중에 통합할 수 있지만, 현재는 기존 방식 유지
}
