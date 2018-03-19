cbuffer MatrixBuffer : register(b0)
{
    row_major matrix world;
    row_major matrix view;
    row_major matrix projection;

    row_major matrix projectorViewProjection;
}

// VertexPositionNormalTexture
struct VSIn
{
    float3 position : SV_Position;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
};

struct VSOut
{
    float4 viewPosition : PROJECTOR_POS;
    float2 tc : TEXCOORD;
    float4 position : SV_Position;
};

VSOut main(VSIn input)
{
    VSOut output;

    output.position = mul(float4(input.position, 1.f), world);

    output.viewPosition = mul(output.position, projectorViewProjection);

    output.position = mul(output.position, view);
    output.position = mul(output.position, projection);

    output.tc = input.texCoord;

    return output;
}