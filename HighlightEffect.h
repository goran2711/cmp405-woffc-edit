#pragma once
#include "Effects.h"
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

    // Strongly typed wrapper around a D3D constant buffer.
    template<typename T>
    class ConstantBuffer
    {
    public:
        // Constructor.
        ConstantBuffer() = default;
        explicit ConstantBuffer(_In_ ID3D11Device* device)
        {
            Create(device);
        }

        ConstantBuffer(ConstantBuffer const&) = delete;
        ConstantBuffer& operator= (ConstantBuffer const&) = delete;

        void Create(_In_ ID3D11Device* device)
        {
            D3D11_BUFFER_DESC desc = {};

            desc.ByteWidth = sizeof(T);
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            device->CreateBuffer(&desc, nullptr, mConstantBuffer.ReleaseAndGetAddressOf());

            //SetDebugObjectName(mConstantBuffer.Get(), "DirectXTK");
        }

        // Writes new data into the constant buffer.
        void SetData(_In_ ID3D11DeviceContext* deviceContext, T const& value)
        {
            assert(mConstantBuffer.Get() != 0);

            D3D11_MAPPED_SUBRESOURCE mappedResource;

            deviceContext->Map(mConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

            *(T*) mappedResource.pData = value;

            deviceContext->Unmap(mConstantBuffer.Get(), 0);
        }

        // Looks up the underlying D3D constant buffer.
        ID3D11Buffer* GetBuffer()
        {
            return mConstantBuffer.Get();
        }


    private:
        // The underlying D3D object.
        Microsoft::WRL::ComPtr<ID3D11Buffer> mConstantBuffer;
    };

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

