#include "pch.h"
#include "RenderCommand/Public/SetViewModeCommand.h"

/**
 * @brief ViewMode 변경 처리를 실행하는 함수
 */
void FRHISetViewModeCommand::Execute()
{
    if (!Renderer)
    {
        return;
    }

    Renderer->SetViewModeType(ViewMode);
}
