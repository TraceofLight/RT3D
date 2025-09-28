#include "pch.h"
#include "Renderer/RenderCommand/Public/SetRenderTargetCommand.h"

#include "FViewport.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/SceneView/Public/SceneView.h"

void FRHISetRenderTargetCommand::Execute()
{
    if (!View)
        return;

    // 뷰포트의 렌더 타겟을 설정
    FViewport* Viewport = View->GetViewport();
    if (Viewport)
    {
        URHIDevice* RHI = FSceneRenderer::GetGlobalRHI();

        // D3D11 뷰포트 설정
        D3D11_VIEWPORT D3DViewport = {};
        D3DViewport.Width = static_cast<float>(Viewport->GetSizeX());
        D3DViewport.Height = static_cast<float>(Viewport->GetSizeY());
        D3DViewport.MinDepth = 0.0f;
        D3DViewport.MaxDepth = 1.0f;
        D3DViewport.TopLeftX = static_cast<float>(Viewport->GetStartX());
        D3DViewport.TopLeftY = static_cast<float>(Viewport->GetStartY());

        RHI->GetDeviceContext()->RSSetViewports(1, &D3DViewport);

        // 렌더 타겟과 깊이 버퍼 설정
        ID3D11RenderTargetView* RenderTargetView = Viewport->GetRenderTargetView();
        ID3D11DepthStencilView* DepthStencilView = Viewport->GetDepthStencilView();

        if (RenderTargetView && DepthStencilView)
        {
            RHI->GetDeviceContext()->OMSetRenderTargets(1, &RenderTargetView, DepthStencilView);
        }
    }
}
