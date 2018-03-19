#include "DepthSampler.h"
#include "ReadData.h"

using namespace DirectX;
using namespace Microsoft::WRL;

DepthSampler::DepthSampler(ID3D11Device* device, DXGI_FORMAT depthBufferFormat)
{
    // Create buffer to store the result in
    CD3D11_TEXTURE2D_DESC bufDesc(DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 1, 1, 0, D3D11_BIND_UNORDERED_ACCESS, D3D11_USAGE_DEFAULT);

    device->CreateTexture2D(&bufDesc, nullptr, m_depthSampleTexture.ReleaseAndGetAddressOf());

    bufDesc.BindFlags = 0;
    bufDesc.Usage = D3D11_USAGE_STAGING;
    bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    device->CreateTexture2D(&bufDesc, nullptr, m_stagingTexture.ReleaseAndGetAddressOf());

    CD3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc(D3D11_UAV_DIMENSION_TEXTURE2D, bufDesc.Format);
    device->CreateUnorderedAccessView(m_depthSampleTexture.Get(), &uavDesc, m_depthSampleUAV.ReleaseAndGetAddressOf());

    // Create const buffer
    m_settingsBuffer.Create(device);

    // Create compute shader
    auto csBlob = DX::ReadData(L"depthSample_cs.cso");
    device->CreateComputeShader(csBlob.data(), csBlob.size(), nullptr, m_computeShader.ReleaseAndGetAddressOf());
}

void DepthSampler::Execute(ID3D11DeviceContext* context, float x, float y, ID3D11ShaderResourceView* depthStencilSRV)
{
    // Unbind the depth stencil buffer so that it can be used as input
    ID3D11RenderTargetView* renderTargetView;
    ID3D11DepthStencilView* depthStencilView;
    context->OMGetRenderTargets(1, &renderTargetView, &depthStencilView);
    context->OMSetRenderTargets(1, &renderTargetView, nullptr);

    // Set const buffer
    m_settings.sampleX = x;
    m_settings.sampleY = y;

    m_settingsBuffer.SetData(context, m_settings);

    context->CSSetConstantBuffers(0, 1, m_settingsBuffer.GetAddressOfBuffer());

    // Set SRV and UAV
    context->CSSetShaderResources(0, 1, &depthStencilSRV);
    context->CSSetUnorderedAccessViews(0, 1, m_depthSampleUAV.GetAddressOf(), nullptr);

    // Dispatch shader
    context->CSSetShader(m_computeShader.Get(), nullptr, 0);
    context->Dispatch(1, 1, 1);

    // Unbind the depth stencil buffer from input and re-bind it as output
    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->CSSetShaderResources(0, 1, &nullSRV);
    context->OMSetRenderTargets(1, &renderTargetView, depthStencilView);

    m_samplePoint.x = x;
    m_samplePoint.y = y;
    m_firstRun = false;
}

void DepthSampler::ReadDepthValue(ID3D11DeviceContext* context)
{
    if (m_firstRun)
        return;

    // Copy sampled value to staging texture
    context->CopyResource(m_stagingTexture.Get(), m_depthSampleTexture.Get());

    // Copy staging texture to CPU
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    context->Map(m_stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mappedResource);

    // pData is probably 4 floats
    m_exponentialDepth = static_cast<float*>(mappedResource.pData)[0];

    context->Unmap(m_stagingTexture.Get(), 0);
}

XMVECTOR XM_CALLCONV DepthSampler::GetWorldSpaceCoordinate(D3D11_VIEWPORT viewport, FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection) const
{
    XMVECTOR samplePoint = XMVectorSet(m_samplePoint.x, m_samplePoint.y, m_exponentialDepth, 1.f);

    return XMVector3Unproject(samplePoint, 0.f, 0.f, viewport.Width, viewport.Height, viewport.MinDepth, viewport.MaxDepth, projection, view, world);
}


