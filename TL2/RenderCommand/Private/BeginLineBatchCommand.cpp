#include "pch.h"
#include "RenderCommand/Public/BeginLineBatchCommand.h"

/**
 * @brief Line Batch 시작 Command 실행 함수
 */
void FRHIBeginLineBatchCommand::Execute()
{
    if (!Renderer)
    {
        return;
    }

    Renderer->BeginLineBatch();
}
