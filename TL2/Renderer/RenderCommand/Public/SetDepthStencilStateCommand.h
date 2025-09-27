#pragma once
#include "RenderCommand.h"

/**
 * @brief Depth Stencil State 설정 Command Class
 */
class FRHISetDepthStencilStateCommand :
    public IRHICommand
{
public:
    FRHISetDepthStencilStateCommand(URenderer* InRenderer, EComparisonFunc InCompareFunc)
        : Renderer(InRenderer), CompareFunction(InCompareFunc)
    {
    }

    void Execute() override;

    ERHICommandType GetCommandType() const override
    {
        return ERHICommandType::SetBoundShaderState;
    }

private:
    URenderer* Renderer;
    EComparisonFunc CompareFunction;
};
