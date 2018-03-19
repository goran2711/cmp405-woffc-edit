#pragma once
#include "CustomEffect.h"

class ProjectiveTexturingEffect : public CustomEffect
{
public:
    explicit ProjectiveTexturingEffect(ID3D11Device* device);

    void Apply(ID3D11DeviceContext* context) final;

    // IEffectMatrices interface
    void XM_CALLCONV SetWorld(FXMMATRIX value) final;
    void XM_CALLCONV SetView(FXMMATRIX value) final;
    void XM_CALLCONV SetProjection(FXMMATRIX value) final;
    void XM_CALLCONV SetWorldToProjectorUV(FXMMATRIX value);
    void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, FXMMATRIX worldToProjectorUV);

    void SetDecalTexture(ID3D11ShaderResourceView* decalTexture);

private:
    void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) final;

    __declspec(align(16))
        struct MatrixBuffer
    {
        XMMATRIX world;
        XMMATRIX view;
        XMMATRIX projection;
        XMMATRIX worldToProjectorUV;
    } m_matrices;
    ConstantBuffer<MatrixBuffer>    m_matrixBuffer;

    ID3D11ShaderResourceView* m_decalTexture;
};

