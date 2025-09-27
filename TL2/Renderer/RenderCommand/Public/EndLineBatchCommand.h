#pragma once
#include "RenderCommand.h"

/**
 * @brief Line Batch End Command Class
 */
class FRHIEndLineBatchCommand :
    public IRHICommand
{
public:
    FRHIEndLineBatchCommand(URenderer* InRenderer, const FMatrix& InModelMatrix,
                            const FMatrix& InViewMatrix, const FMatrix& InProjMatrix)
        : Renderer(InRenderer), ModelMatrix(InModelMatrix), ViewMatrix(InViewMatrix),
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
    FMatrix ModelMatrix;
    FMatrix ViewMatrix;
    FMatrix ProjMatrix;
};
