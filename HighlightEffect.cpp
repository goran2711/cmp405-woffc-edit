#include "HighlightEffect.h"
#include "ReadData.h"


HighlightEffect::HighlightEffect(ID3D11Device* device)
{
    // Create vertex shader
    m_vertexShaderBlob = DX::ReadData(L"highlight_vs.cso");

    device->CreateVertexShader(m_vertexShaderBlob.data(), m_vertexShaderBlob.size(), nullptr, m_vertexShader.ReleaseAndGetAddressOf());

    // Create pixel shader
    std::vector<uint8_t> psBlob = DX::ReadData(L"highlight_ps.cso");

    device->CreatePixelShader(psBlob.data(), psBlob.size(), nullptr, m_pixelShader.ReleaseAndGetAddressOf());

    // Store vertex shader bytecode so that it can be used to create input layouts
    m_vertexShaderBytecode.code = m_vertexShaderBlob.data();
    m_vertexShaderBytecode.length = m_vertexShaderBlob.size();

    // Create constant buffers
    m_matrixBuffer.Create(device);
    m_propertiesBuffer.Create(device);
}

void HighlightEffect::Apply(ID3D11DeviceContext * deviceContext)
{
    // Set vertex and fragment shader
    deviceContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    deviceContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

    // Set constant buffers
    m_matrixBuffer.SetData(deviceContext, m_matrices);

    ID3D11Buffer* matriceBuffer[] = { m_matrixBuffer.GetBuffer() };
    deviceContext->VSSetConstantBuffers(0, 1, matriceBuffer);

    m_propertiesBuffer.SetData(deviceContext, { XMVectorSet(1.f, 0.f, 0.f, 1.f) });

    ID3D11Buffer* propertiesBuffer[] = { m_propertiesBuffer.GetBuffer() };
    deviceContext->PSSetConstantBuffers(0, 1, propertiesBuffer);

    // NOTE: ModelMesh::PrepareForRendering takes care of states (raster states, samplers, depth, etc.)
    // NOTE: Drawing (handling of vertex buffers, setting input layouts, etc.) is done in the Model/Mesh/MeshPart class
}

void HighlightEffect::GetVertexShaderBytecode(void const ** pShaderByteCode, size_t * pByteCodeLength)
{
    *pShaderByteCode = m_vertexShaderBytecode.code;
    *pByteCodeLength = m_vertexShaderBytecode.length;
}

void XM_CALLCONV HighlightEffect::SetWorld(FXMMATRIX value)
{
    m_matrices.world = value;
}

void XM_CALLCONV HighlightEffect::SetView(FXMMATRIX value)
{
    m_matrices.view = value;
}

void XM_CALLCONV HighlightEffect::SetProjection(FXMMATRIX value)
{
    m_matrices.projection = value;
}

void XM_CALLCONV HighlightEffect::SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection)
{
    m_matrices.world = world;
    m_matrices.view = view;
    m_matrices.projection = projection;
}
