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

/* static */ const XMVECTORF32 Game::HIGHLIGHT_COLOUR = { 1.f, 1.f, 0.f, 0.5f };

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

    CD3D11_DEPTH_STENCIL_DESC dsDesc(
        FALSE,
        D3D11_DEPTH_WRITE_MASK_ALL,
        D3D11_COMPARISON_LESS,
        TRUE,
        STENCIL_SELECTED_OBJECT,
        STENCIL_SELECTED_OBJECT,
        D3D11_STENCIL_OP_KEEP,
        D3D11_STENCIL_OP_KEEP,
        D3D11_STENCIL_OP_REPLACE,
        D3D11_COMPARISON_ALWAYS,
        
        D3D11_STENCIL_OP_KEEP,
        D3D11_STENCIL_OP_KEEP,
        D3D11_STENCIL_OP_REPLACE,
        D3D11_COMPARISON_ALWAYS
    );

    // DSS for writing selected object to stencil buffer
    m_deviceResources->GetD3DDevice()->CreateDepthStencilState(&dsDesc, m_dss[DSS_WRITE_SELECTED_OBJECT].ReleaseAndGetAddressOf());

    // DSS for reading selected object from stencil buffer (pass if read value != STENCIL_SELECTED_OBJECT--makes stencil act as cutout)
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsDesc.StencilReadMask = STENCIL_SELECTED_OBJECT;
    dsDesc.StencilWriteMask = STENCIL_SELECTED_OBJECT;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;

    m_deviceResources->GetD3DDevice()->CreateDepthStencilState(&dsDesc, m_dss[DSS_NOT_EQ_SELECTED_OBJECT].ReleaseAndGetAddressOf());

    // DSS for writing terrain to stencil buffer
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.StencilReadMask = STENCIL_TERRAIN;
    dsDesc.StencilWriteMask = STENCIL_TERRAIN;
    dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
    dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
    dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    m_deviceResources->GetD3DDevice()->CreateDepthStencilState(&dsDesc, m_dss[DSS_WRITE_TERRAIN].ReleaseAndGetAddressOf());

    // DSS for reading terrain from stencil buffer (pass if read value == STENCIL_TERRAIN--makes decal only appear on terrain)
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
    dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	m_deviceResources->GetD3DDevice()->CreateDepthStencilState(&dsDesc, m_dss[DSS_EQ_TERRAIN].ReleaseAndGetAddressOf());
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
    m_displayChunk.m_terrainEffect->SetProjection(m_projection);
	m_displayChunk.m_terrainEffect->SetWorld(SimpleMath::Matrix::Identity);


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

void Game::RenderSceneGraph(ID3D11DeviceContext * context, FXMMATRIX view, CXMMATRIX projection)
{
	const XMVECTOR zero = XMVectorZero();

    for (auto itModel = m_displayList.cbegin(); itModel != m_displayList.cend(); ++itModel)
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

        XMMATRIX local = m_world * XMMatrixTransformation(zero, SimpleMath::Quaternion::Identity, scale, zero, rotate, translate);

        // First, render object normally
        model->Draw(context, *m_states, local, view, projection, false);	// second to last variable in draw,  make TRUE for wireframe

        // Render selected objects to the stencil buffer and to a separate render target in a flat colour
        if (std::find(m_selectionIDs.cbegin(), m_selectionIDs.cend(), itModel->m_ID) != m_selectionIDs.cend())
            RenderSelectedObject(context, *model, local, view, projection);

        m_deviceResources->PIXEndEvent();
    }
}

void XM_CALLCONV Game::RenderSelectedObject(ID3D11DeviceContext* context, const DirectX::Model & model, FXMMATRIX local, CXMMATRIX view, CXMMATRIX projection)
{
    // Change render target, but use the same depth/stencil buffer
    context->OMSetRenderTargets(1, m_highlightRTV.GetAddressOf(), m_deviceResources->GetDepthStencilView());
    
    m_highlightEffect->SetMatrices(local, view, projection);
    
    int j = 0;
    for (auto itMesh = model.meshes.cbegin(); itMesh != model.meshes.cend(); ++itMesh)
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
                context->OMSetDepthStencilState(m_dss[DSS_WRITE_SELECTED_OBJECT].Get(), STENCIL_SELECTED_OBJECT);
            });
        }
    }
    
    // Reset render target
    ID3D11RenderTargetView* rtv[] = { m_deviceResources->GetBackBufferRenderTargetView() };
    context->OMSetRenderTargets(1, rtv, m_deviceResources->GetDepthStencilView());
}

