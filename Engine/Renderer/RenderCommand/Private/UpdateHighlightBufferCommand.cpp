#include "pch.h"
#include "Renderer/RenderCommand/Public/UpdateHighlightBufferCommand.h"

/**
 * @brief Highlight Buffer Command를 실행하는 함수
 */
void FRHIUpdateHighlightBufferCommand::Execute()
{
    if (!RHIDevice)
    {
        return;
    }
    
    RHIDevice->UpdateHighLightConstantBuffers(bIsSelected ? 1 : 0, Color, 0, 0, 0, Gizmo);
}
