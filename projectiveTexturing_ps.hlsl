Texture2D decalTexture : register(t0);
SamplerState decalSampler : register(s0);

struct VSOut
{
    float4 viewPosition : PROJECTOR_POS;
    float2 tc : TEXCOORD;
};

float4 main(VSOut input) : SV_Target
{
    // Calculate the projected texture coordinates
    //float2 projectTexCoord = ((input.viewPosition.xy / input.viewPosition.w) / float2(2.f, -2.f)) + 0.5f;
    float2 projectTexCoord = input.viewPosition.xy / input.viewPosition.w;

    return decalTexture.Sample(decalSampler, projectTexCoord);
}