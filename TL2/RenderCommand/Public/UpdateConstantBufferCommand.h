#pragma once
#include "RenderCommand.h"

/**
 * @brief ConstantBuffer 업데이트 Command 클래스
 */
class FRHIUpdateConstantBufferCommand :
    public IRHICommand
{
public:
    FRHIUpdateConstantBufferCommand(URenderer* InRenderer, const FMatrix& InModelMatrix,
                                    const FMatrix& InViewMatrix, const FMatrix& InProjMatrix)
        : Renderer(InRenderer), ModelMatrix(InModelMatrix), ViewMatrix(InViewMatrix),
          ProjMatrix(InProjMatrix)
    {
    }

    void Execute() override;

    ERHICommandType GetCommandType() const override
    {
        return ERHICommandType::SetBoundShaderState;
    }

private:
    URenderer* Renderer;
    FMatrix ModelMatrix;
    FMatrix ViewMatrix;
    FMatrix ProjMatrix;
};
