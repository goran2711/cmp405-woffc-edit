cbuffer HighlightProperties : register(b0)
{
    float4 color;
};

float4 main(float4 position : SV_Position) : SV_Target
{
    return float4(1.f, 0.f, 0.f, 1.f);
}