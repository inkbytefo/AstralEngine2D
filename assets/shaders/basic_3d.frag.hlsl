// Basic 3D Fragment Shader - SDL_GPU için HLSL

// Fragment Input (Vertex Shader'dan gelen)
struct FragmentInput
{
    float4 position : SV_Position;
    float3 color    : TEXCOORD0;
    float2 uv       : TEXCOORD1;
};

// Fragment Output
struct FragmentOutput
{
    float4 color : SV_Target0;
};

// Texture ve Sampler (opsiyonel)
// SDL_GPU: set=2, binding=0 (Fragment Sampler Slot 0)
// Texture2D<float4> mainTexture : register(t0, space2);
// SamplerState mainSampler : register(s0, space2);

FragmentOutput main(FragmentInput input)
{
    FragmentOutput output;
    
    // Basit vertex color rendering
    output.color = float4(input.color, 1.0);
    
    // Texture kullanmak isterseniz:
    // float4 texColor = mainTexture.Sample(mainSampler, input.uv);
    // output.color = float4(input.color, 1.0) * texColor;
    
    return output;
}
