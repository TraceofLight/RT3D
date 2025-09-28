#include "pch.h"
#include "../Public/DebugPass.h"

#include "CameraActor.h"
#include "FViewport.h"

// RenderCommand 헤더들
#include "SelectionManager.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/SceneView/Public/SceneView.h"
#include "Renderer\RenderCommand\Public\UpdateConstantBufferCommand.h"
#include "Renderer\RenderCommand\Public\UpdateHighlightBufferCommand.h"
#include "Renderer\RenderCommand\Public\SetViewModeCommand.h"
#include "Renderer\RenderCommand\Public\SetDepthStencilStateCommand.h"
#include "Renderer\RenderCommand\Public\SetBlendStateCommand.h"
#include "LineComponent.h"

FDebugPass::~FDebugPass()
{
    // Debug Pass 소멸자 - 필요한 경우 리소스 정리
}

void FDebugPass::Initialize()
{
    // DebugPass 초기화 작업
}

void FDebugPass::Cleanup()
{
    // DebugPass 정리 작업
}

void FDebugPass::Execute(const FSceneView* View, FSceneRenderer* SceneRenderer)
{
    if (!View || !SceneRenderer) return;

    UWorld* World = View->GetWorld();
    if (!World) return;

    URHIDevice* RHI = FSceneRenderer::GetGlobalRHI();
    if (!RHI) return;

    // 뷰 매트릭스 계산
    ACameraActor* Camera = View->GetCamera();
    FViewport* Viewport = View->GetViewport();

    if (!Camera || !Viewport) return;

    float ViewportAspectRatio = static_cast<float>(Viewport->GetSizeX()) / static_cast<float>(
        Viewport->GetSizeY());
    if (Viewport->GetSizeY() == 0) ViewportAspectRatio = 1.0f;

    FMatrix ViewMatrix = Camera->GetViewMatrix();
    FMatrix ProjectionMatrix = Camera->GetProjectionMatrix(ViewportAspectRatio, Viewport);

    // === 라인 배치 시작 ===
    BeginLineBatch();

    // === 그리드 렌더링 ===
    if (World->IsShowFlagEnabled(EEngineShowFlags::SF_Grid))
    {
        RenderGrid(View, SceneRenderer);
    }

    // === 기즈모 렌더링 ===
    // TODO: 기즈모는 별도 처리가 필요할 수 있음 (FViewportClient에서 별도 렌더링)
    RenderGizmos(View, SceneRenderer);

    // === 라인 배치 종료 ===
    EndLineBatch(FMatrix::Identity(), ViewMatrix, ProjectionMatrix);
}

void FDebugPass::BeginLineBatch()
{
    // TODO: RHI를 통한 라인 배치 시작
    // URenderer의 BeginLineBatch() 로직을 RHI로 이식
    URHIDevice* RHI = FSceneRenderer::GetGlobalRHI();
    if (!RHI) return;

    // 라인 배치 초기화 작업
}

void FDebugPass::EndLineBatch(const FMatrix& ModelMatrix, const FMatrix& ViewMatrix,
                              const FMatrix& ProjectionMatrix)
{
    // TODO: RHI를 통한 라인 배치 종료 및 렌더링
    // URenderer의 EndLineBatch() 로직을 RHI로 이식
    URHIDevice* RHI = FSceneRenderer::GetGlobalRHI();
    if (!RHI) return;

    // 모든 라인을 GPU로 전송하여 렌더링
}

void FDebugPass::RenderGrid(const FSceneView* View, FSceneRenderer* SceneRenderer)
{
    if (!View) return;

    UWorld* World = View->GetWorld();
    if (!World) return;

    URHIDevice* RHI = FSceneRenderer::GetGlobalRHI();
    if (!RHI) return;

    // 그리드 액터 찾기
    AGridActor* GridActor = World->GetGridActor();
    if (GridActor)
    {
        // GridActor의 LineComponent들을 처리
        RenderActorLines(GridActor, View, SceneRenderer);
    }

    // 모든 액터들의 LineComponent들을 처리
    const TArray<AActor*>& Actors = World->GetActors();
    for (AActor* Actor : Actors)
    {
        if (Actor && !Actor->GetActorHiddenInGame())
        {
            RenderActorLines(Actor, View, SceneRenderer);
        }
    }
}

