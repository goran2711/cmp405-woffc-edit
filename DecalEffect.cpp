#include "DecalEffect.h"

DecalEffect::DecalEffect(ID3D11Device* device)
    :   CustomEffect(device, L"decal_vs.cso", L"decal_ps.cso")
{
    // Create const buffer
    m_matrixBuffer.Create(device);
}

void DecalEffect::Apply(ID3D11DeviceContext* context, ID3D11ShaderResourceView* projectorDepthTexture, ID3D11ShaderResourceView* decalTexture)
{
    Apply(context);

    // Bind const buffers
    m_matrixBuffer.SetData(context, m_matrices);
    context->VSSetConstantBuffers(0, 1, m_matrixBuffer.GetAddressOfBuffer());

    // Bind textures
    ID3D11ShaderResourceView* textures[] = { projectorDepthTexture, decalTexture };
    context->PSSetShaderResources(0, 2, textures);
}

void XM_CALLCONV DecalEffect::SetWorld(FXMMATRIX value)
{
    m_matrices.world = value;
}

void XM_CALLCONV DecalEffect::SetView(FXMMATRIX value)
{
    m_matrices.view = value;
}

void XM_CALLCONV DecalEffect::SetProjection(FXMMATRIX value)
{
    m_matrices.projection = value;
}

void XM_CALLCONV DecalEffect::SetWorldToProjectorClip(FXMMATRIX value)
{
    m_matrices.worldToProjectorClip = value;
}

void XM_CALLCONV DecalEffect::SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, FXMMATRIX worldToProjectorClip)
{
    SetMatrices(world, view, projection);

    m_matrices.worldToProjectorClip = worldToProjectorClip;
}

void XM_CALLCONV DecalEffect::SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection)
{
    m_matrices.world = world;
    m_matrices.view = view;
    m_matrices.projection = projection;
}