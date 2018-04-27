#pragma once
#include "CustomEffect.h"

class HighlightEffect : public CustomEffect
{
public:
    explicit HighlightEffect(_In_ ID3D11Device* device);

    // IEffect interface
    virtual void __cdecl Apply(_In_ ID3D11DeviceContext* deviceContext) override;

    // IEffectMatrices interface
    virtual void XM_CALLCONV SetWorld(FXMMATRIX value) override;
    virtual void XM_CALLCONV SetView(FXMMATRIX value) override;
    virtual void XM_CALLCONV SetProjection(FXMMATRIX value) override;
    virtual void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

    // HighlightEffect-specific interface
    void XM_CALLCONV SetHighlightColour(FXMVECTOR colour);

private:
    __declspec(align(16))
        struct HighlightProperties
    {
        XMVECTOR colour;
    };

    HighlightProperties m_highlightProperties;
    ConstantBuffer<HighlightProperties> m_propertiesBuffer;

    // Helper stores matrix parameter values
    struct EffectMatrices
    {
        EffectMatrices()
        {
            world = XMMatrixIdentity();
            view = XMMatrixIdentity();
            projection = XMMatrixIdentity();
        }

        XMMATRIX world;
        XMMATRIX view;
        XMMATRIX projection;
    };

    EffectMatrices m_matrices;
    ConstantBuffer<EffectMatrices> m_matrixBuffer;
};