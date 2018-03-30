cbuffer MatrixBuffer : register(b0)
{
    row_major matrix invViewProjection;
    row_major matrix worldToProjectorClip;
};

Texture2D cameraDepthTex : register(t0);

Texture2D decalTexture : register(t1);

SamplerState texSampler : register(s0);


float4 main(float4 colour : COLOR0, float2 texCoord : TEXCOORD0) : SV_Target
{
    static const float DEPTH_BIAS = 0.001f;

    //// Step 1: Convert screen space coordinate to world space coordinate with help from the scene depth buffer
    // Depth of this pixel from the camera's pov
    float sceneDepth = cameraDepthTex.Sample(texSampler, texCoord).r;

    // UV to NDC
    float2 xyNDC = (texCoord * float2(2.f, -2.f)) + float2(-1.f, 1.f);

    // NDC to world
    float4 ndc = float4(xyNDC, sceneDepth, 1.f);
    float4 ws = mul(ndc, invViewProjection);
    ws /= ws.w;

    //// Step 2: Convert world space coordinate to the projector's relative NDC so that the point's depth relative to the projector can be obtained
    // World to projector clip
    float4 projectorHMC = mul(ws, worldToProjectorClip);

    // Discard fragments that are not visible to the projector (outside of clip space)
    clip( abs(projectorHMC.w) - abs(projectorHMC.xy) );

    // Projector clip to NDC
    float3 projectorNDC = projectorHMC.xyz / projectorHMC.w;

    //// Step 3: Sample decal texture as if we were rendering from the projector's pov
    // NDC to UV
    float2 projectorUV = (projectorNDC.xy * float2(0.5f, -0.5f)) + 0.5f;

    return decalTexture.Sample(texSampler, projectorUV);
}