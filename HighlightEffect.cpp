#include "HighlightEffect.h"
#include "DirectXColors.h"

HighlightEffect::HighlightEffect(ID3D11Device * device)
    :   CustomEffect(device, L"highlight_vs.cso", L"highlight_ps.cso")
{
    // Create constant buffers
    m_matrixBuffer.Create(device);
    m_propertiesBuffer.Create(device);

    // Initialise highlight colour
    m_highlightProperties.colour = Colors::White;
}

void HighlightEffect::Apply(ID3D11DeviceContext* deviceContext)
{
    CustomEffect::Apply(deviceContext);

    // Set constant buffers
    m_matrixBuffer.SetData(deviceContext, m_matrices);
    m_propertiesBuffer.SetData(deviceContext, m_highlightProperties);

    deviceContext->VSSetConstantBuffers(0, 1, m_matrixBuffer.GetAddressOfBuffer());
    deviceContext->PSSetConstantBuffers(0, 1, m_propertiesBuffer.GetAddressOfBuffer());
}

void XM_CALLCONV HighlightEffect::SetHighlightColour(FXMVECTOR colour)
{
    m_highlightProperties.colour = colour;
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