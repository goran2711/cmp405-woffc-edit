#pragma once
#include "Effects.h"
#include "ConstantBuffer.h"
#include <memory>
#include <vector>
#include <wrl.h>

// This shader will render the back-faces of a mesh using a vertex shader that performs
// vertex manipulation by extruding them along their normal, and a simple flat pixel shader

// I should only need one shader (no permutations)

using namespace DirectX;

class HighlightEffect : public IEffect, public IEffectMatrices
{
    // Helper stores matrix parameter values, and computes derived matrices.
    struct EffectMatrices
    {
        EffectMatrices()
        {
            world = XMMatrixIdentity();
            view = XMMatrixIdentity();
            projection = XMMatrixIdentity();
            worldView = XMMatrixIdentity();
        }

        XMMATRIX world;
        XMMATRIX view;
        XMMATRIX projection;
        XMMATRIX worldView;

        //void SetConstants(_Inout_ int& dirtyFlags, _Inout_ XMMATRIX& worldViewProjConstant);
    };

    // Points to a precompiled vertex or pixel shader program.
    struct ShaderBytecode
    {
        void const* code;
        size_t length;
    };

public:
    explicit HighlightEffect(_In_ ID3D11Device* device);

    // TODO: Move/copy/destructors


    // IEffect interface
    void __cdecl Apply(_In_ ID3D11DeviceContext* deviceContext) final;

    // Used by DirectX::ModelMeshPart in CreateInputLayout (and ModifyEffect)
    void __cdecl GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength) final;

    // IEffectMatrices interface
    void XM_CALLCONV SetWorld(FXMMATRIX value) final;
    void XM_CALLCONV SetView(FXMMATRIX value) final;
    void XM_CALLCONV SetProjection(FXMMATRIX value) final;
    void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) final;

    // HighlightEffect-specific interface
    void XM_CALLCONV SetHighlightColour(FXMVECTOR colour);

protected:

private:
    __declspec(align(16))
        struct HighlightProperties
    {
        XMVECTOR colour;
    };

    // CPU resources
    // NOTE: This one has some EffectDirtyFlags business I'm don't fully comprehend yet--might want to look into it
    EffectMatrices m_matrices;
    HighlightProperties m_highlightProperties;

    std::vector<uint8_t> m_vertexShaderBlob;
    ShaderBytecode m_vertexShaderBytecode;

    // GPU resources
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;

    ConstantBuffer<EffectMatrices> m_matrixBuffer;

    ConstantBuffer<HighlightProperties> m_propertiesBuffer;
};

