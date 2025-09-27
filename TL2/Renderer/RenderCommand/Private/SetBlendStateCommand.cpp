#include "pch.h"
#include "Renderer/RenderCommand/Public/SetBlendStateCommand.h"

/**
 * @brief Blend State 설정 실행 함수
 */
void FRHISetBlendStateCommand::Execute()
{
    if (!Renderer)
    {
        return;
    }

    Renderer->OMSetBlendState(bEnableBlend);
}
