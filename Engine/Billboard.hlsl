// C++에서 상수 버퍼를 통해 전달될 데이터
cbuffer CameraInfo : register(b0)
{
    float3 textWorldPos;
    row_major matrix viewMatrix;
    row_major matrix projectionMatrix;
    row_major matrix viewInverse;
}

// C++의 BillboardCharInfo와 레이아웃이 동일해야 하는 입력 구조체
struct VS_INPUT
{
    float3 centerPos : WORLDPOSITION;
    float2 size : SIZE;
    float4 uvRect : UVRECT;
    uint vertexId : SV_VertexID; // GPU가 자동으로 부여하는 고유 정점 ID
};

struct PS_INPUT
{
    float4 pos_screenspace : SV_POSITION;
    float2 tex : TEXCOORD0;
};

Texture2D fontAtlas : register(t0);
SamplerState linearSampler : register(s0);


PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    // 카메라 회전 무시 (뷰 역행렬로 변환)
    float3 pos_aligned = mul(float4(input.centerPos, 0.0f), viewInverse).xyz;

    // 월드 좌표계에서 최종 위치 계산
    float3 finalPos_worldspace = textWorldPos + pos_aligned;

    // 뷰-프로젝션 변환
    output.pos_screenspace = mul(float4(finalPos_worldspace, 1.0f), mul(viewMatrix, projectionMatrix)) * 0.1;

    // UV 계산: vertexId % 4로 코너 결정
    // 0=좌상단(0,0), 1=우상단(1,0), 2=좌하단(0,1), 3=우하단(1,1)
    uint corner = input.vertexId % 4;
    float u = input.uvRect.x + ((corner == 1 || corner == 3) ? input.uvRect.z : 0.0f);
    float v = input.uvRect.y + ((corner == 2 || corner == 3) ? input.uvRect.w : 0.0f);
    output.tex = float2(u, v);

    return output;
}

float4 mainPS(PS_INPUT input) : SV_Target
{
    float4 color = fontAtlas.Sample(linearSampler, input.tex);

    clip(color.a - 0.5f); // alpha - 0.5f < 0 이면 해당픽셀 렌더링 중단

    return color;
}