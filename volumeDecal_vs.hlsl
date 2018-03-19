cbuffer Matrices : register(b0)
{
    row_major matrix world;
    row_major matrix view;
    row_major matrix projection;

    row_major matrix screenToLocal;
};

// DirectX::GeometricPrimitive::VertexType (aka. DirectX::VertexPositionNormalTexture)
struct VSInput
{
    float3 position : SV_Position;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
};

struct VSOutput
{
    float4 position : SV_Position;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    output.position = mul(float4(input.position, 1.f), world);
    output.position = mul(output.position, view);
    output.position = mul(output.position, projection);

    return output;
}