cbuffer Matrices : register(b0)
{
	matrix world;
	matrix view;
	matrix projection;
};

struct VSOutput
{
	float4 Diffuse    : COLOR;
	float4 PositionPS : SV_Position;
};

struct GSOutput
{
	float4 colour : COLOR;
	float4 pos : SV_POSITION;
};

[maxvertexcount(6)]
void main(
	point VSOutput input[1], 
	inout LineStream< GSOutput > output
)
{
	for (int i = 0; i < 6; i += 2)
	{
		GSOutput pointA;
		pointA.pos = input[0].PositionPS;
		pointA.colour = float4(1.f, 0.f, 0.f, 1.f);

		GSOutput pointB;
		pointB.pos = pointA.pos + float4(0.f, 1.f, 0.f, 0.f);
		pointB.colour = float4(0.f, 1.f, 0.f, 1.f);

		output.Append(pointA);
		output.Append(pointB);
	}
}