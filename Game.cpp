//
// Game.cpp
//

#include "pch.h"
#include "Game.h"
#include "DisplayObject.h"
#include <string>
#include "ReadData.h"
#include <WICTextureLoader.h>

using namespace DirectX;
//using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

Game::Game()
{
    // NOTE: Need to use DSS format R24G8_TYPELESS because I am creating a SRV for it
	m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_R24G8_TYPELESS);
	m_deviceResources->RegisterDeviceNotify(this);
	m_displayList.clear();

	//initial Settings
	//modes
	m_grid = false;
}

Game::~Game()
{

	#ifdef DXTK_AUDIO
	if (m_audEngine)
	{
		m_audEngine->Suspend();
	}
	#endif
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
	m_gamePad = std::make_unique<GamePad>();

	m_keyboard = std::make_unique<Keyboard>();

	m_mouse = std::make_unique<Mouse>();
	m_mouse->SetWindow(window);

	m_deviceResources->SetWindow(window, width, height);

	m_deviceResources->CreateDeviceResources();
	CreateDeviceDependentResources();

	m_deviceResources->CreateWindowSizeDependentResources();
	CreateWindowSizeDependentResources();

	m_sprites->SetViewport(m_deviceResources->GetScreenViewport());

	#ifdef DXTK_AUDIO
	// Create DirectXTK for Audio objects
	AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
	#ifdef _DEBUG
	eflags = eflags | AudioEngine_Debug;
	#endif

	m_audEngine = std::make_unique<AudioEngine>(eflags);

	m_audioEvent = 0;
	m_audioTimerAcc = 10.f;
	m_retryDefault = false;

	m_waveBank = std::make_unique<WaveBank>(m_audEngine.get(), L"adpcmdroid.xwb");

	m_soundEffect = std::make_unique<SoundEffect>(m_audEngine.get(), L"MusicMono_adpcm.wav");
	m_effect1 = m_soundEffect->CreateInstance();
	m_effect2 = m_waveBank->CreateInstance(10);

	m_effect1->Play(true);
	m_effect2->Play();
	#endif

    CD3D11_DEPTH_STENCIL_DESC dsDesc(CD3D11_DEFAULT{});

	// Depth test parameters
	dsDesc.DepthEnable = false; // Disable depth test when writing to the stencil buffer (outline will always show, regardless of occlusion)

	// Stencil test parameters
	dsDesc.StencilEnable = true;
	dsDesc.StencilReadMask = STENCIL_SELECTED_OBJECT;
    dsDesc.StencilWriteMask = STENCIL_SELECTED_OBJECT;

	// Stencil operations if pixel is front-facing
	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing
	dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create depth stencil state
    m_deviceResources->GetD3DDevice()->CreateDepthStencilState(&dsDesc, m_stencilReplaceState.ReleaseAndGetAddressOf());

    // THEORY: Enabling depth test for this DSS means that the move hover will not "pass through" objects (to the terrain underneath/behind)
    dsDesc.DepthEnable = true;
    dsDesc.StencilReadMask = STENCIL_TERRAIN;
    dsDesc.StencilWriteMask = STENCIL_TERRAIN;

    m_deviceResources->GetD3DDevice()->CreateDepthStencilState(&dsDesc, m_stencilReplaceStateTerrain.ReleaseAndGetAddressOf());

    dsDesc.DepthEnable = false;
    dsDesc.StencilReadMask = STENCIL_SELECTED_OBJECT;
    dsDesc.StencilWriteMask = STENCIL_SELECTED_OBJECT;

	// Stencil operations if pixel is front-facing
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;

	// Stencil operations if pixel is back-facing
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;

	// Create depth stencil state
    m_deviceResources->GetD3DDevice()->CreateDepthStencilState(&dsDesc, m_stencilTestState.ReleaseAndGetAddressOf());

    dsDesc.StencilReadMask = STENCIL_TERRAIN;
    dsDesc.StencilWriteMask = STENCIL_TERRAIN;

	m_deviceResources->GetD3DDevice()->CreateDepthStencilState(&dsDesc, m_stencilTestStateTerrain.ReleaseAndGetAddressOf());
}

