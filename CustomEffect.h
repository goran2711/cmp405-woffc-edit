#pragma once
#include "Effects.h"
#include "ConstantBuffer.h"
#include <memory>
#include <vector>
#include <wrl.h>

// Effect class that loads and prepares a vertex and pixel shader

using namespace DirectX;

class CustomEffect : public IEffect, public IEffectMatrices
{
    // Points to a precompiled vertex or pixel shader program.
    struct ShaderBytecode
    {
        void const* code;
        size_t length;
    };

public:
    CustomEffect(_In_ ID3D11Device* device, const std::wstring& vsFilename, const std::wstring& psFilename);

    virtual ~CustomEffect() = default;

    // IEffect interface
    virtual void __cdecl Apply(_In_ ID3D11DeviceContext* deviceContext) override;

    // Used by DirectX::ModelMeshPart in CreateInputLayout (and ModifyEffect)
    virtual void __cdecl GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength) override;

private:
    // CPU resources
    std::vector<uint8_t> m_vertexShaderBlob;
    ShaderBytecode m_vertexShaderBytecode;

    // GPU resources
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
};

