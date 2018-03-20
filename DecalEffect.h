#pragma once
#include "CustomEffect.h"

class DecalEffect : public CustomEffect
{
public:
    explicit DecalEffect(ID3D11Device* device);

    void Apply(ID3D11DeviceContext* context, ID3D11ShaderResourceView* projectorDepthTexture, ID3D11ShaderResourceView* decalTexture);

    // IEffectMatrices interface
    void XM_CALLCONV SetWorld(FXMMATRIX value) final;
    void XM_CALLCONV SetView(FXMMATRIX value) final;
    void XM_CALLCONV SetProjection(FXMMATRIX value) final;
    void XM_CALLCONV SetWorldToProjectorClip(FXMMATRIX value);
    void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, FXMMATRIX worldToProjectorClip);

private:
    // Hide CustomEffect interface
    using CustomEffect::Apply;

    void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) final;

    __declspec(align(16))
        struct MatrixBuffer
    {
        XMMATRIX world;
        XMMATRIX view;
        XMMATRIX projection;
        XMMATRIX worldToProjectorClip;
    } m_matrices;
    ConstantBuffer<MatrixBuffer>    m_matrixBuffer;

    ID3D11ShaderResourceView* m_decalTexture;
};

