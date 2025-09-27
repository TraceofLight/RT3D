#include "pch.h"
#include "RenderCommand/Public/EndLineBatchCommand.h"

/**
 * @brief Line Batch 종료 Command 실행 함수
 */
void FRHIEndLineBatchCommand::Execute()
{
    if (!Renderer)
    {
        return;
    }

    Renderer->EndLineBatch(ModelMatrix, ViewMatrix, ProjMatrix);
}
