struct GSOutput
{
	float4 pos : SV_POSITION;
	float4 colour : COLOR;
};

float4 main(GSOutput input) : SV_TARGET
{
	return input.colour;
}