void Game::PostProcess(ID3D11DeviceContext* context)
{
    D3D11_VIEWPORT viewport = m_deviceResources->GetScreenViewport();
    RECT fullscreenRect{ 0, 0, viewport.Width, viewport.Height };

    // Projected texture / decal
    if (m_showTerrainBrush)
    {
        // Copy depth/stencil buffer because we want to sample it in the pixel shader, but we also want to
        // adhere to the stencil buffer (same resource cannot be bound to input and output simultaneously)
        context->CopyResource(m_depthStencilTexCopy.Get(), m_deviceResources->GetDepthStencilTexture());

        m_decalMatrices.invViewProjection = XMMatrixInverse(nullptr, m_view * m_projection);
        m_decalMatrices.worldToProjectorClip = XMLoadFloat4x4(&m_projectorView) * XMLoadFloat4x4(&m_projectorProjection);
        m_decalMatrixBuffer.SetData(context, m_decalMatrices);

        // Project decal onto terrain
        m_sprites->Begin(SpriteSortMode_Immediate, m_states->NonPremultiplied(), nullptr, nullptr, nullptr, [&]
        {
            // Custom state
            context->OMSetDepthStencilState(m_dss[DSS_EQ_TERRAIN].Get(), STENCIL_TERRAIN);

            // Const buffers
            context->PSSetConstantBuffers(0, 1, m_decalMatrixBuffer.GetAddressOfBuffer());

            // Override shader
            context->PSSetShader(m_decalPixelShader.Get(), nullptr, 0);

            // Textures
            // NOTE: register(t0) is set to m_depthStencilSRVCopy.Get() by SpriteBatch
            context->PSSetShaderResources(1, 1, m_brushDecalTextureSRV.GetAddressOf());
        });
        m_sprites->Draw(m_depthStencilSRVCopy.Get(), fullscreenRect);
        m_sprites->End();

        // Cleanup shader resources (not strictly necessary)
        ID3D11ShaderResourceView* nullSRVs[] = { nullptr, nullptr, nullptr };
        context->PSSetShaderResources(0, 3, nullSRVs);

        context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
    }

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

        // NOTE: Upscaling is done implicitly by applying the blurred texture on a sprite that covers the entire back-buffer
        ID3D11RenderTargetView* rtv[] = { m_deviceResources->GetBackBufferRenderTargetView() };
        context->RSSetViewports(1, &viewport);

        context->OMSetRenderTargets(1, rtv, m_deviceResources->GetDepthStencilView());

        // Render the blurred texture on top of the already rendered screen (with blending) while respecting the stencil buffer
        m_sprites->Begin(SpriteSortMode_Immediate, m_states->AlphaBlend(), nullptr, nullptr, nullptr, [&]
        {
            // Do not blend the blurred texture with the areas occupied by a selected object
            context->OMSetDepthStencilState(m_dss[DSS_NOT_EQ_SELECTED_OBJECT].Get(), STENCIL_SELECTED_OBJECT);
        });
        m_sprites->Draw(m_rt2SRV.Get(), fullscreenRect);
        m_sprites->End();

        context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
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
    RenderSceneGraph(context, m_view, m_projection);
    m_deviceResources->PIXEndEvent();

    //RENDER TERRAIN
    context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
    // Imprint the terrain onto the stencil buffer
    context->OMSetDepthStencilState(m_dss[DSS_WRITE_TERRAIN].Get(), STENCIL_TERRAIN);
    context->RSSetState(m_states->CullNone());

    ID3D11SamplerState* samplers[] = { m_states->AnisotropicWrap() };
    context->PSSetSamplers(0, 1, samplers);

    //Render the batch,  This is handled in the Display chunk becuase it has the potential to get complex
    m_displayChunk.RenderBatch(context, m_view, m_projection);

    context->OMSetDepthStencilState(m_states->DepthDefault(), 0);

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
			std::min(selRectBeginX, selRectEndX),   // left
			std::min(selRectBeginY, selRectEndY),   // top
			std::max(selRectBeginX, selRectEndX),   // right
			std::max(selRectBeginY, selRectEndY)    // bottom
		};

		m_sprites->Begin(SpriteSortMode::SpriteSortMode_Deferred, m_states->NonPremultiplied());
		m_sprites->Draw(m_selectionBoxTexture.Get(), selectionRect);
		m_sprites->End();
	}
    
    // HUD
    m_sprites->Begin();
    std::wstring var =  L"FPS: " + std::to_wstring(m_timer.GetFramesPerSecond()) +
                        L"\nFrame time: " + std::to_wstring(m_timer.GetElapsedSeconds() * 1000) + L"ms";
    m_font->DrawString(m_sprites.get(), var.c_str(), XMFLOAT2(10, 10), Colors::Yellow);
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

	constexpr float TRANSPARENT_BLACK[4] = { 0.f, 0.f, 0.f, 0.f };
    context->ClearRenderTargetView(m_highlightRTV.Get(), TRANSPARENT_BLACK);
    context->ClearRenderTargetView(m_rt1RTV.Get(), TRANSPARENT_BLACK);
    context->ClearRenderTargetView(m_rt2RTV.Get(), TRANSPARENT_BLACK);

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

	const XMVECTOR center = XMLoadFloat3(&modelSpaceBB.Center);
	const XMVECTOR extents = XMLoadFloat3(&modelSpaceBB.Extents);

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

	m_deviceResources->PIXBeginEvent(L"Selection box picking: XMVector3ProjectStream");

	// calculate corners
	XMFLOAT3 cornerStream[8];
	for (int i = 0; i < 8; ++i)
	{
		const XMVECTOR corner = center + (extents * offsets[i]);
		XMStoreFloat3(&cornerStream[i], corner);
	}

	// do the actual projection
	XMFLOAT3 projectedCornerStream[8];
	XMVector3ProjectStream(projectedCornerStream, sizeof(XMFLOAT3), cornerStream, sizeof(XMFLOAT3), 8,
						   0.f, 0.f, viewport.Width, viewport.Height, 0.f, 1.f, m_projection, m_view, local);

	std::vector<XMVECTOR> corners(8);
    std::transform(projectedCornerStream, projectedCornerStream + 8, corners.begin(),
    [](const XMFLOAT3& corner)
    {
        return XMLoadFloat3(&corner);
    });

	m_deviceResources->PIXEndEvent();

	// Check if projected corners are within viewport range ([0, viewport.Width - 1], [0, viewport.Height - 1], [0, 1])
	// NOTE: Could be solved earlier/more cleanly with frustum culling
	const XMVECTOR lowBounds = XMVectorZero();
	const XMVECTOR highBounds = XMVectorSet(viewport.Width, viewport.Height, 1.f, 0.f);
	
	corners.erase(std::remove_if(corners.begin(), corners.end(),
		[lowBounds, highBounds] (FXMVECTOR corner)
	{
		bool inLowBounds = XMVector3GreaterOrEqual(corner, lowBounds);
		bool inHighBounds = XMVector3Less(corner, highBounds);

		return !(inLowBounds && inHighBounds);
	}), corners.end());

	// Guess we're not making a BB then
	if (corners.empty())
		return false;

	// Calculate min coordinates
	XMVECTOR min = XMVectorSplatInfinity();
	for (int i = 0; i < corners.size(); ++i)
		min = XMVectorMin(min, corners[i]);

	// Calculate max coordinates
	XMVECTOR max = XMVectorZero();
	for (int i = 0; i < corners.size(); ++i)
		max = XMVectorMax(max, corners[i]);

	// Create screen-space bounding box
	screenSpaceBB.CreateFromPoints(screenSpaceBB, min, max);

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
	m_displayChunk.LoadHeightMap(m_deviceResources->GetD3DDevice());
	m_displayChunk.InitialiseBatch();
    m_displayChunk.InitialiseRendering(m_deviceResources.get());
	m_displayChunk.m_terrainEffect->SetProjection(m_projection);
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

	// Create a screen space bounding box for the selection box
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

