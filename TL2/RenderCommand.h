#pragma once

enum class ERHICommandType : uint8
{
    AddPrimitive, // Scene에 Primitive 추가
    RemovePrimitive, // Scene에서 Primitive 정보 제거

    CreateTexture, // GPU에서 사용할 새로운 Texture Resource 생성 명령
    ReleaseResource, // 더 이상 사용하지 않는 GPU Resource 해제

    SetRenderTarget, // 렌더링 결과를 출력할 대상 설정
    SetBoundShaderState, // 사용할 Shade, Vertex Declaration 등, 렌더링 파이프라인 상태 설정

    DrawIndexedPrimitives, // Primitive를 그리는 Draw Call (점, 선, 삼각형)
    DispatchComputeShader, // Compute Shader를 실행하여 병렬 계산 작업을 시작하는 명령


    SubmitCommandList, // GPU 처리 시작 명령
    WaitOnFence, // Batch 처리 등을 위한 Blocking
    
    Num
};

/**
 * @brief Renderer한테 보내야 하는 정보를 담는 클래스
 * 해당 정보를 통해 가지고 있는 Rendering 정보를 
 */
class IRHICommand
{
public:
    IRHICommand() = default;
    virtual ~IRHICommand() = default;

    virtual void Execute() = 0;

    // XXX(KHJ): FName_None 처리하고 싶은데 없고 귀찮으니 어차피 직접 호출할 일 없어야 하니까 적당한 값으로 처리
    virtual const FName& GetName() const { return FName{""}; }
};
