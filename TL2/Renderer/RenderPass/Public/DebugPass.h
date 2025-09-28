#pragma once
#include "RenderPass.h"

/**
 * @brief 디버그 렌더링 패스
 * 기즈모, 그리드, 선택 하이라이트 등을 해당 Pass에서 처리해야 함
 */
class FDebugPass :
    public IRenderPass
{
public:
    FDebugPass() : IRenderPass(ERenderPassType::DebugPass)
    {
    }

    ~FDebugPass() override;
    
    void Execute(const FSceneView* View, FSceneRenderer* SceneRenderer) override;
    void Initialize() override;
    void Cleanup() override;

private:
    void BeginLineBatch();
    void EndLineBatch(const FMatrix& ModelMatrix, const FMatrix& ViewMatrix, const FMatrix& ProjectionMatrix);
    void RenderGrid(const FSceneView* View, FSceneRenderer* SceneRenderer);
    void RenderGizmos(const FSceneView* View, FSceneRenderer* SceneRenderer);
    void RenderActorLines(AActor* Actor, const FSceneView* View, FSceneRenderer* SceneRenderer);
};
