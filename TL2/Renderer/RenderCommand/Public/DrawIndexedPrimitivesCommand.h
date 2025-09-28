#pragma once
#include "RenderCommand.h"

#include "SetRenderTargetCommand.h"

/**
 * @brief 메시 렌더링 Command를 정의한 함수
 */
class FRHIDrawIndexedPrimitivesCommand :
    public IRHICommand
{
public:
    FRHIDrawIndexedPrimitivesCommand(URHIDevice* InRHIDevice, UPrimitiveComponent* InComponent,
                                     const FMatrix& InViewMatrix, const FMatrix& InProjMatrix)
        : RHIDevice(InRHIDevice), Component(InComponent), ViewMatrix(InViewMatrix),
          ProjMatrix(InProjMatrix)
    {
    }

    void Execute() override;

    void SetupShaderForComponent(UPrimitiveComponent* InComponent);

    ERHICommandType GetCommandType() const override
    {
        return ERHICommandType::DrawIndexedPrimitives;
    }

private:
    URHIDevice* RHIDevice;
    UPrimitiveComponent* Component;
    FMatrix ViewMatrix;
    FMatrix ProjMatrix;
};
