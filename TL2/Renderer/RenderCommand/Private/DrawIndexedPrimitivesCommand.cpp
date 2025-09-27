#include "pch.h"
#include "Renderer/RenderCommand/Public/DrawIndexedPrimitivesCommand.h"

/**
 * @brief Primitive Render 명령을 실행 
 */
void FRHIDrawIndexedPrimitivesCommand::Execute()
{
    if (!Renderer || !Component)
    {
        return;
    }

    // 컴포넌트 렌더링 실행
    Component->Render(Renderer, ViewMatrix, ProjMatrix);
}
