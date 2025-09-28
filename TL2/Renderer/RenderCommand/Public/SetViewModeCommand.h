#pragma once
#include "RenderCommand.h"

/**
 * @brief View Mode Set Command Class
 */
class FRHISetViewModeCommand :
    public IRHICommand
{
public:
    FRHISetViewModeCommand(URHIDevice* InRHIDevice, EViewModeIndex InViewMode)
        : RHIDevice(InRHIDevice), ViewMode(InViewMode)
    {
    }

    void Execute() override;

    ERHICommandType GetCommandType() const override
    {
        return ERHICommandType::SetBoundShaderState;
    }

private:
    URHIDevice* RHIDevice;
    EViewModeIndex ViewMode;
};
