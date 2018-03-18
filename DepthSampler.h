#pragma once
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

    void ReadDepthValue(ID3D11DeviceContext* context);

    float GetExponentialDepthValue() const { return m_exponentialDepth; }
    //float XM_CALLCONV GetLinearDepthValue(D3D11_VIEWPORT viewport, DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection) const;

private:
    // The 1x1 texture which will be rendered to
    Microsoft::WRL::ComPtr<ID3D11Texture2D>             m_depthSampleTexture;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>   m_depthSampleUAV;

    // Staging textures CANNOT be bound to the render pipeline, and so I need a second texture to use for staging
    Microsoft::WRL::ComPtr<ID3D11Texture2D>             m_stagingTexture;

    __declspec(align(16)) struct SettingsBuffer
    {
        float sampleX, sampleY;
    } m_settings;
    Microsoft::WRL::ComPtr<ID3D11Buffer>    m_settingsBuffer;

    //  "[...] because depth-stencil formats and multisample layouts are implementation
    //  details of a particular GPU design, the operating system can’t expose these formats
    //  and layouts to the CPU in general. Therefore, staging resources can't be a depth-stencil
    //  buffer or a multisampled render target."
    // SOUNDS LIKE: A USAGE_STAGING buffer can be a render target, as long as it is not multisampled!
    //              Fortunately, it seems like I will be using it as a UAV anyway

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

