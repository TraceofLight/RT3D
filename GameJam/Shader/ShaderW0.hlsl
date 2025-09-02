cbuffer constants : register(b0)
{
    float3 Offset;
    float ScaleX;
    float ScaleY;
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
    
    // 비균등 스케일 적용
    float3 scaledPos = float3(input.position.x * ScaleX,
                              input.position.y * ScaleY,
                              input.position.z);
    output.position = float4(scaledPos + Offset, 1.0f);
    
    // 색상은 그대로 전달합니다.
    output.color = input.color;
    
    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    // Output the color directly
    return input.color;
}
