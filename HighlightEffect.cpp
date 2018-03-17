#include "HighlightEffect.h"
#include "ReadData.h"


HighlightEffect::HighlightEffect(ID3D11Device* device)
    : m_vertexShaderBytecode(new ShaderBytecode(), [](ShaderBytecode* vertexShaderBytecode)
{
    if (vertexShaderBytecode->code)
        delete[] vertexShaderBytecode->code;
})
{
    // Create vertex shader
    std::vector<uint8_t> vsBlob = DX::ReadData(L"highlight_vs.cso");

    device->CreateVertexShader(vsBlob.data(), vsBlob.size(), nullptr, m_vertexShader.ReleaseAndGetAddressOf());

    // Create pixel shader
    std::vector<uint8_t> psBlob = DX::ReadData(L"highlight_ps.cso");

    device->CreatePixelShader(psBlob.data(), psBlob.size(), nullptr, m_pixelShader.ReleaseAndGetAddressOf());

    // Store vertex shader bytecode so that it can be used to create input layouts
    m_vertexShaderBytecode->code = new std::vector<uint8_t>(vsBlob);
    m_vertexShaderBytecode->length = vsBlob.size();
}

void HighlightEffect::Apply(ID3D11DeviceContext * deviceContext)
{
    // TODO: Set vertex and fragment shader
    deviceContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    deviceContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

    // TODO: Set constant buffers
    ID3D11Buffer* matriceBuffer[] = { m_matrixBuffer.GetBuffer() };
    deviceContext->VSSetConstantBuffers(0, 1, matriceBuffer);

    ID3D11Buffer* propertiesBuffer[] = { m_propertiesBuffer.GetBuffer() };
    deviceContext->PSSetConstantBuffers(0, 1, propertiesBuffer);

    // NOTE: ModelMesh::PrepareForRendering takes care of states (raster states, samplers, depth, etc.)
    // NOTE: Drawing (handling of vertex buffers, setting input layouts, etc.) is done in the Model/Mesh/MeshPart class
}

void HighlightEffect::GetVertexShaderBytecode(void const ** pShaderByteCode, size_t * pByteCodeLength)
{
    pShaderByteCode = &m_vertexShaderBytecode->code;
    pByteCodeLength = &m_vertexShaderBytecode->length;
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