bool Game::CursorIntersectsTerrain(long cursorX, long cursorY, DirectX::XMVECTOR & wsCoord)
{
    const XMVECTOR origin = m_camera.GetPosition();
    return m_displayChunk.CursorIntersectsTerrain(origin, cursorX, cursorY, m_deviceResources->GetScreenViewport(), m_projection, m_view, m_world, wsCoord);
}

void Game::ShowBrushDecal(bool val)
{
    m_showTerrainBrush = val;
}

void XM_CALLCONV Game::SetBrushDecalPosition(FXMVECTOR wsCoord, float brushSize)
{
    // Projector position a little bit above the area the cursor is hovering over
    // NOTE: One thing to be weary of here is that the projector's near plane could potentially clip the terrain
    //        when doing terrain manipulation, since (currently) the BVH is not being regenerated as the geometry
    //        is displaced.
    static constexpr float PROJECTOR_FAR = 256.f;
    static constexpr float PROJECTOR_OFFSET = PROJECTOR_FAR - 1.f;
    XMVECTOR projectorPosition = wsCoord + XMVectorSet(0.f, PROJECTOR_OFFSET, 0.f, 0.f);

    // Projector is focusing on a point below it (towards terrain)
    XMVECTOR projectorFocus = projectorPosition + XMVectorSet(0.f, -1.f, 0.f, 0.f);
    XMMATRIX projectorView = XMMatrixLookAtLH(projectorPosition, projectorFocus, XMVectorSet(0.f, 0.f, 1.f, 0.f));

    // Update projection matrix if brush size changed
    if (m_brushSize != brushSize)
    {
        XMMATRIX projectorProjection = XMMatrixOrthographicLH(brushSize, brushSize, 0.01f, PROJECTOR_FAR);

        XMStoreFloat4x4(&m_projectorProjection, projectorProjection);
        m_brushSize = brushSize;
    }

    XMStoreFloat4x4(&m_projectorView, projectorView);
}

