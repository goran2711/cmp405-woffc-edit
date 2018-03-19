#pragma once
#include "Effects.h"
#include "ConstantBuffer.h"
#include <vector>

class VolumeDecal : public DirectX::IEffect, public DirectX::IEffectMatrices
{
public:
    explicit VolumeDecal(ID3D11Device* device);

    void __cdecl GetVertexShaderBytecode(_Out_ void const** pShaderByteCode, _Out_ size_t* pByteCodeLength) final;

    // DecalMatrices interface
    void XM_CALLCONV SetWorld(DirectX::FXMMATRIX value) final { m_matrices.world = value; }
    void XM_CALLCONV SetView(DirectX::FXMMATRIX value) final { m_matrices.view = value; }
    void XM_CALLCONV SetProjection(DirectX::FXMMATRIX value) final { m_matrices.projection = value; }
    void XM_CALLCONV SetMatrices(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection);

    //// VolumeDecal interface

    // Unbinds the depth/stencil buffer so that it may be set as input
    // NOTE: May be poor design in case the user would like to use on "off-screen" depth/stencil buffer
    void Prepare(ID3D11DeviceContext* context);

    // NOTE: It's probably going to be necessary to set custom sampler states for this (do in Prepare?)
    void __cdecl Apply(_In_ ID3D11DeviceContext* context, ID3D11ShaderResourceView* depthStencilSRV);

    // Unbinds the depth/stencil buffer from input and binds it to the render target again
    void Finish(ID3D11DeviceContext* context);

    void SetPixelSize(ID3D11DeviceContext* context, float viewportWidth, float viewportHeight);
    void SetDecalTexture(ID3D11ShaderResourceView* decalTexture);

private:
    // IEffect interface
    void __cdecl Apply(_In_ ID3D11DeviceContext* context) final;

    __declspec(align(16))
        struct DecalMatrices
    {
        // Transforms the decal volume to the right spot
        DirectX::XMMATRIX world;
        DirectX::XMMATRIX view;
        DirectX::XMMATRIX projection;

        // Computes the position of the geometry inside the decal volume relative to itself
        DirectX::XMMATRIX screenToLocal;
    };

    __declspec(align(16))
        struct FixedBuffer
    {
        float pixelSizeX, pixelSizeY;
    };

    // Constant buffers
    DecalMatrices   m_matrices;
    ConstantBuffer<DecalMatrices>   m_matrixBuffer;

    ConstantBuffer<FixedBuffer>    m_fixedBuffer;

    // Textures
    ID3D11ShaderResourceView*   m_decal = nullptr;

    // Shaders
    Microsoft::WRL::ComPtr<ID3D11VertexShader>  m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_pixelShader;

    struct ShaderBytecode
    {
        void const* code;
        size_t length;
    } m_vertexShaderBytecode;
    std::vector<uint8_t> m_vertexShaderBlob;

    // State/housekeeping
    ID3D11RenderTargetView* m_rtv = nullptr;
    ID3D11DepthStencilView* m_dsv = nullptr;
};