void Game::SetGridState(bool state)
{
	m_grid = state;
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick(InputCommands *Input)
{
	//copy over the input commands so we have a local version to use elsewhere.
	inputCommands = *Input;
	m_timer.Tick([&]()
	{
		Update(m_timer);
	});

	#ifdef DXTK_AUDIO
	// Only update audio engine once per frame
	if (!m_audEngine->IsCriticalError() && m_audEngine->Update())
	{
		// Setup a retry in 1 second
		m_audioTimerAcc = 1.f;
		m_retryDefault = true;
	}
	#endif

	Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
	m_camera.Update(inputCommands);

	//apply camera vectors
	m_view = m_camera.GetViewMatrix();

	m_batchEffect->SetView(m_view);
	m_batchEffect->SetWorld(SimpleMath::Matrix::Identity);
	m_displayChunk.m_terrainEffect->SetView(m_view);
	m_displayChunk.m_terrainEffect->SetWorld(SimpleMath::Matrix::Identity);

    // Projective texturing business


	#ifdef DXTK_AUDIO
	m_audioTimerAcc -= (float) timer.GetElapsedSeconds();
	if (m_audioTimerAcc < 0)
	{
		if (m_retryDefault)
		{
			m_retryDefault = false;
			if (m_audEngine->Reset())
			{
				// Restart looping audio
				m_effect1->Play(true);
			}
		}
		else
		{
			m_audioTimerAcc = 4.f;

			m_waveBank->Play(m_audioEvent++);

			if (m_audioEvent >= 11)
				m_audioEvent = 0;
		}
	}
	#endif


}

void Game::PostProcess(ID3D11DeviceContext* context)
{
    D3D11_VIEWPORT viewport = m_deviceResources->GetScreenViewport();
    RECT fullscreenRect{ 0, 0, viewport.Width, viewport.Height };

    // Blend in the terrain w/ projected texture on it
    m_sprites->Begin(SpriteSortMode_Immediate, m_states->Additive());
    m_sprites->Draw(m_rt3SRV.Get(), fullscreenRect);
    m_sprites->End();

    // Selection highlighting
    if (!m_selectionIDs.empty())
    {
        RECT quarterRect{ 0, 0, viewport.Width / 4, viewport.Height / 4 };
        D3D11_VIEWPORT quarterViewport{ 0, 0, quarterRect.right, quarterRect.bottom, viewport.MinDepth, viewport.MaxDepth };

        // Downscale highlight buffer
        context->OMSetRenderTargets(1, m_rt1RTV.GetAddressOf(), nullptr);

        m_sprites->Begin(SpriteSortMode_Immediate);
        m_sprites->Draw(m_highlightSRV.Get(), quarterRect);
        m_sprites->End();

        // Blur highlight buffer
        context->OMSetRenderTargets(1, m_rt2RTV.GetAddressOf(), nullptr);
        context->RSSetViewports(1, &quarterViewport);

        m_blurPostProcess->SetEffect(BasicPostProcess::GaussianBlur_5x5);
        m_blurPostProcess->SetGaussianParameter(1.f);

        // Source texture is the texture I rendered the flat representaton of the selected model(s) onto
        m_blurPostProcess->SetSourceTexture(m_rt1SRV.Get());
        m_blurPostProcess->Process(context);

        // NOTE: Upscaling is done "implicitly" by applying the blurred texture on a sprite that covers the entire back-buffer
        ID3D11RenderTargetView* rtv[] = { m_deviceResources->GetBackBufferRenderTargetView() };
        context->RSSetViewports(1, &viewport);

        context->OMSetRenderTargets(1, rtv, m_deviceResources->GetDepthStencilView());

        // Render the blurred texture on top of the already rendered screen (with additive blending) while respecting the stencil buffer
        m_sprites->Begin(SpriteSortMode_Immediate, m_states->Additive(), nullptr, nullptr, nullptr, [&]
        {
            // Do not blend the blurred texture with the areas occupied by a selected object
            context->OMSetDepthStencilState(m_stencilTestState.Get(), STENCIL_SELECTED_OBJECT);
        });
        m_sprites->Draw(m_rt2SRV.Get(), fullscreenRect);
        m_sprites->End();
    }
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0)
	{
		return;
	}

	Clear();

	m_deviceResources->PIXBeginEvent(L"Render");
	auto context = m_deviceResources->GetD3DDeviceContext();

	if (m_grid)
	{
		// Draw procedurally generated dynamic grid
		const XMVECTORF32 xaxis = { 512.f, 0.f, 0.f };
		const XMVECTORF32 yaxis = { 0.f, 0.f, 512.f };
		DrawGrid(xaxis, yaxis, g_XMZero, 512, 512, Colors::Gray);
	}

	//RENDER OBJECTS FROM SCENEGRAPH
	{
		int i = 0;
		for (auto itModel = m_displayList.cbegin(); itModel != m_displayList.cend(); ++itModel, ++i)
		{
			auto model = itModel->m_model;
			assert(model != nullptr);

			m_deviceResources->PIXBeginEvent(L"Draw model");
			const XMVECTORF32 scale = { itModel->m_scale.x, itModel->m_scale.y, itModel->m_scale.z };
			const XMVECTORF32 translate = { itModel->m_position.x, itModel->m_position.y, itModel->m_position.z };

			//convert degrees into radians for rotation matrix
			XMVECTOR rotate = SimpleMath::Quaternion::CreateFromYawPitchRoll(XMConvertToRadians(itModel->m_orientation.y),
																 XMConvertToRadians(itModel->m_orientation.x),
																 XMConvertToRadians(itModel->m_orientation.z));

			XMMATRIX local = m_world * XMMatrixTransformation(g_XMZero, SimpleMath::Quaternion::Identity, scale, g_XMZero, rotate, translate);

            // First, render object normally
			model->Draw(context, *m_states, local, m_view, m_projection, false);	// second to last variable in draw,  make TRUE for wireframe

            // Render selected objects to the stencil buffer and to a separate render target in a flat colour
            if (std::find(m_selectionIDs.cbegin(), m_selectionIDs.cend(), itModel->m_ID) != m_selectionIDs.cend())
            {
                // Change render target, but use the same depth/stencil buffer
                context->OMSetRenderTargets(1, m_highlightRTV.GetAddressOf(), m_deviceResources->GetDepthStencilView());

                m_highlightEffect->SetMatrices(local, m_view, m_projection);

                int j = 0;
                for (auto itMesh = model->meshes.cbegin(); itMesh != model->meshes.cend(); ++itMesh)
                {
                    auto mesh = itMesh->get();
                    assert(mesh != 0);

                    mesh->PrepareForRendering(context, *m_states);

                    for (auto pit = mesh->meshParts.cbegin(); pit != mesh->meshParts.cend(); ++pit)
                    {
                        auto part = pit->get();
                        assert(part != 0);

                        part->Draw(context, m_highlightEffect.get(), m_highlightEffectLayouts[mesh->name][j++].Get(), [&]
                        {
                            // Custom DSS so the mesh will be rendered to the stencil buffer also
                            context->OMSetDepthStencilState(m_stencilReplaceState.Get(), STENCIL_SELECTED_OBJECT);
                        });
                    }
                }

                // Reset render target
                ID3D11RenderTargetView* rtv[] = { m_deviceResources->GetBackBufferRenderTargetView() };
                context->OMSetRenderTargets(1, rtv, m_deviceResources->GetDepthStencilView());
            }

			//	//// TODO: Geometry shader to render pivot point
			//	//context->RSSetState(m_states->CullCounterClockwise());
			//	//context->GSSetShader(m_pivotGeometryShader.Get(), nullptr, 0);
			//	//m_pivotEffect->SetMatrices(local, m_view, m_projection);
			//	//m_pivotEffect->Apply(context);
			//	//// Render the pivot point
			//	//m_pivotBatch->Begin();
			//	//m_pivotBatch->Draw(D3D_PRIMITIVE_TOPOLOGY_POINTLIST, &DirectX::VertexPosition(itModel->m_position), 1);
			//	//m_pivotBatch->End();

			//	//// Unbind geometry shader
			//	//context->GSSetShader(nullptr, nullptr, 0);
			//}

			m_deviceResources->PIXEndEvent();
		}
	}
	m_deviceResources->PIXEndEvent();

	//RENDER TERRAIN
	context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
	context->RSSetState(m_states->CullNone());

    ID3D11SamplerState* samplers[] = { m_states->AnisotropicWrap() };
    context->PSSetSamplers(0, 1, samplers);

	//Render the batch,  This is handled in the Display chunk becuase it has the potential to get complex
    // NOTE: Assuming PrimitiveBatch doesn't mess with states
    context->OMSetDepthStencilState(m_stencilReplaceStateTerrain.Get(), STENCIL_TERRAIN);

	m_displayChunk.RenderBatch(m_deviceResources);


    //// Render terrain again, but this time with projective texturing
    //context->OMSetRenderTargets(1, m_rt3RTV.GetAddressOf(), m_deviceResources->GetDepthStencilView());
    //context->OMSetDepthStencilState(m_states->DepthRead(), 0);

    //m_displayChunk.RenderBatch(m_deviceResources, true);


    //context->OMSetDepthStencilState(m_states->DepthDefault(), 0);

    // Sampling depth shenanigans
    m_depthSampler->ReadDepthValue(context);

    float exponentialDepth = m_depthSampler->GetExponentialDepthValue();
    XMVECTOR wsCoord = m_depthSampler->GetWorldSpaceCoordinate(m_deviceResources->GetScreenViewport(), m_world, m_view, m_projection);

    XMVECTOR projectorPosition = wsCoord + (XMVectorSet(0.f, 1.f, 0.f, 0.f) * 2.f);
    XMVECTOR projectorFocus = projectorPosition - XMVectorSet(0.f, 1.f, 0.f, 0.f);

    XMMATRIX projectorView = XMMatrixLookAtLH(projectorPosition, projectorFocus, XMVectorSet(0.f, 1.f, 0.f, 0.f));
    // FIX: near + far should be taken from the viewport
    XMMATRIX projectorProjection = XMMatrixOrthographicLH(256.f, 256.f, 0.1f, 1000.f);

    // Matrix for transforming homogenous clip space to UV space
    static const XMMATRIX toTextureSpace(
        0.5f, 0.f, 0.f, 0.f,
        0.f, -0.5f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.5f, 0.5f, 0.f, 1.f
    );

    XMMATRIX projectorTransform = projectorView * projectorProjection * toTextureSpace;

    // wsCoord: ({wsCoord.m128_f32[0]}, {wsCoord.m128_f32[1]}, {wsCoord.m128_f32[2]})
    m_depthSampler->Execute(context, (float) inputCommands.mouseX, (float) inputCommands.mouseY, m_deviceResources->GetDepthStencilShaderResourceView());

    //// FIX: Volume decal experiments--didn't work..
    //m_volumeDecal->SetMatrices(XMMatrixScaling(3.f, 5.f, 3.f) * XMMatrixTranslation(0.f, 2.f, 0.f), m_view, m_projection);

    //m_volumeDecal->Prepare(context);
    //m_volumeDecal->Apply(context, m_deviceResources->GetDepthStencilShaderResourceView());

    //m_decalCube->Draw(m_volumeDecal.get(), m_volumeDecalInputLayout.Get(), true, false, [&]
    //{
    //    ID3D11SamplerState* samplers[] = { m_states->PointClamp(), m_linearBorderSS.Get() };
    //    context->PSSetSamplers(0, 2, samplers);

    //    // NOTE: Probably unnecessary since depth/stencil buffer is not bound
    //    context->OMSetDepthStencilState(m_states->DepthNone(), 0);
    //});

    //context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
    //m_volumeDecal->Finish(context);

    PostProcess(context);

	// DRAW SELECTION RECT
	if (inputCommands.selectionRectangleBegin.x != -1)
	{
		long selRectBeginX = inputCommands.selectionRectangleBegin.x;
		long selRectBeginY = inputCommands.selectionRectangleBegin.y;
		long selRectEndX = inputCommands.selectionRectangleEnd.x;
		long selRectEndY = inputCommands.selectionRectangleEnd.y;

		RECT selectionRect =
		{
			(selRectBeginX < selRectEndX ? selRectBeginX : selRectEndX),	// left
			(selRectBeginY < selRectEndY ? selRectBeginY : selRectEndY),	// top
			(selRectBeginX >= selRectEndX ? selRectBeginX : selRectEndX),	// right
			(selRectBeginY >= selRectEndY ? selRectBeginY : selRectEndY)	// bottom
		};

		// FIX: Not actually transparent
		m_sprites->Begin(SpriteSortMode::SpriteSortMode_Deferred, m_states->AlphaBlend());
		m_sprites->Draw(m_selectionBoxTexture.Get(), selectionRect);
		m_sprites->End();
	}
    
    // HUD
    m_sprites->Begin();
    DirectX::SimpleMath::Vector3 camPosition = m_camera.GetPosition();
    WCHAR   Buffer[256];
    std::wstring var = L"Cam X: " + std::to_wstring(camPosition.x) + L" Cam Z: " + std::to_wstring(camPosition.z);
    m_font->DrawString(m_sprites.get(), var.c_str(), XMFLOAT2(100, 10), Colors::Yellow);
    m_sprites->End();

	m_deviceResources->Present();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
	m_deviceResources->PIXBeginEvent(L"Clear");

	// Clear the views.
	auto context = m_deviceResources->GetD3DDeviceContext();
	auto renderTarget = m_deviceResources->GetBackBufferRenderTargetView();
	auto depthStencil = m_deviceResources->GetDepthStencilView();

	context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
	context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    context->ClearRenderTargetView(m_highlightRTV.Get(), Colors::Black);
    context->ClearRenderTargetView(m_rt1RTV.Get(), Colors::Black);
    context->ClearRenderTargetView(m_rt2RTV.Get(), Colors::Black);
    context->ClearRenderTargetView(m_rt3RTV.Get(), Colors::Black);

	// Set the viewport.
	auto viewport = m_deviceResources->GetScreenViewport();
	context->RSSetViewports(1, &viewport);

	m_deviceResources->PIXEndEvent();
}

