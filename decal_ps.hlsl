cbuffer MatrixBuffer : register(b0)
{
    row_major matrix invViewProjection;
    row_major matrix worldToProjectorClip;
};

Texture2D projectorDepthTex : register(t0);
Texture2D cameraDepthTex : register(t1);

Texture2D decalTexture : register(t2);

SamplerState texSampler : register(s0);


float4 main(float4 colour : COLOR0, float2 texCoord : TEXCOORD0) : SV_Target
{
    // Depth of this pixel from the camera's pov
    float sceneDepth = cameraDepthTex.Sample(texSampler, texCoord).r;

    // UV to NDC
    float2 xyNDC = (texCoord * float2(2.f, -2.f)) + float2(-1.f, 1.f);

    // NDC to world
    float4 ndc = float4(xyNDC, sceneDepth, 1.f);
    float4 ws = mul(ndc, invViewProjection);
    ws /= ws.w;

    // World to projector clip
    float4 projectorHMC = mul(ws, worldToProjectorClip);

    // Discard fragments that are not visible to the projector (outside of clip space)
    clip( abs(projectorHMC.w) - abs(projectorHMC.xy) );

    // Projector clip to NDC
    float3 projectorNDC = projectorHMC.xyz / projectorHMC.w;

    // Depth of this pixel from the projector's pov
    float sceneProjectorDepth = projectorNDC.z;

    // NDC to UV
    float2 projectorUV = (projectorNDC.xy * float2(0.5f, -0.5f)) + 0.5f;

    //return float4(projectorUV, 0.f, 1.f);
    // The depth at this fragment's position from the projector's pov (may or may not be the same, depending on if something is occluding it)
    float projectorDepth = projectorDepthTex.Sample(texSampler, projectorUV).r;

    // Only sample the decal if the fragment is visible to the projector
    clip( sceneProjectorDepth - projectorDepth );

    return decalTexture.Sample(texSampler, projectorUV);
}