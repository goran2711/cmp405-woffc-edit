#include "ProjectiveTexturingEffect.h"

ProjectiveTexturingEffect::ProjectiveTexturingEffect(ID3D11Device* device)
    :   CustomEffect(device, L"projectiveTexturing_vs.cso", L"projectiveTexturing_ps.cso")
{
    // Create const buffer
    m_matrixBuffer.Create(device);
}

void ProjectiveTexturingEffect::Apply(ID3D11DeviceContext* context)
{
    CustomEffect::Apply(context);

    // Bind const buffers
    m_matrixBuffer.SetData(context, m_matrices);
    context->VSSetConstantBuffers(0, 1, m_matrixBuffer.GetAddressOfBuffer());

    // Bind textures
    context->PSSetShaderResources(0, 1, &m_decalTexture);
}

void XM_CALLCONV ProjectiveTexturingEffect::SetWorld(FXMMATRIX value)
{
    m_matrices.world = value;
}

void XM_CALLCONV ProjectiveTexturingEffect::SetView(FXMMATRIX value)
{
    m_matrices.view = value;
}

void XM_CALLCONV ProjectiveTexturingEffect::SetProjection(FXMMATRIX value)
{
    m_matrices.projection = value;
}

void XM_CALLCONV ProjectiveTexturingEffect::SetProjectorViewProjection(FXMMATRIX value)
{
    m_matrices.projectorViewProjection = value;
}

void XM_CALLCONV ProjectiveTexturingEffect::SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, FXMMATRIX projectorViewProjection)
{
    SetMatrices(world, view, projection);

    m_matrices.projectorViewProjection = projectorViewProjection;
}

void ProjectiveTexturingEffect::SetDecalTexture(ID3D11ShaderResourceView* decalTexture)
{
    m_decalTexture = decalTexture;
}

void XM_CALLCONV ProjectiveTexturingEffect::SetMatrices(FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection)
{
    m_matrices.world = world;
    m_matrices.view = view;
    m_matrices.projection = projection;
}