void XM_CALLCONV Game::DrawGrid(FXMVECTOR xAxis, FXMVECTOR yAxis, FXMVECTOR origin, size_t xdivs, size_t ydivs, GXMVECTOR color)
{
	m_deviceResources->PIXBeginEvent(L"Draw grid");

	auto context = m_deviceResources->GetD3DDeviceContext();
	context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(m_states->DepthNone(), 0);
	context->RSSetState(m_states->CullCounterClockwise());

	m_batchEffect->Apply(context);

	context->IASetInputLayout(m_batchInputLayout.Get());

	m_batch->Begin();

	xdivs = std::max<size_t>(1, xdivs);
	ydivs = std::max<size_t>(1, ydivs);

	for (size_t i = 0; i <= xdivs; ++i)
	{
		float fPercent = float(i) / float(xdivs);
		fPercent = (fPercent * 2.0f) - 1.0f;
		XMVECTOR vScale = XMVectorScale(xAxis, fPercent);
		vScale = XMVectorAdd(vScale, origin);

		VertexPositionColor v1(XMVectorSubtract(vScale, yAxis), color);
		VertexPositionColor v2(XMVectorAdd(vScale, yAxis), color);
		m_batch->DrawLine(v1, v2);
	}

	for (size_t i = 0; i <= ydivs; i++)
	{
		float fPercent = float(i) / float(ydivs);
		fPercent = (fPercent * 2.0f) - 1.0f;
		XMVECTOR vScale = XMVectorScale(yAxis, fPercent);
		vScale = XMVectorAdd(vScale, origin);

		VertexPositionColor v1(XMVectorSubtract(vScale, xAxis), color);
		VertexPositionColor v2(XMVectorAdd(vScale, xAxis), color);
		m_batch->DrawLine(v1, v2);
	}

	m_batch->End();

	m_deviceResources->PIXEndEvent();
}

