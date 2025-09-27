#pragma once
#include "RenderCommand.h"

/**
 * @brief View Mode Set Command Class
 */
class FRHISetViewModeCommand :
    public IRHICommand
{
public:
    FRHISetViewModeCommand(URenderer* InRenderer, EViewModeIndex InViewMode)
        : Renderer(InRenderer), ViewMode(InViewMode)
    {
    }

    void Execute() override;

    ERHICommandType GetCommandType() const override
    {
        return ERHICommandType::SetBoundShaderState;
    }

private:
    URenderer* Renderer;
    EViewModeIndex ViewMode;
};
