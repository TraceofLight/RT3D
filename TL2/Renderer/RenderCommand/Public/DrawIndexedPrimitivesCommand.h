#pragma once
#include "RenderCommand.h"

/**
 * @brief 메시 렌더링 Command를 정의한 함수
 */
class FRHIDrawIndexedPrimitivesCommand :
    public IRHICommand
{
public:
    FRHIDrawIndexedPrimitivesCommand(URenderer* InRenderer, UPrimitiveComponent* InComponent,
                                     const FMatrix& InViewMatrix, const FMatrix& InProjMatrix)
        : Renderer(InRenderer), Component(InComponent), ViewMatrix(InViewMatrix),
          ProjMatrix(InProjMatrix)
    {
    }

    void Execute() override;

    ERHICommandType GetCommandType() const override
    {
        return ERHICommandType::DrawIndexedPrimitives;
    }

private:
    URenderer* Renderer;
    UPrimitiveComponent* Component;
    FMatrix ViewMatrix;
    FMatrix ProjMatrix;
};
