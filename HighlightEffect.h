#pragma once
#include "Effects.h"
#include "EffectCommon.h"
#include "ConstantBuffer.h"
#include <memory>

// This shader will render the back-faces of a mesh using a vertex shader that performs
// vertex manipulation by extruding them along their normal, and a simple flat pixel shader

// I should only need one shader (no permutations)

using namespace DirectX;

class HighlightEffect : public IEffect, public IEffectMatrices
{
public:
    explicit HighlightEffect(_In_ ID3D11Device* device);

    // TODO: Move/copy/destructors


    // IEffect interface
    void __cdecl Apply(_In_ ID3D11DeviceContext* deviceContext) override;

    // Used by DirectX::ModelMeshPart in CreateInputLayout (and ModifyEffect)
    void __cdecl GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength) override;

    // IEffectMatrices interface
    void XM_CALLCONV SetWorld(FXMMATRIX value) override;
    void XM_CALLCONV SetView(FXMMATRIX value) override;
    void XM_CALLCONV SetProjection(FXMMATRIX value) override;
    void XM_CALLCONV SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) override;

    // TODO: Interface to set vertex(highlight) colour

protected:

private:
    // CPU resources
    // NOTE: This one has some EffectDirtyFlags business I'm don't fully comprehend yet--might want to look into it
    EffectMatrices m_matrices;
    
    std::unique_ptr<ShaderBytecode, void(*)(ShaderBytecode*)> m_vertexShaderBytecode;

    // GPU resources
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;

    ConstantBuffer<EffectMatrices> m_matrixBuffer;

    __declspec(align(16))
        struct HighlightProperties
    {
        XMVECTOR colour;
    };

    ConstantBuffer<HighlightProperties> m_propertiesBuffer;
};

