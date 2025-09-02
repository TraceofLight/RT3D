cbuffer constants : register(b0)
{
    float3 Offset;
    float ScaleX;
    float ScaleY;
    float Rotation;
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
    
    // 비등방 스케일
    float2 scaled = float2(input.position.x * ScaleX, input.position.y * ScaleY);

    // 회전
    float c = cos(Rotation);
    float s = sin(Rotation);
    float2 rotated = float2(scaled.x * c - scaled.y * s,
                            scaled.x * s + scaled.y * c);

    // Offset (이미 NDC 공간을 직접 사용하므로 단순 더하기)
    float2 world = rotated + Offset.xy;
    
    output.position = float4(world.xy, input.position.z + Offset.z, 1.0f);
    
    // 색상은 그대로 전달합니다.
    output.color = input.color;
    
    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    // Output the color directly
    return input.color;
}