bool XM_CALLCONV Game::CreateScreenSpaceBoundingBox(const DirectX::BoundingBox & modelSpaceBB, FXMMATRIX local, DirectX::BoundingBox& screenSpaceBB) const
{
	const auto viewport = m_deviceResources->GetScreenViewport();

	XMVECTOR center = XMLoadFloat3(&modelSpaceBB.Center);
	XMVECTOR extents = XMLoadFloat3(&modelSpaceBB.Extents);

	// project all corners to screen space
	static const XMVECTOR offsets[8] =
	{
		XMVectorSet(-1.f, -1.f, -1.f, 1.f),		// top left front
		XMVectorSet(-1.f, -1.f,  1.f, 1.f),		// top left back
		XMVectorSet(-1.f,  1.f, -1.f, 1.f),		// bottom left front
		XMVectorSet(-1.f,  1.f,  1.f, 1.f),		// bottom left back
		XMVectorSet( 1.f,  1.f,  1.f, 1.f),		// top right front
		XMVectorSet( 1.f,  1.f, -1.f, 1.f),		// top right back
		XMVectorSet( 1.f, -1.f,  1.f, 1.f),		// bottom right front
		XMVectorSet( 1.f, -1.f, -1.f, 1.f)		// bottom right back
	};

	// Including storing/loading in annotation because the alternate method does not require it
	// (Measuring overhead to be fair)
	m_deviceResources->PIXBeginEvent(L"Selection box picking: XMVector3ProjectStream");

	// calculate corners
	XMFLOAT3 cornerStream[8];
	for (int i = 0; i < 8; ++i)
	{
		XMVECTOR corner = center + (extents * offsets[i]);
		XMStoreFloat3(&cornerStream[i], corner);
	}

	// do the actual projection
	// NOTE: From looking at nsight, XMVector3ProjectStream approach takes avg. 2µs
	//        while XMVector3Project (8 times) takes ~7µs
	//       So while it looks a bit more unruly, there is some evidence to justify the choice
	XMFLOAT3 projectedCornerStream[8];
	XMVector3ProjectStream(projectedCornerStream, sizeof(XMFLOAT3), cornerStream, sizeof(XMFLOAT3), 8,
						   0.f, 0.f, viewport.Width, viewport.Height, 0.f, 1.f, m_projection, m_view, local);

	std::vector<XMVECTOR> corners(8);
	for (int i = 0; i < 8; ++i)
		corners[i] = XMLoadFloat3(&projectedCornerStream[i]);

	m_deviceResources->PIXEndEvent();

	// Check if projected corners are within viewport range ([0, viewport.Width - 1], [0, viewport.Height - 1], [0, 1])
	// NOTE: Could be solved earlier/more cleanly with frustum culling
	const XMVECTOR lowBounds = XMVectorZero();
	const XMVECTOR highBounds = XMVectorSet(viewport.Width, viewport.Height, 1.f, 0.f);
	
	for (auto corner = corners.begin(); corner != corners.end();)
	{
		bool inLowBounds = XMVector3GreaterOrEqual(*corner, lowBounds);
		bool inHighBounds = XMVector3Less(*corner, highBounds);

		if (!(inLowBounds && inHighBounds))
			corner = corners.erase(corner);
		else
			++corner;
	}

	// There are not enough corners(vertices) left to create a bounding box...
	if (corners.size() < 2)
		return false;

	// Calculate min coordinates
	XMVECTOR min = XMVectorSplatInfinity();
	for (int i = 0; i < corners.size(); ++i)
		min = XMVectorMin(min, corners[i]);

	float minX = XMVectorGetX(min);
	float minY = XMVectorGetY(min);
	float minZ = XMVectorGetZ(min);

	// Calculate max coordinates
	XMVECTOR max = XMVectorZero();
	for (int i = 0; i < corners.size(); ++i)
		max = XMVectorMax(max, corners[i]);

	float maxX = XMVectorGetX(max);
	float maxY = XMVectorGetY(max);
	float maxZ = XMVectorGetZ(max);

	// Create screen-space bounding box
	screenSpaceBB.CreateFromPoints(screenSpaceBB, XMVectorSet(minX, minY, minZ, 1.f), XMVectorSet(maxX, maxY, maxZ, 1.f));

	return true;
}

auto Game::GetDisplayListIteratorFromID(int id)
{
    return std::find_if(m_displayList.begin(), m_displayList.end(),
                        [&](const auto& object) {return object.m_ID == id; });
}

auto Game::GetDisplayListIteratorFromID(int id) const
{
    return std::find_if(m_displayList.cbegin(), m_displayList.cend(),
                        [&](const auto& object) {return object.m_ID == id; });
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
}

void Game::OnDeactivated()
{
}

void Game::OnSuspending()
{
	#ifdef DXTK_AUDIO
	m_audEngine->Suspend();
	#endif
}