void FDebugPass::RenderGizmos(const FSceneView* View, FSceneRenderer* SceneRenderer)
{
    if (!View) return;
    
    UWorld* World = View->GetWorld();
    if (!World) return;
    
    // 기즈모 액터 찾기
    AGizmoActor* GizmoActor = World->GetGizmoActor();
    if (!GizmoActor) return;
    
    // 선택된 액터가 있을 때만 기즈모 렌더링
    if (!USelectionManager::GetInstance().HasSelection()) return;
    
    URHIDevice* RHI = FSceneRenderer::GetGlobalRHI();
    if (!RHI) return;
    
    ACameraActor* Camera = View->GetCamera();
    FViewport* Viewport = View->GetViewport();
    if (!Camera || !Viewport) return;
    
    // 기즈모 컴포넌트들 가져오기
    TArray<USceneComponent*>* Components = GizmoActor->GetGizmoComponents();
    if (!Components) return;
    
    float ViewportAspectRatio = static_cast<float>(Viewport->GetSizeX()) / static_cast<float>(Viewport->GetSizeY());
    if (Viewport->GetSizeY() == 0) ViewportAspectRatio = 1.0f;
    
    FMatrix ViewMatrix = Camera->GetViewMatrix();
    FMatrix ProjectionMatrix = Camera->GetProjectionMatrix(ViewportAspectRatio, Viewport);
    EViewModeIndex ViewModeIndex = World->GetViewModeIndex();
    
    // 기즈모 컴포넌트들 렌더링
    for (int32 i = 0; i < Components->Num(); ++i)
    {
        USceneComponent* Component = (*Components)[i];
        if (!Component) continue;
        
        // 컴포넌트 활성 상태 확인
        if (UActorComponent* ActorComp = Cast<UActorComponent>(Component))
        {
            if (!ActorComp->IsActive()) continue;
        }
        
        FMatrix ModelMatrix = Component->GetWorldMatrix();
        FVector HighlightColor(1.0f, 1.0f, 1.0f);
        
        // RenderCommand를 통한 상수 버퍼 업데이트
        FRHIUpdateConstantBufferCommand UpdateConstantBufferCmd(RHI, ModelMatrix, ViewMatrix, ProjectionMatrix);
        UpdateConstantBufferCmd.Execute();
        
        // 기즈모 축 하이라이트 설정
        bool bIsHighlighted = (GizmoActor->GetGizmoAxis() == i + 1);
        FRHIUpdateHighlightBufferCommand UpdateHighlightCmd(RHI, bIsHighlighted, HighlightColor, i + 1);
        UpdateHighlightCmd.Execute();
        
        // 프리미티브 컴포넌트 렌더링
        if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
        {
            // 기즈모 렌더링을 위한 RHI 상태 설정 (RenderCommand 사용)
            FRHISetViewModeCommand SetViewModeCmd(RHI, EViewModeIndex::VMI_Unlit);
            SetViewModeCmd.Execute();
            
            FRHISetDepthStencilStateCommand SetDepthCmd(RHI, EComparisonFunc::Always);
            SetDepthCmd.Execute();
            
            FRHISetBlendStateCommand SetBlendCmd(RHI, true);
            SetBlendCmd.Execute();
            
            // TODO: Primitive 컴포넌트를 RHI로 렌더링
            // Primitive->RenderWithRHI(RHI, ViewMatrix, ProjectionMatrix);
            
            // 상태 복구 (RenderCommand 사용)
            FRHISetBlendStateCommand RestoreBlendCmd(RHI, false);
            RestoreBlendCmd.Execute();
            
            FRHISetDepthStencilStateCommand RestoreDepthCmd(RHI, EComparisonFunc::LessEqual);
            RestoreDepthCmd.Execute();
            
            FRHISetViewModeCommand RestoreViewModeCmd(RHI, ViewModeIndex);
            RestoreViewModeCmd.Execute();
        }
    }
    
    // 기즈모 렌더링 종료시 상태 설정 (RenderCommand 사용)
    FRHISetBlendStateCommand FinalBlendCmd(RHI, true); // 알파 블렌딩 유지
    FinalBlendCmd.Execute();
}

void FDebugPass::RenderActorLines(AActor* Actor, const FSceneView* View, FSceneRenderer* SceneRenderer)
{
    if (!Actor || !View || !SceneRenderer) return;
    
    URHIDevice* RHI = FSceneRenderer::GetGlobalRHI();
    if (!RHI) return;
    
    ACameraActor* Camera = View->GetCamera();
    FViewport* Viewport = View->GetViewport();
    if (!Camera || !Viewport) return;
    
    float ViewportAspectRatio = static_cast<float>(Viewport->GetSizeX()) / static_cast<float>(Viewport->GetSizeY());
    if (Viewport->GetSizeY() == 0) ViewportAspectRatio = 1.0f;
    
    FMatrix ViewMatrix = Camera->GetViewMatrix();
    FMatrix ProjectionMatrix = Camera->GetProjectionMatrix(ViewportAspectRatio, Viewport);
    
    // 액터의 LineComponent들을 찾아서 렌더링
    for (USceneComponent* Component : Actor->GetComponents())
    {
        if (!Component) continue;
        
        if (UActorComponent* ActorComp = Cast<UActorComponent>(Component))
        {
            if (!ActorComp->IsActive()) continue;
        }
        
        if (ULineComponent* LineComp = Cast<ULineComponent>(Component))
        {
            if (LineComp->HasVisibleLines())
            {
                // TODO: 라인 데이터를 가져와서 RHI로 렌더링
                // 현재는 GetWorldLineData()를 사용해서 라인 데이터 추출 가능
                TArray<FVector> StartPoints, EndPoints;
                TArray<FVector4> Colors;
                
                LineComp->GetWorldLineData(StartPoints, EndPoints, Colors);
                
                // RHI를 사용해서 라인들을 렌더링 (구 Renderer 방식)
                RenderLines(StartPoints, EndPoints, Colors, ViewMatrix, ProjectionMatrix);
            }
        }
    }
}

