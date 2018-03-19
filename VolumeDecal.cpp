#include "VolumeDecal.h"
#include "ReadData.h"

using namespace DirectX;

VolumeDecal::VolumeDecal(ID3D11Device* device)
{
    // Create vertex shader
    m_vertexShaderBlob = DX::ReadData(L"volumeDecal_vs.cso");

    device->CreateVertexShader(m_vertexShaderBlob.data(), m_vertexShaderBlob.size(), nullptr, m_vertexShader.ReleaseAndGetAddressOf());

    // Create pixel shader
    std::vector<uint8_t> psBlob = DX::ReadData(L"volumeDecal_ps.cso");

    device->CreatePixelShader(psBlob.data(), psBlob.size(), nullptr, m_pixelShader.ReleaseAndGetAddressOf());

    // Store vertex shader bytecode so that it can be used to create input layouts
    m_vertexShaderBytecode.code = m_vertexShaderBlob.data();
    m_vertexShaderBytecode.length = m_vertexShaderBlob.size();

    // Create constat buffers
    m_fixedBuffer.Create(device);
    m_matrixBuffer.Create(device);
}

void VolumeDecal::Prepare(ID3D11DeviceContext* context)
{
    // Keep a hold of them so we can set them back later
    context->OMGetRenderTargets(1, &m_rtv, &m_dsv);

    // Unbind depth/stencil buffer
    context->OMSetRenderTargets(1, &m_rtv, nullptr);
}

void VolumeDecal::Apply(ID3D11DeviceContext* context, ID3D11ShaderResourceView* depthStencilSRV)
{
    // Set shader resources
    ID3D11ShaderResourceView* textures[] = { depthStencilSRV, m_decal };
    context->PSSetShaderResources(0, 2, textures);

    Apply(context);
}

void VolumeDecal::Finish(ID3D11DeviceContext* context)
{
    // Unbind stencil buffer (both textures, actually) from input
    static ID3D11ShaderResourceView* nullSRV[] = { nullptr, nullptr };
    context->PSSetShaderResources(0, 2, nullSRV);

    // Rebind depth/stencil buffer
    context->OMSetRenderTargets(1, &m_rtv, m_dsv);
}

void VolumeDecal::Apply(ID3D11DeviceContext* context)
{
    // Set shaders
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);

    // Set and bind constant buffers
    m_matrixBuffer.SetData(context, m_matrices);

    context->VSSetConstantBuffers(0, 1, m_matrixBuffer.GetAddressOfBuffer());

    ID3D11Buffer* psConstBuffers[] = { m_matrixBuffer.GetBuffer(), m_fixedBuffer.GetBuffer() };
    context->PSSetConstantBuffers(0, 2, psConstBuffers);

}

void VolumeDecal::GetVertexShaderBytecode(void const ** pShaderByteCode, size_t * pByteCodeLength)
{
    *pShaderByteCode = m_vertexShaderBytecode.code;
    *pByteCodeLength = m_vertexShaderBytecode.length;
}

void XM_CALLCONV VolumeDecal::SetMatrices(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection)
{
    // Create a matrix that transforms a screen-space coordinate to decal volume's UV space
    static const XMMATRIX texToClip = XMMatrixSet(
         2.f,  0.f, 0.f, 0.f,
         0.f, -2.f, 0.f, 0.f,
         0.f,  0.f, 1.f, 0.f,
        -1.f,  1.f, 0.f, 1.f
    );

    XMMATRIX invViewProjection = XMMatrixInverse(nullptr, view * projection);
    XMMATRIX screenToLocal = texToClip * invViewProjection * world;

    m_matrices = {
        world,
        view,
        projection,
        screenToLocal
    };
}

void VolumeDecal::SetPixelSize(ID3D11DeviceContext* context, float viewportWidth, float viewportHeight)
{
    // Set the buffer data immediately since it is not supposed to change very often
    FixedBuffer pixelSize{ 1.f / viewportWidth, 1.f / viewportHeight };
    m_fixedBuffer.SetData(context, pixelSize);
}

void VolumeDecal::SetDecalTexture(ID3D11ShaderResourceView * decalTexture)
{
    m_decal = decalTexture;
}
