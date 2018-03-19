cbuffer Matrices : register(b0)
{
    row_major matrix world;
    row_major matrix view;
    row_major matrix projection;

    row_major matrix screenToLocal;
};

cbuffer FixedBuffer : register(b1)
{
    float2 pixelSize;
};

struct VSOutput
{
    float4 position : SV_Position;
};

Texture2D depthTexture : register(t0);
Texture2D decalTexture : register(t1);

SamplerState depthSampler : register(s0);
SamplerState decalSampler : register(s1);

float4 main(VSOutput input) : SV_Target
{
    // Compute normalized screen position (SV_POSITION is in screen coordinates, pixelSize is (1 / screenDim))
    float2 texCoord = input.position.xy * pixelSize;

    // Sample the depth buffer at this point
    float depth = depthTexture.Sample(depthSampler, texCoord).r;

    // Unproject this point
    float4 ssCoord = float4(texCoord, depth, 1.f);
    float4 relPos = mul(ssCoord, screenToLocal);
    relPos /= relPos.w;

    // Sample the decal
    return decalTexture.Sample(decalSampler, relPos.xy);
}