void FDebugPass::RenderLines(const TArray<FVector>& StartPoints, const TArray<FVector>& EndPoints, 
                             const TArray<FVector4>& Colors, const FMatrix& ViewMatrix, const FMatrix& ProjectionMatrix)
{
    if (StartPoints.empty() || EndPoints.empty() || Colors.empty()) return;
    if (StartPoints.size() != EndPoints.size() || StartPoints.size() != Colors.size()) return;
    
    URHIDevice* RHI = FSceneRenderer::GetGlobalRHI();
    if (!RHI) return;
    
    
    // 구 Renderer Line 배치 방식을 RHI로 직접 구현
    // 1. 라인 셸이더 로드 및 바인드
    UShader* LineShader = UResourceManager::GetInstance().Load<UShader>("ShaderLine.hlsl", EVertexLayoutType::PositionColor);
    if (!LineShader) 
    {
        return;
    }
    
    ID3D11DeviceContext* DeviceContext = RHI->GetDeviceContext();
    DeviceContext->VSSetShader(LineShader->GetVertexShader(), nullptr, 0);
    DeviceContext->PSSetShader(LineShader->GetPixelShader(), nullptr, 0);
    DeviceContext->IASetInputLayout(LineShader->GetInputLayout());
    
    // 2. 상수 버퍼 업데이트
    RHI->UpdateConstantBuffers(FMatrix::Identity(), ViewMatrix, ProjectionMatrix);
    
    // 3. 라인 데이터를 정점 버퍼로 생성
    std::vector<FVertexSimple> vertices;
    std::vector<uint32> indices;
    
    vertices.reserve(StartPoints.size() * 2);
    indices.reserve(StartPoints.size() * 2);
    
    for (size_t i = 0; i < StartPoints.size(); ++i)
    {
        uint32 startIndex = static_cast<uint32>(vertices.size());
        
        // 시작점과 끝점을 정점으로 추가
        FVertexSimple startVertex, endVertex;
        startVertex.Position = StartPoints[i];
        startVertex.Color = Colors[i];
        endVertex.Position = EndPoints[i];
        endVertex.Color = Colors[i];
        
        vertices.push_back(startVertex);
        vertices.push_back(endVertex);
        
        // 라인 인덱스 추가
        indices.push_back(startIndex);
        indices.push_back(startIndex + 1);
    }
    
    // 4. 동적 버퍼 생성 및 데이터 업로드
    ID3D11Buffer* vertexBuffer = nullptr;
    ID3D11Buffer* indexBuffer = nullptr;
    
    // 정점 버퍼 생성
    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_DYNAMIC;
    vbd.ByteWidth = static_cast<UINT>(sizeof(FVertexSimple) * vertices.size());
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    D3D11_SUBRESOURCE_DATA vinitData = {};
    vinitData.pSysMem = vertices.data();
    
    HRESULT hr = RHI->GetDevice()->CreateBuffer(&vbd, &vinitData, &vertexBuffer);
    if (FAILED(hr) || !vertexBuffer)
    {
        return;
    }
    
    // 인덱스 버퍼 생성
    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = static_cast<UINT>(sizeof(uint32) * indices.size());
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    
    D3D11_SUBRESOURCE_DATA iinitData = {};
    iinitData.pSysMem = indices.data();
    
    hr = RHI->GetDevice()->CreateBuffer(&ibd, &iinitData, &indexBuffer);
    if (FAILED(hr) || !indexBuffer)
    {
        if (vertexBuffer) vertexBuffer->Release();
        return;
    }
    
    // 5. 버퍼 바인드 및 렌더링
    UINT stride = sizeof(FVertexSimple);
    UINT offset = 0;
    
    DeviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    DeviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    
    // 6. DrawCall 실행!
    DeviceContext->DrawIndexed(static_cast<UINT>(indices.size()), 0, 0);
    
    // 7. 리소스 정리
    if (vertexBuffer)
    {
        vertexBuffer->Release();
    }
    if (indexBuffer)
    {
        indexBuffer->Release();
    }
}
