#include "CustomEffect.h"
#include "ReadData.h"

CustomEffect::CustomEffect(ID3D11Device* device, const std::wstring& vsFilename, const std::wstring& psFilename)
{
    // Create vertex shader
    m_vertexShaderBlob = DX::ReadData(vsFilename.c_str());

    device->CreateVertexShader(m_vertexShaderBlob.data(), m_vertexShaderBlob.size(), nullptr, m_vertexShader.ReleaseAndGetAddressOf());

    // Create pixel shader
    std::vector<uint8_t> psBlob = DX::ReadData(psFilename.c_str());

    device->CreatePixelShader(psBlob.data(), psBlob.size(), nullptr, m_pixelShader.ReleaseAndGetAddressOf());

    // Store vertex shader bytecode so that it can be used to create input layouts
    m_vertexShaderBytecode.code = m_vertexShaderBlob.data();
    m_vertexShaderBytecode.length = m_vertexShaderBlob.size();
}

void CustomEffect::Apply(ID3D11DeviceContext * deviceContext)
{
    // Set vertex and fragment shader
    deviceContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    deviceContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

    // NOTE: ModelMesh::PrepareForRendering takes care of states (raster states, samplers, depth, etc.)
    // NOTE: Drawing (handling of vertex buffers, setting input layouts, etc.) is done in the Model/Mesh/MeshPart class
}

void CustomEffect::GetVertexShaderBytecode(void const ** pShaderByteCode, size_t * pByteCodeLength)
{
    *pShaderByteCode = m_vertexShaderBytecode.code;
    *pByteCodeLength = m_vertexShaderBytecode.length;
}