void Game::OnResuming()
{
	m_timer.ResetElapsedTime();

	#ifdef DXTK_AUDIO
	m_audEngine->Resume();
	#endif
}

void Game::OnWindowSizeChanged(int width, int height)
{
	if (!m_deviceResources->WindowSizeChanged(width, height))
		return;

	CreateWindowSizeDependentResources();
}

void Game::BuildDisplayList(std::vector<SceneObject> * SceneGraph)
{
    using namespace std::placeholders;

    // clear out any old data if this is a rebuild
    if (!m_displayList.empty())
    {
        m_displayList.clear();
        m_highlightEffectLayouts.clear();
    }

	// Create a visual representation of every scene object
    std::for_each(SceneGraph->cbegin(), SceneGraph->cend(), std::bind(&Game::AddDisplayListItem, this, _1));
}

void Game::BuildDisplayChunk(ChunkObject * SceneChunk)
{
	//populate our local DISPLAYCHUNK with all the chunk info we need from the object stored in toolmain
	//which, to be honest, is almost all of it. Its mostly rendering related info so...
	m_displayChunk.PopulateChunkData(SceneChunk);		//migrate chunk data
	m_displayChunk.LoadHeightMap(m_deviceResources);
	m_displayChunk.m_terrainEffect->SetProjection(m_projection);
	m_displayChunk.InitialiseBatch();
}

void Game::SaveDisplayChunk(ChunkObject * SceneChunk)
{
	m_displayChunk.SaveHeightMap();			//save heightmap to file.
}

bool Game::AddDisplayListItem(const SceneObject & sceneObject)
{
    auto device = m_deviceResources->GetD3DDevice();
    auto context = m_deviceResources->GetD3DDeviceContext();

    //create a temp display object that we will populate then append to the display list.
    DisplayObject newDisplayObject;

    newDisplayObject.m_ID = sceneObject.ID;

    //load model
    std::wstring modelwstr = StringToWCHART(sceneObject.model_path);							//convect string to Wchar
    newDisplayObject.m_model = Model::CreateFromCMO(device, modelwstr.c_str(), *m_fxFactory, true);	//get DXSDK to load model "False" for LH coordinate system (maya)

                                                                                                    //Load Texture
    std::wstring texturewstr = StringToWCHART(sceneObject.tex_diffuse_path);								//convect string to Wchar
    HRESULT rs;
    rs = CreateDDSTextureFromFile(device, texturewstr.c_str(), nullptr, &newDisplayObject.m_texture_diffuse);	//load tex into Shader resource

                                                                                                                //if texture fails.  load error default
    if (rs)
    {
        CreateDDSTextureFromFile(device, L"database/data/Error.dds", nullptr, &newDisplayObject.m_texture_diffuse);	//load tex into Shader resource
    }

    //apply new texture to models effect
    newDisplayObject.m_model->UpdateEffects([&](IEffect* effect) //This uses a Lambda function,  if you dont understand it: Look it up.
    {
        auto lights = dynamic_cast<BasicEffect*>(effect);
        if (lights)
        {
            lights->SetTexture(newDisplayObject.m_texture_diffuse);
        }
    });

    // Create input layout for the selection highlight effect
    for (auto mit = newDisplayObject.m_model->meshes.cbegin(); mit != newDisplayObject.m_model->meshes.cend(); ++mit)
    {
        auto mesh = mit->get();
        assert(mesh != 0);

        for (auto it = mesh->meshParts.cbegin(); it != mesh->meshParts.cend(); ++it)
        {
            auto part = it->get();
            assert(part != 0);

            Microsoft::WRL::ComPtr<ID3D11InputLayout> il;
            part->CreateInputLayout(m_deviceResources->GetD3DDevice(), m_highlightEffect.get(), il.GetAddressOf());

            // Add part input layout for the model
            m_highlightEffectLayouts[mesh->name].emplace_back(il);
        }
    }

    //set position
    newDisplayObject.m_position.x = sceneObject.posX;
    newDisplayObject.m_position.y = sceneObject.posY;
    newDisplayObject.m_position.z = sceneObject.posZ;

    //setorientation
    newDisplayObject.m_orientation.x = sceneObject.rotX;
    newDisplayObject.m_orientation.y = sceneObject.rotY;
    newDisplayObject.m_orientation.z = sceneObject.rotZ;

    //set scale
    newDisplayObject.m_scale.x = sceneObject.scaX;
    newDisplayObject.m_scale.y = sceneObject.scaY;
    newDisplayObject.m_scale.z = sceneObject.scaZ;

    //set wireframe / render flags
    newDisplayObject.m_render = sceneObject.editor_render;
    newDisplayObject.m_wireframe = sceneObject.editor_wireframe;

    m_displayList.push_back(newDisplayObject);


    return true;
}

void Game::UpdateDisplayListItem(const SceneObject & sceneObject)
{
    auto it = GetDisplayListIteratorFromID(sceneObject.ID);

    if (it == m_displayList.end())
        return;

    it->m_position.x = sceneObject.posX;
    it->m_position.y = sceneObject.posY;
    it->m_position.z = sceneObject.posZ;

    it->m_orientation.x = sceneObject.rotX;
    it->m_orientation.y = sceneObject.rotY;
    it->m_orientation.z = sceneObject.rotZ;

    it->m_scale.x = sceneObject.scaX;
    it->m_scale.y = sceneObject.scaY;
    it->m_scale.z = sceneObject.scaZ;
}

bool Game::RemoveDisplayListItem(int id)
{
    auto it = GetDisplayListIteratorFromID(id);

    if (it == m_displayList.cend())
        return false;

    // TODO: Do something about the input layouts as well--if the last of a mesh type has been deleted,
    //       perhaps that input layout (for highlight effect) should be deleted also?
    //       - Somehow utilise shared_ptr for this purpose?

    m_displayList.erase(it);
    return true;
}

