#include "pch.h"
#include "Renderer/RenderCommand/Public/RHICommandList.h"

#include "Renderer/RenderCommand/Public/DrawIndexedPrimitivesCommand.h"
#include "Renderer/RenderCommand/Public/SetBlendStateCommand.h"
#include "Renderer/RenderCommand/Public/SetDepthStencilStateCommand.h"
#include "Renderer/RenderCommand/Public/UpdateConstantBufferCommand.h"


class FRHISetRenderTargetCommand;

FRHICommandList::FRHICommandList(URHIDevice* InRHIDevice)
    : RHIDevice(InRHIDevice)
    , ExecutedCommandCount(0)
    , TotalDrawCalls(0)
{
}

FRHICommandList::~FRHICommandList()
{
    Clear();
}

void FRHICommandList::Execute()
{
    if (!RHIDevice)
    {
        return;
    }

    ExecutedCommandCount = 0;
    TotalDrawCalls = 0;
    
    while (!PendingCommands.empty())
    {
        IRHICommand* Command = PendingCommands.front();
        PendingCommands.pop();

        if (Command)
        {
            InternalExecuteCommand(Command);
            delete Command;
        }
    }
}

void FRHICommandList::Clear()
{
    while (!PendingCommands.empty())
    {
        IRHICommand* Command = PendingCommands.front();
        PendingCommands.pop();
        delete Command;
    }
    
    ExecutedCommandCount = 0;
    TotalDrawCalls = 0;
}

void FRHICommandList::InternalExecuteCommand(IRHICommand* Command)
{
    if (!Command)
        return;

    Command->Execute();
    ExecutedCommandCount++;

    // Draw Call 통계 업데이트
    ERHICommandType CommandType = Command->GetCommandType();
    if (CommandType == ERHICommandType::DrawIndexedPrimitives)
    {
        TotalDrawCalls++;
    }
}

// 래퍼 메서드들
void FRHICommandList::SetRenderTarget(const FSceneView* View)
{
    EnqueueCommand<FRHISetRenderTargetCommand>(View);
}

void FRHICommandList::SetDepthStencilState(EComparisonFunc Func)
{
    EnqueueCommand<FRHISetDepthStencilStateCommand>(Func);
}

void FRHICommandList::SetBlendState(bool bEnableBlending)
{
    EnqueueCommand<FRHISetBlendStateCommand>(bEnableBlending);
}

void FRHICommandList::UpdateConstantBuffers(const FMatrix& Model, const FMatrix& View, const FMatrix& Projection)
{
    EnqueueCommand<FRHIUpdateConstantBufferCommand>(Model, View, Projection);
}

void FRHICommandList::DrawIndexedPrimitive(UPrimitiveComponent* Component, const FMatrix& ViewMatrix, const FMatrix& ProjMatrix)
{
    EnqueueCommand<FRHIDrawIndexedPrimitivesCommand>(Component, ViewMatrix, ProjMatrix);
}

void FRHICommandList::SetViewport(float X, float Y, float Width, float Height, float MinDepth, float MaxDepth)
{
    // 직접 RHI 호출 (단순한 작업이므로 Command로 감쌀 필요 없음)
    if (RHIDevice)
    {
        D3D11_VIEWPORT Viewport = {};
        Viewport.TopLeftX = X;
        Viewport.TopLeftY = Y;
        Viewport.Width = Width;
        Viewport.Height = Height;
        Viewport.MinDepth = MinDepth;
        Viewport.MaxDepth = MaxDepth;
        
        RHIDevice->GetDeviceContext()->RSSetViewports(1, &Viewport);
    }
}

void FRHICommandList::ClearRenderTarget(float R, float G, float B, float A)
{
    // 직접 RHI 호출
    if (RHIDevice)
    {
        // TODO: 현재 렌더 타겟을 클리어하는 로직 구현
        // RHIDevice에 ClearRenderTarget 메소드가 있다면 사용
    }
}