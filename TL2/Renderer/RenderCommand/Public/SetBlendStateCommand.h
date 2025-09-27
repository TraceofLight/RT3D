#pragma once
#include "RenderCommand.h"

/**
* @brief Blend State 설정 Command 클래스
 */
class FRHISetBlendStateCommand :
    public IRHICommand
{
public:
    FRHISetBlendStateCommand(URenderer* InRenderer, bool bInEnableBlend)
        : Renderer(InRenderer), bEnableBlend(bInEnableBlend)
    {
    }

    void Execute() override;

    ERHICommandType GetCommandType() const override
    {
        return ERHICommandType::SetBoundShaderState;
    }

private:
    URenderer* Renderer;
    bool bEnableBlend;
};
