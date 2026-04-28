cbuffer ModelBuffer : register(b0)
{
    row_major float4x4 WorldMatrix;
}

cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
}

struct VS_INPUT
{
    float3 CenterPos : WORLDPOSITION;
    float2 Size : SIZE;
    float4 UVRect : UVRECT;
    uint VertexId : SV_VertexID; // GPU가 자동으로 부여하는 고유 정점 ID
};

struct PS_INPUT
{
    float4 PosScreenspace : SV_POSITION;
    float2 Tex : TEXCOORD0;
};

Texture2D FontAtlas : register(t0);
SamplerState LinearSampler : register(s0);


PS_INPUT mainVS(VS_INPUT Input)
{
    PS_INPUT Output;

    float4x4 MVP = mul(mul(WorldMatrix, ViewMatrix), ProjectionMatrix);
    
    Output.PosScreenspace = mul(float4(Input.CenterPos, 1.0f), MVP);
    
    Output.Tex = Input.UVRect.xy; // UV는 C++에서 계산했으므로 그대로 전달
        
    return Output;
}

float4 mainPS(PS_INPUT Input) : SV_Target
{
    float4 Color = FontAtlas.Sample(LinearSampler, Input.Tex);

    // alpha - 0.5f < 0 이면 해당픽셀 렌더링 중단
    if (Color.a - 0.5f < 0)
    {
        discard;
    }

    return Color;
}