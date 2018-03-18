cbuffer HighlightProperties : register(b0)
{
    float4 color;
};

struct VSOutput
{
    float4 position : SV_POSITION;
};

float4 main(VSOutput input) : SV_Target
{
    return float4(1.f, 1.f, 1.f, 1.f);
}