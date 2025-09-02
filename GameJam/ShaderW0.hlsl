cbuffer constants : register(b0)
{
    float3 Offset;
    float Scale;
}

// ShaderW0.hlsl
struct VS_INPUT
{
    float4 position : POSITION; // Input position from vertex buffer
    float4 color : COLOR; // Input color from vertex buffer
};

struct PS_INPUT
{
    float4 position : SV_POSITION; // Transformed position to pass to the pixel shader
    float4 color : COLOR; // Color to pass to the pixel shader
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    
    // 1. 입력된 정점 위치에 Scale(반지름)을 곱하여 크기를 조절합니다.
    // 2. 그 결과에 Offset(위치)을 더하여 공을 최종 위치로 이동시킵니다.
    output.position = float4(input.position.xyz * Scale, 1.0f);
    output.position.xyz += Offset;
    
    // 색상은 그대로 전달합니다.
    output.color = input.color;
    
    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    // Output the color directly
    return input.color;
}
