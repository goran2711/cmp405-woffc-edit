cbuffer Matrices : register(b0)
{
    // row_major?
    matrix world;
    matrix view;
    matrix projection;
    matrix worldView;   // Improperly configured (DO NOT USE!)
};

struct VSInput
{
    float3 position : SV_Position;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;   // Ignored
    float4 color : COLOR;
    float2 texCoord : TEXCOORD; // Ignored
};

struct VSOutput
{
    float4 position : SV_POSITION;
};

float4 main(VSInput input) : SV_Position
{
    static const float DIST = 2.f;

    float4 newPosition = float4(input.position + (input.normal * DIST), 1.f);

    newPosition = mul(newPosition, world);
    newPosition = mul(newPosition, view);
    newPosition = mul(newPosition, projection);

    return newPosition;
}