#include "pch.h"
#include "RenderCommand/Public/UpdateHighlightBufferCommand.h"

/**
 * @brief Highlight Buffer Command를 실행하는 함수
 */
void FRHIUpdateHighlightBufferCommand::Execute()
{
    if (!Renderer)
    {
        return;
    }
    
    Renderer->
        UpdateHighLightConstantBuffer(bIsSelected ? 1 : 0, Color, 0, 0, 0, Gizmo);
}
