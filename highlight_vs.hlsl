cbuffer Matrices : register(b0)
{
    row_major matrix world;
    row_major matrix view;
    row_major matrix projection;
    row_major matrix worldView;   // Improperly configured (DO NOT USE!)
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

VSOutput main(VSInput input)
{
    static const float DIST = 0.02f;

    float4 newPosition = float4(input.position, 1.f);
    //float4 newPosition = float4(input.position + (input.normal * DIST), 1.f);

    newPosition = mul(newPosition, world);
    newPosition = mul(newPosition, view);
    newPosition = mul(newPosition, projection);

    VSOutput output;
    output.position = newPosition;

    return output;
}