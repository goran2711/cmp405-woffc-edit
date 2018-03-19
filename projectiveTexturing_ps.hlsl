Texture2D decalTexture : register(t0);
SamplerState decalSampler : register(s0);

struct VSOut
{
    float4 texCoord : PROJECTOR_POS;
    float4 position : SV_Position;
};

float4 main(VSOut input) : SV_Target
{
    float2 texCoord = input.texCoord.xy / input.texCoord.w;

    return decalTexture.Sample(decalSampler, texCoord);
}