#pragma once
#include "RenderCommand.h"

/**
* @brief Line Batch 처리 Command Class
 */
class FRHIBeginLineBatchCommand :
    public IRHICommand
{
public:
    FRHIBeginLineBatchCommand(URenderer* InRenderer) : Renderer(InRenderer)
    {
    }

    void Execute() override;

    ERHICommandType GetCommandType() const override
    {
        return ERHICommandType::SetBoundShaderState;
    }

private:
    URenderer* Renderer;
};
