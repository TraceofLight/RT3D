#include "pch.h"
#include "SceneRenderer.h"

#include "RenderPass/Public/BasePass.h"
#include "RenderPass/Public/DepthPrePass.h"
#include "RenderPass/Public/DebugPass.h"
#include "RenderPass/Public/RenderPass.h"
#include "SceneView/Public/SceneView.h"
#include "SceneViewFamily/Public/SceneViewFamily.h"

// 전역 RHI 인스턴스
D3D11RHI* FSceneRenderer::GlobalRHI = nullptr;

FSceneRenderer* FSceneRenderer::CreateSceneRenderer(const FSceneViewFamily& InViewFamily)
{
    return new FSceneRenderer(InViewFamily);
}

FSceneRenderer::FSceneRenderer(const FSceneViewFamily& InViewFamily)
    : ViewFamily(&InViewFamily)
{
    CreateDefaultRenderPasses();
}

FSceneRenderer::~FSceneRenderer()
{
    Cleanup();
}

void FSceneRenderer::Render()
{
    if (!ViewFamily || !ViewFamily->IsValid() || !GlobalRHI)
    {
        return;
    }

    // ViewFamily에서 Views 추출
    const TArray<FSceneView*>& Views = ViewFamily->GetViews();

    for (FSceneView* SceneView : Views)
    {
        if (SceneView)
        {
            RenderView(SceneView);
        }
    }
}

void FSceneRenderer::RenderView(const FSceneView* InSceneView)
{
    if (!InSceneView)
    {
        return;
    }

    // 각 렌더 패스 실행
    for (IRenderPass* Pass : RenderPasses)
    {
        if (Pass && Pass->IsEnabled())
        {
            Pass->Execute(InSceneView, this);
        }
    }
}

void FSceneRenderer::Cleanup()
{
    for (IRenderPass* Pass : RenderPasses)
    {
        if (Pass)
        {
            Pass->Cleanup();
            delete Pass;
        }
    }

    RenderPasses.Empty();
}

void FSceneRenderer::CreateDefaultRenderPasses()
{
    // 기본 렌더 패스들 생성 및 초기화
    FDepthPrePass* DepthPass = new FDepthPrePass();
    DepthPass->Initialize();
    RenderPasses.Add(DepthPass);

    FBasePass* BasePass = new FBasePass();
    BasePass->Initialize();
    RenderPasses.Add(BasePass);

    FDebugPass* DebugPass = new FDebugPass();
    DebugPass->Initialize();
    RenderPasses.Add(DebugPass);
}
