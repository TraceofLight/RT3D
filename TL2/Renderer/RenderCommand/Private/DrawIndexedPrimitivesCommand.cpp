#include "pch.h"
#include "Renderer/RenderCommand/Public/DrawIndexedPrimitivesCommand.h"
#include "LineComponent.h"
#include "ResourceManager.h"
#include "Shader.h"

/**
 * @brief Primitive Render 명령을 실행 
 */
void FRHIDrawIndexedPrimitivesCommand::Execute()
{
    if (!RHIDevice || !Component)
    {
        return;
    }

    SetupShaderForComponent(Component);
    
    // 컬포넌트 렌더링 실행
    Component->Render(RHIDevice, ViewMatrix, ProjMatrix);
}

void FRHIDrawIndexedPrimitivesCommand::SetupShaderForComponent(UPrimitiveComponent* InComponent)
{
    if (!InComponent || !RHIDevice) return;
    
    UShader* ComponentShader;
    
    // 언리얼 스타일: 컴포넌트 타입에 따른 셰이더 선택
    if (ULineComponent* LineComp = Cast<ULineComponent>(InComponent))
    {
        // 라인 렌더링용 셰이더 로드
        ComponentShader = UResourceManager::GetInstance().Load<UShader>("ShaderLine.hlsl", EVertexLayoutType::PositionColor);
    }
    else
    {
        // 기본 셰이더 로드
        ComponentShader = UResourceManager::GetInstance().Load<UShader>("StaticMeshShader.hlsl", EVertexLayoutType::PositionColorTexturNormal);
    }
    
    // 셰이더 바인딩
    if (ComponentShader)
    {
        ID3D11DeviceContext* DeviceContext = RHIDevice->GetDeviceContext();
        DeviceContext->VSSetShader(ComponentShader->GetVertexShader(), nullptr, 0);
        DeviceContext->PSSetShader(ComponentShader->GetPixelShader(), nullptr, 0);
        DeviceContext->IASetInputLayout(ComponentShader->GetInputLayout());
    }
}
