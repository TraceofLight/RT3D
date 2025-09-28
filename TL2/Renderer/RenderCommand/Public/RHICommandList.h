#pragma once

#include "RenderCommand.h"
#include <queue>

class URHIDevice;

/**
 * @brief RenderCommand들을 수집하고 일괄 실행하는 클래스 (언리얼 엔진 스타일)
 * 여러 패스에서 생성된 RenderCommand들을 모아서 효율적으로 GPU에 제출
 */
class FRHICommandList
{
public:
    FRHICommandList(URHIDevice* InRHIDevice);
    ~FRHICommandList();

    // RenderCommand 추가
    template<typename TCommand, typename... TArgs>
    void EnqueueCommand(TArgs&&... Args)
    {
        TCommand* Command = new TCommand(RHIDevice, std::forward<TArgs>(Args)...);
        PendingCommands.push(Command);
    }

    // 모든 Command 실행
    void Execute();
    
    // Command 큐 비우기
    void Clear();
    
    // 통계
    int32 GetCommandCount() const { return static_cast<int32>(PendingCommands.size()); }
    bool IsEmpty() const { return PendingCommands.empty(); }

    // RHI Device 접근자
    URHIDevice* GetRHIDevice() const { return RHIDevice; }

    // 자주 사용되는 Command 래퍼들
    void SetViewport(float X, float Y, float Width, float Height, float MinDepth = 0.0f, float MaxDepth = 1.0f);
    void SetRenderTarget(const class FSceneView* View);
    void ClearRenderTarget(float R = 0.0f, float G = 0.0f, float B = 0.0f, float A = 1.0f);
    void SetDepthStencilState(EComparisonFunc Func);
    void SetBlendState(bool bEnableBlending);
    void UpdateConstantBuffers(const FMatrix& Model, const FMatrix& View, const FMatrix& Projection);
    void DrawIndexedPrimitive(UPrimitiveComponent* Component, const FMatrix& ViewMatrix, const FMatrix& ProjMatrix);

private:
    URHIDevice* RHIDevice;
    TQueue<IRHICommand*> PendingCommands;
    
    // 통계 추적
    int32 ExecutedCommandCount = 0;
    int32 TotalDrawCalls = 0;
    
    void InternalExecuteCommand(IRHICommand* Command);
};