#pragma once
#include "RenderCommand.h"

/**
* @brief HighLight Buffer 업데이트 명령
 */
class FRHIUpdateHighlightBufferCommand : public IRHICommand
{
public:
    FRHIUpdateHighlightBufferCommand(URenderer* InRenderer, bool bInIsSelected,
                                     const FVector& InColor, uint32 InGizmo = 0)
        : Renderer(InRenderer), bIsSelected(bInIsSelected), Color(InColor), Gizmo(InGizmo)
    {
    }

    void Execute() override;

    ERHICommandType GetCommandType() const override
    {
        return ERHICommandType::SetBoundShaderState;
    }

private:
    URenderer* Renderer;
    bool bIsSelected;
    FVector Color;
    uint32 Gizmo;
};
