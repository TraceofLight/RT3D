#include "pch.h"
#include "Renderer/RenderCommand/Public/UpdateConstantBufferCommand.h"

/**
 * @brief Constant Buffer를 업데이트하는 Command 실행 함수
 */
void FRHIUpdateConstantBufferCommand::Execute()
{
    if (!Renderer)
    {
        return;
    }

    Renderer->UpdateConstantBuffer(ModelMatrix, ViewMatrix, ProjMatrix);
}

