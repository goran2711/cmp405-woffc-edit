#pragma once
#include "ConstantBuffer.h"
#include <d3d11_1.h>
#include <DirectXMath.h>
#include <memory>
#include <wrl.h>

// This class will provide functionality for retrieving the depth value from the depth buffer at a specified coordinate 
// - It could potentially wait until the next frame to actually read from the GPU, so as to not completely flush the pipeline (kill parallelism)

class DepthSampler
{
public:
    DepthSampler(ID3D11Device* device, DXGI_FORMAT depthBufferFormat);

    void Execute(ID3D11DeviceContext* context, float x, float y, ID3D11ShaderResourceView* depthStencilSRV);

    // TODO: Figure out the best place to read depth
    // NOTE: The depth value should be read in the NEXT frame, not immediately after Execute is called because it kills performance
    void ReadDepthValue(ID3D11DeviceContext* context);

    float GetExponentialDepthValue() const { return m_exponentialDepth; }
    DirectX::XMVECTOR XM_CALLCONV GetWorldSpaceCoordinate(D3D11_VIEWPORT viewport, DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection) const;

private:
    // The 1x1 texture which the sampled depth value will be output to
    Microsoft::WRL::ComPtr<ID3D11Texture2D>             m_depthSampleTexture;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>   m_depthSampleUAV;

    //  "[...] because depth-stencil formats and multisample layouts are implementation
    //  details of a particular GPU design, the operating system can’t expose these formats
    //  and layouts to the CPU in general. Therefore, staging resources can't be a depth-stencil
    //  buffer or a multisampled render target."
    //  ! Staging textures CANNOT be bound to the render pipeline, and so I need a second texture to use for staging
    Microsoft::WRL::ComPtr<ID3D11Texture2D>             m_stagingTexture;

    __declspec(align(16)) struct SettingsBuffer
    {
        float sampleX, sampleY;
    } m_settings;
    ConstantBuffer<SettingsBuffer>  m_settingsBuffer;

    // The compute shader
    Microsoft::WRL::ComPtr<ID3D11ComputeShader>     m_computeShader;

    // The sampled value and the point from which it was sampled
    struct
    {
        long x, y;
    }       m_samplePoint;
    float   m_exponentialDepth;

    bool m_firstRun = true;
};