bool Game::Pick(POINT cursorPos, RECT clientRect, int& id) const
{
	// Points on the near and far plane in screen space
	const XMVECTOR nearPoint = XMVectorSet(cursorPos.x, cursorPos.y, 0.f, 0.f);
	const XMVECTOR farPoint = XMVectorSet(cursorPos.x, cursorPos.y, 1.f, 0.f);

	// NOTE: Inefficient due to the scene graph not actually being a graph
	int nearestID = -1;
	float nearestDist = std::numeric_limits<float>::max();
	for (auto itModel = m_displayList.cbegin(); itModel != m_displayList.cend(); ++itModel)
	{
		auto model = itModel->m_model;
		assert(model != nullptr);

		for (auto itMesh = model->meshes.cbegin(); itMesh != model->meshes.cend(); ++itMesh)
		{
			auto mesh = *itMesh;
			assert(mesh != nullptr);

			auto meshBB = mesh->boundingBox;
			auto world = SimpleMath::Matrix::CreateScale(itModel->m_scale) * SimpleMath::Matrix::CreateFromYawPitchRoll(itModel->m_orientation.x, itModel->m_orientation.y, itModel->m_orientation.z) * SimpleMath::Matrix::CreateTranslation(itModel->m_position);

			// Convert points to model space
			// NOTE: Could also use SimpleMath::Viewport::Unproject, but afaik I don't already have a SimpleMath::Viewport
			XMVECTOR nearPointMS = XMVector3Unproject(nearPoint, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom, 0.f, 1.f, m_projection, m_view, world);
			XMVECTOR farPointMS = XMVector3Unproject(farPoint, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom, 0.f, 1.f, m_projection, m_view, world);

			// Calculate normalised direction
			XMVECTOR direction = XMVector3Normalize(farPointMS - nearPointMS);

			// Test for intersection between bounding box and picking ray
			float dist = std::numeric_limits<float>::max();
			if (meshBB.Intersects(nearPointMS, direction, dist))
			{
				if (dist < nearestDist)
				{
					nearestDist = dist;
					nearestID = itModel->m_ID;
				}
			}
		}
	}

	// Check if the click intersected any objects
	if (nearestID != -1)
	{
		id = nearestID;
		return true;
	}

	return false;
}

bool Game::PickWithinScreenRectangle(RECT selectionRect, std::vector<int>& selections, PickingMode mode) const
{
	// only clear selections if we are doing normal picking
	if (mode == PICK_NORMAL)
		selections.clear();

	//// EXPERIMENT: Skewed picking frustum (convert selection box to view space?)
	//XMVECTOR topLeftNear		=	XMVectorSet(selectionRect.left, selectionRect.top, 0.f, 1.f);
	//XMVECTOR bottomLeftNear		=	XMVectorSet(selectionRect.left, selectionRect.bottom, 0.f, 1.f);
	//XMVECTOR topRightNear		=	XMVectorSet(selectionRect.right, selectionRect.top, 0.f, 1.f);
	//XMVECTOR bottomRightNear	=	XMVectorSet(selectionRect.right, selectionRect.bottom, 0.f, 1.f);

	////XMVECTOR topLeftFar			=	XMVectorSet(selectionRect.left, selectionRect.top, 1.f, 1.f);
	////XMVECTOR bottomLeftFar		=	XMVectorSet(selectionRect.left, selectionRect.bottom, 1.f, 1.f);
	////XMVECTOR topRightFar		=	XMVectorSet(selectionRect.right, selectionRect.top, 1.f, 1.f);
	////XMVECTOR bottomRightFar		=	XMVectorSet(selectionRect.right, selectionRect.bottom, 1.f, 1.f);

	//const XMMATRIX identity = XMMatrixIdentity();
	//topLeftNear		= XMVector3Unproject(topLeftNear, 0.f, 0.f, viewport.Width, viewport.Height, 0.f, 1.f, m_projection, identity, identity);
	//bottomLeftNear	= XMVector3Unproject(bottomLeftNear, 0.f, 0.f, viewport.Width, viewport.Height, 0.f, 1.f, m_projection, identity, identity);
	//topRightNear	= XMVector3Unproject(topRightNear, 0.f, 0.f, viewport.Width, viewport.Height, 0.f, 1.f, m_projection, identity, identity);
	//bottomRightNear	= XMVector3Unproject(bottomRightNear, 0.f, 0.f, viewport.Width, viewport.Height, 0.f, 1.f, m_projection, identity, identity);

	////topLeftFar		= XMVector3Unproject(topLeftFar, 0.f, 0.f, viewport.Width, viewport.Height, 0.f, 1.f, m_projection, identity, identity);
	////bottomLeftFar	= XMVector3Unproject(bottomLeftFar, 0.f, 0.f, viewport.Width, viewport.Height, 0.f, 1.f, m_projection, identity, identity);
	////topRightFar		= XMVector3Unproject(topRightFar, 0.f, 0.f, viewport.Width, viewport.Height, 0.f, 1.f, m_projection, identity, identity);
	////bottomRightFar	= XMVector3Unproject(bottomRightFar, 0.f, 0.f, viewport.Width, viewport.Height, 0.f, 1.f, m_projection, identity, identity);

	//XMMATRIX pickingMatrix = XMMatrixPerspectiveOffCenterRH(
	//	XMVectorGetX(topLeftNear),
	//	XMVectorGetX(bottomRightNear),
	//	XMVectorGetY(bottomRightNear),
	//	XMVectorGetY(topRightNear),
	//	0.01f, 1000.f
	//);

	//BoundingFrustum pickingFrustum;
	//BoundingFrustum::CreateFromMatrix(pickingFrustum, pickingMatrix);
	//
	//// Iterate over every model in the scene (inefficient)
	//for (auto itModel = m_displayList.cbegin(); itModel != m_displayList.cend(); ++itModel)
	//{
	//	auto model = itModel->m_model;
	//	assert(model != 0);

	//	// calculate world (local) matrix
	//	const XMVECTORF32 scale = { itModel->m_scale.x, itModel->m_scale.y, itModel->m_scale.z };
	//	const XMVECTORF32 translate = { itModel->m_position.x, itModel->m_position.y, itModel->m_position.z };

	//	//convert degrees into radians for rotation matrix
	//	XMVECTOR rotate = Quaternion::CreateFromYawPitchRoll(XMConvertToRadians(itModel->m_orientation.y),
	//														 XMConvertToRadians(itModel->m_orientation.x),
	//														 XMConvertToRadians(itModel->m_orientation.z));

	//	SimpleMath::Matrix local = m_world * XMMatrixTransformation(g_XMZero, Quaternion::Identity, scale, g_XMZero, rotate, translate);

	//	for (auto itMesh = model->meshes.cbegin(); itMesh != model->meshes.cend(); ++itMesh)
	//	{
	//		auto mesh = *itMesh;

	//		auto meshBB = mesh->boundingBox;

	//		meshBB.Transform(meshBB, local * m_view);

	//		if (pickingFrustum.Contains(meshBB))
	//		{
	//			selections.push_back(itModel->m_ID);
	//		}
	//	}
	//}

	////////////////////////////////////////////////////////////////////////
	// EXPERIMENT: Create a screen space bounding box for the selection box
	XMVECTOR topLeftNear = XMVectorSet(selectionRect.left, selectionRect.top, 0.f, 1.f);
	XMVECTOR bottomRightFar = XMVectorSet(selectionRect.right, selectionRect.bottom, 1.f, 1.f);

	BoundingBox pickingBB;
	pickingBB.CreateFromPoints(pickingBB, topLeftNear, bottomRightFar);

	// Iterate over every model in the scene (inefficient)
	for (auto itModel = m_displayList.cbegin(); itModel != m_displayList.cend(); ++itModel)
	{
		auto model = itModel->m_model;
		assert(model != 0);

		// calculate world (local) matrix
		const XMVECTORF32 scale = { itModel->m_scale.x, itModel->m_scale.y, itModel->m_scale.z };
		const XMVECTORF32 translate = { itModel->m_position.x, itModel->m_position.y, itModel->m_position.z };

		//convert degrees into radians for rotation matrix
		XMVECTOR rotate = SimpleMath::Quaternion::CreateFromYawPitchRoll(XMConvertToRadians(itModel->m_orientation.y),
															 XMConvertToRadians(itModel->m_orientation.x),
															 XMConvertToRadians(itModel->m_orientation.z));

		SimpleMath::Matrix local = m_world * XMMatrixTransformation(g_XMZero, SimpleMath::Quaternion::Identity, scale, g_XMZero, rotate, translate);

		for (auto itMesh = model->meshes.cbegin(); itMesh != model->meshes.cend(); ++itMesh)
		{
			auto mesh = *itMesh;

			// get model-space bounding box of mesh
			BoundingBox meshBB = mesh->boundingBox;

			// project to screen-space
			BoundingBox ssMeshBB;
			if (!CreateScreenSpaceBoundingBox(meshBB, local, ssMeshBB))
				break;
			
			// If projected bounding box is inside selection box, add the ID to the selection vector
			if (pickingBB.Contains(ssMeshBB))
			{
				if (mode == PICK_NORMAL)
					selections.push_back(itModel->m_ID);
				else
				{
					// delete the entry if it already exists--otherwise add it
					auto alreadySelectedIt = std::find(selections.cbegin(), selections.cend(), itModel->m_ID);
					
					if (alreadySelectedIt != selections.cend())
						selections.erase(alreadySelectedIt);
					else if (mode == PICK_INVERT)
						selections.push_back(itModel->m_ID);
				}
			}
		}
	}

	return !selections.empty();
}

