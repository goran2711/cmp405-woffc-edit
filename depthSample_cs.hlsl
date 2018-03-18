cbuffer Settings : register(b0)
{
    float2 samplePoint;
};

Texture2D gDepthTexture : register(t0);
RWTexture2D<float4> gDepthOut : register(u0);

[numthreads(1, 1, 1)]
void main(int3 groupThreadID : SV_GroupThreadID, int3 dispatchID : SV_DispatchThreadID)
{
    // Sample without any filtering
    float depth = gDepthTexture[int2(samplePoint.x, samplePoint.y)].r;

    gDepthOut[int2(0, 0)] = float4(depth, depth, depth, depth);
}