void XM_CALLCONV Game::ManipulateTerrain(DirectX::FXMVECTOR wsCoord, bool elevate, float brushSize, float brushForce)
{
    // If the player is trying to manipulate the terrain at this location
    m_displayChunk.ManipulateTerrain(wsCoord, elevate, int(brushSize), brushForce);
    RefitTerrainBVH();
}

void Game::RefitTerrainBVH()
{
    m_displayChunk.RefitBVH();
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
    m_highlightEffect->SetHighlightColour(HIGHLIGHT_COLOUR);

    m_blurPostProcess = std::make_unique<BasicPostProcess>(m_deviceResources->GetD3DDevice());

    // Load decal texture
    DX::ThrowIfFailed(
		CreateDDSTextureFromFile(device, L"database/data/brush_marker.dds", nullptr, m_brushDecalTextureSRV.ReleaseAndGetAddressOf())
    );

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

	// Create a 1x1 texture for selection box
	CD3D11_TEXTURE2D_DESC selBoxTexDesc(DXGI_FORMAT_R32G32B32A32_FLOAT, 1U, 1U, 1U, 0U, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE);

	D3D11_SUBRESOURCE_DATA initialData;
	initialData.pSysMem = HIGHLIGHT_COLOUR.f;
	initialData.SysMemPitch = sizeof(float) * 4;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> selectionTex;
	device->CreateTexture2D(&selBoxTexDesc, &initialData, selectionTex.ReleaseAndGetAddressOf());

	CD3D11_SHADER_RESOURCE_VIEW_DESC selSRVdesc(D3D11_SRV_DIMENSION_TEXTURE2D, selBoxTexDesc.Format);
	device->CreateShaderResourceView(selectionTex.Get(), &selSRVdesc, m_selectionBoxTexture.ReleaseAndGetAddressOf());

    // Create pixel shader for decaling
    auto decalPSBlob = DX::ReadData(L"decal_ps.cso");

    device->CreatePixelShader(decalPSBlob.data(), decalPSBlob.size(), nullptr, m_decalPixelShader.ReleaseAndGetAddressOf());

    m_decalMatrixBuffer.Create(device);
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

    // Copy of depth-stencil so that it can be sampled while also doing depth tests
    device->CreateTexture2D(&m_deviceResources->GetDepthStencilTextureDesc(), nullptr, m_depthStencilTexCopy.ReleaseAndGetAddressOf());
    device->CreateShaderResourceView(m_depthStencilTexCopy.Get(), &m_deviceResources->GetDepthStencilSRVDesc(), m_depthStencilSRVCopy.ReleaseAndGetAddressOf());
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
