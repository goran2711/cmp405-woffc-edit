#pragma once
#include <d3d11_1.h>
#include <wrl.h>
#include <cassert>

// NOTE: Implementation taken from https://github.com/Microsoft/DirectXTK/blob/a0dfe4d69746628f11788dce3d862d55a6604340/Src/ConstantBuffer.h
//       Because NuGet version of DXTK does not include ConstantBuffer.h for some reason

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

    ID3D11Buffer* const * GetAddressOfBuffer() const
    {
        return mConstantBuffer.GetAddressOf();
    }


private:
    // The underlying D3D object.
    Microsoft::WRL::ComPtr<ID3D11Buffer> mConstantBuffer;
};
