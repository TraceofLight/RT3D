#pragma once
#include "RenderCommand.h"

class FSceneView;

/**
 * @brief RenderTarget 설정 Command를 정의한 클래스
 */
class FRHISetRenderTargetCommand :
    public IRHICommand
{
public:
    FRHISetRenderTargetCommand(URenderer* InRenderer, const FSceneView* InView)
        : Renderer(InRenderer), View(InView)
    {
    }

    void Execute() override;

    ERHICommandType GetCommandType() const override
    {
        return ERHICommandType::SetRenderTarget;
    }

private:
    URenderer* Renderer;
    const FSceneView* View;
};
