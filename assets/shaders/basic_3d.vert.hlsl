// Basic 3D Vertex Shader - SDL_GPU için HLSL
// Uniform Block: set=1, binding=0 (SDL_GPU kuralı)

// Vertex Input
struct VertexInput
{
    float3 position : TEXCOORD0;  // location 0
    float3 color    : TEXCOORD1;  // location 1
    float2 uv       : TEXCOORD2;  // location 2
};

// Vertex Output -> Fragment Shader Input
struct VertexOutput
{
    float4 position : SV_Position;
    float3 color    : TEXCOORD0;
    float2 uv       : TEXCOORD1;
};

// Uniform Buffer - MVP Matrisleri
// SDL_GPU: set=1, binding=0 (Vertex Uniform Slot 0)
cbuffer UniformBlock : register(b0, space1)
{
    float4x4 model      : packoffset(c0);
    float4x4 view       : packoffset(c4);
    float4x4 projection : packoffset(c8);
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    
    // MVP transformation
    float4 worldPos = mul(model, float4(input.position, 1.0));
    float4 viewPos = mul(view, worldPos);
    output.position = mul(projection, viewPos);
    
    // Pass-through
    output.color = input.color;
    output.uv = input.uv;
    
    return output;
}