#ifdef DXTK_AUDIO
void Game::NewAudioDevice()
{
	if (m_audEngine && !m_audEngine->IsAudioDevicePresent())
	{
		// Setup a retry in 1 second
		m_audioTimerAcc = 1.f;
		m_retryDefault = true;
	}
}
#endif

#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
	auto context = m_deviceResources->GetD3DDeviceContext();
	auto device = m_deviceResources->GetD3DDevice();

	m_states = std::make_unique<CommonStates>(device);

	m_fxFactory = std::make_unique<EffectFactory>(device);
	m_fxFactory->SetDirectory(L"database/data/"); //fx Factory will look in the database directory
	m_fxFactory->SetSharing(false);	//we must set this to false otherwise it will share effects based on the initial tex loaded (When the model loads) rather than what we will change them to.

	m_sprites = std::make_unique<SpriteBatch>(context);

	m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(context);

	m_batchEffect = std::make_unique<BasicEffect>(device);
	m_batchEffect->SetVertexColorEnabled(true);

	{
		void const* shaderByteCode;
		size_t byteCodeLength;

		m_batchEffect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

		DX::ThrowIfFailed(
			device->CreateInputLayout(VertexPositionColor::InputElements,
									  VertexPositionColor::InputElementCount,
									  shaderByteCode, byteCodeLength,
									  m_batchInputLayout.ReleaseAndGetAddressOf())
		);
	}
	
	m_highlightEffect = std::make_unique<HighlightEffect>(m_deviceResources->GetD3DDevice());
    m_highlightEffect->SetHighlightColour(Colors::Yellow);

    m_blurPostProcess = std::make_unique<BasicPostProcess>(m_deviceResources->GetD3DDevice());

    m_depthSampler = std::make_unique<DepthSampler>(m_deviceResources->GetD3DDevice(), m_deviceResources->GetDepthBufferFormat());
    m_volumeDecal = std::make_unique<VolumeDecal>(m_deviceResources->GetD3DDevice());

    m_decalCube = GeometricPrimitive::CreateCube(m_deviceResources->GetD3DDeviceContext());
    m_decalCube->CreateInputLayout(m_volumeDecal.get(), m_volumeDecalInputLayout.ReleaseAndGetAddressOf());

    // Load decal texture
    // FIX: Figure out how to load in my own texture
    //DX::ThrowIfFailed(
    //    CreateDDSTextureFromFile(device, L"brush_marker.dds", nullptr, m_brushMarkerDecalSRV.ReleaseAndGetAddressOf())
    //);

	m_font = std::make_unique<SpriteFont>(device, L"SegoeUI_18.spritefont");

	//    m_shape = GeometricPrimitive::CreateTeapot(context, 4.f, 8);

		// SDKMESH has to use clockwise winding with right-handed coordinates, so textures are flipped in U
	m_model = Model::CreateFromSDKMESH(device, L"tiny.sdkmesh", *m_fxFactory);


	// Load textures
	DX::ThrowIfFailed(
		CreateDDSTextureFromFile(device, L"seafloor.dds", nullptr, m_texture1.ReleaseAndGetAddressOf())
	);

	DX::ThrowIfFailed(
		CreateDDSTextureFromFile(device, L"windowslogo.dds", nullptr, m_texture2.ReleaseAndGetAddressOf())
	);

	m_pivotBatch = std::make_unique<PrimitiveBatch<VertexPosition>>(context);
	m_pivotEffect = std::make_unique<BasicEffect>(device);

	{
		void const* shaderByteCode;
		size_t byteCodeLength;

		m_pivotEffect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

		device->CreateInputLayout(VertexPosition::InputElements, VertexPosition::InputElementCount, shaderByteCode, byteCodeLength, m_pivotInputLayout.ReleaseAndGetAddressOf());
	}

	// TODO: Load geometry shader .cso file to blob
	auto blob = DX::ReadData(L"pivot_gs.cso");
	device->CreateGeometryShader(blob.data(), blob.size(), nullptr, m_pivotGeometryShader.ReleaseAndGetAddressOf());


	// Create a 1x1 texture for selection box
	CD3D11_TEXTURE2D_DESC selBoxTexDesc(DXGI_FORMAT_R8G8B8A8_UNORM, 1U, 1U, 1U, 0U, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE);

    PackedVector::XMCOLOR pixel(1.f, 0.f, 0.f, 0.f);
	D3D11_SUBRESOURCE_DATA initialData;
	initialData.pSysMem = &pixel.c;
	initialData.SysMemPitch = sizeof(PackedVector::XMCOLOR);

	Microsoft::WRL::ComPtr<ID3D11Texture2D> selectionTex;
	device->CreateTexture2D(&selBoxTexDesc, &initialData, selectionTex.ReleaseAndGetAddressOf());

	CD3D11_SHADER_RESOURCE_VIEW_DESC selSRVdesc(D3D11_SRV_DIMENSION_TEXTURE2D, selBoxTexDesc.Format);
	device->CreateShaderResourceView(selectionTex.Get(), &selSRVdesc, m_selectionBoxTexture.ReleaseAndGetAddressOf());

    // Set decal texture
    m_volumeDecal->SetDecalTexture(m_texture1.Get());

    static constexpr float transparentBlack[] = { 0.f, 0.f, 0.f, 0.f };
    CD3D11_SAMPLER_DESC ssDesc(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_BORDER, D3D11_TEXTURE_ADDRESS_BORDER, D3D11_TEXTURE_ADDRESS_BORDER,
                               0, 1, D3D11_COMPARISON_NEVER, transparentBlack, -FLT_MAX, FLT_MAX);
    device->CreateSamplerState(&ssDesc, m_linearBorderSS.ReleaseAndGetAddressOf());
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
	auto size = m_deviceResources->GetOutputSize();
	float aspectRatio = float(size.right) / float(size.bottom);
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	// This sample makes use of a right-handed coordinate system using row-major matrices.
	m_projection = SimpleMath::Matrix::CreatePerspectiveFieldOfView(
		fovAngleY,
		aspectRatio,
		0.01f,
		1000.0f
	);

	m_sprites->SetViewport(m_deviceResources->GetScreenViewport());
	m_highlightEffect->SetProjection(m_projection);
	m_batchEffect->SetProjection(m_projection);

	if (m_displayChunk.m_terrainEffect)
	    m_displayChunk.m_terrainEffect->SetProjection(m_projection);

    //// Create views and render targets for post processing stuff
    ID3D11Device* device = m_deviceResources->GetD3DDevice();
    DXGI_FORMAT backBufferFormat = m_deviceResources->GetBackBufferFormat();
    D3D11_VIEWPORT viewport = m_deviceResources->GetScreenViewport();

    //// Create full size render target for the scene to be rendered onto (pre post processing)
    CD3D11_TEXTURE2D_DESC sceneDesc(backBufferFormat, viewport.Width, viewport.Height, 1, 1, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET);

    ComPtr<ID3D11Texture2D> sceneTex;
    device->CreateTexture2D(&sceneDesc, nullptr, sceneTex.ReleaseAndGetAddressOf());
    device->CreateRenderTargetView(sceneTex.Get(), nullptr, m_highlightRTV.ReleaseAndGetAddressOf());
    device->CreateShaderResourceView(sceneTex.Get(), nullptr, m_highlightSRV.ReleaseAndGetAddressOf());

    // Create quarter sized render targets for use with blurring
    CD3D11_TEXTURE2D_DESC rtDesc(backBufferFormat, viewport.Width / 4, viewport.Height / 4, 1, 1, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET);

    ComPtr<ID3D11Texture2D> rtTexture1;
    device->CreateTexture2D(&rtDesc, nullptr, rtTexture1.ReleaseAndGetAddressOf());
    device->CreateRenderTargetView(rtTexture1.Get(), nullptr, m_rt1RTV.ReleaseAndGetAddressOf());
    device->CreateShaderResourceView(rtTexture1.Get(), nullptr, m_rt1SRV.ReleaseAndGetAddressOf());

    ComPtr<ID3D11Texture2D> rtTexture2;
    device->CreateTexture2D(&rtDesc, nullptr, rtTexture2.ReleaseAndGetAddressOf());
    device->CreateRenderTargetView(rtTexture2.Get(), nullptr, m_rt2RTV.ReleaseAndGetAddressOf());
    device->CreateShaderResourceView(rtTexture2.Get(), nullptr, m_rt2SRV.ReleaseAndGetAddressOf());

    // Another FULL SIZE render target
    ComPtr<ID3D11Texture2D> rtTexture3;
    device->CreateTexture2D(&sceneDesc, nullptr, rtTexture3.ReleaseAndGetAddressOf());
    device->CreateRenderTargetView(rtTexture3.Get(), nullptr, m_rt3RTV.ReleaseAndGetAddressOf());
    device->CreateShaderResourceView(rtTexture3.Get(), nullptr, m_rt3SRV.ReleaseAndGetAddressOf());


    // Decal volume
    m_volumeDecal->SetPixelSize(m_deviceResources->GetD3DDeviceContext(), viewport.Width, viewport.Height);
}

void Game::OnDeviceLost()
{
	m_states.reset();
	m_fxFactory.reset();
	m_sprites.reset();
	m_batch.reset();
	m_batchEffect.reset();
	m_font.reset();
	m_shape.reset();
	m_model.reset();
	m_texture1.Reset();
	m_texture2.Reset();
	m_batchInputLayout.Reset();
}

void Game::OnDeviceRestored()
{
	CreateDeviceDependentResources();

	CreateWindowSizeDependentResources();
}
#pragma endregion

std::wstring StringToWCHART(std::string s)
{

	int len;
	int slength = (int) s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}
