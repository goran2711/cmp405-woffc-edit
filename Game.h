//
// Game.h
//

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "SceneObject.h"
#include "DisplayObject.h"
#include "DisplayChunk.h"
#include "ChunkObject.h"
#include "InputCommands.h"
#include "Camera.h"
#include <vector>
#include <map>

#include "HighlightEffect.h"
#include "PostProcess.h"

// TODO: Sampling the depth buffer should ignore objects
//       - If possible, DepthSampler could output depth to one channel, and stencil value in another--then I could check stencil values on the CPU


// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game : public DX::IDeviceNotify
{
    enum StencilMask
    {
        STENCIL_SELECTED_OBJECT = (1 << 0),
        STENCIL_TERRAIN         = (1 << 1),
        STENCIL_OBJECT          = (1 << 2)
    };

public:
	enum PickingMode
	{
		PICK_NORMAL,		// anything within selection box is selected
		PICK_INVERT,		// any unselected objects are selected, selected object are unselected (within the selection box)
		PICK_EXCLUSIVE		// any selected objects within the selection box are unselected
	};

	Game();
	~Game();

	// Initialization and management
	void Initialize(HWND window, int width, int height);
	void SetGridState(bool state);

	// Basic game loop
	void Tick(InputCommands * Input);
	void Render();

	// Rendering helpers
	void Clear();

	void SetSelectionIDs(const std::vector<int>& IDs) { m_selectionIDs = IDs; }

	// IDeviceNotify
	virtual void OnDeviceLost() override;
	virtual void OnDeviceRestored() override;

	// Messages
	void OnActivated();
	void OnDeactivated();
	void OnSuspending();
	void OnResuming();
	void OnWindowSizeChanged(int width, int height);

	//tool specific
	void BuildDisplayList(std::vector<SceneObject> * SceneGraph); //note vector passed by reference 
	void BuildDisplayChunk(ChunkObject *SceneChunk);
	void SaveDisplayChunk(ChunkObject *SceneChunk);	//saves geometry et al
	void ClearDisplayList();
    bool AddDisplayListItem(const SceneObject& sceneObject);
    void UpdateDisplayListItem(const SceneObject& sceneObject);
    bool RemoveDisplayListItem(int id);

	bool Pick(POINT cursorPos, RECT clientRect, int& id) const;
	bool PickWithinScreenRectangle(RECT selectionRect, std::vector<int>& selections, PickingMode invert = PICK_NORMAL) const;

    bool CursorIntersectsTerrain(long cursorX, long cursorY, DirectX::XMVECTOR& wsCoord);
    void ShowBrushDecal(bool val = true);
    void XM_CALLCONV SetBrushDecalPosition(DirectX::FXMVECTOR wsCoord, float brushSize);
    void XM_CALLCONV ManipulateTerrain(DirectX::FXMVECTOR wsCoord, bool elevate, float brushSize, float brushForce);

    void RefitTerrainBVH();

#ifdef DXTK_AUDIO
	void NewAudioDevice();
#endif

private:

	void Update(DX::StepTimer const& timer);

    void XM_CALLCONV RenderSceneGraph(ID3D11DeviceContext* context, FXMMATRIX view, CXMMATRIX projection);
    void XM_CALLCONV RenderSelectedObject(ID3D11DeviceContext* context, const DirectX::Model& model, FXMMATRIX local, CXMMATRIX view, CXMMATRIX projection);
    void PostProcess(ID3D11DeviceContext* context);

	void CreateDeviceDependentResources();
	void CreateWindowSizeDependentResources();

	void XM_CALLCONV DrawGrid(DirectX::FXMVECTOR xAxis, DirectX::FXMVECTOR yAxis, DirectX::FXMVECTOR origin, size_t xdivs, size_t ydivs, DirectX::GXMVECTOR color);

	// returns false if no bounding box could be created (outside of viewport)
	bool XM_CALLCONV CreateScreenSpaceBoundingBox(const DirectX::BoundingBox& modelSpaceBB, DirectX::FXMMATRIX local, DirectX::BoundingBox& screenSpaceBB) const;

    auto GetDisplayListIteratorFromID(int id);
    auto GetDisplayListIteratorFromID(int id) const;

	//// tool specific
	std::vector<DisplayObject>			m_displayList;
	DisplayChunk						m_displayChunk;
	InputCommands						inputCommands;
    
	// terrain manipulation brush
    bool m_showTerrainBrush = false;
    float m_brushSize = 0.f;

	__declspec(align(16))
		struct DecalMatrixBuffer
	{
		XMMATRIX invViewProjection;
		XMMATRIX worldToProjectorClip;
	} m_decalMatrices;

	ConstantBuffer<DecalMatrixBuffer> m_decalMatrixBuffer;
	XMFLOAT4X4 m_projectorView, m_projectorProjection;

	enum DSS_Type
	{
		DSS_WRITE_SELECTED_OBJECT,
		DSS_NOT_EQ_SELECTED_OBJECT,
		DSS_WRITE_TERRAIN,
		DSS_EQ_TERRAIN,
		DSS_COUNT
	};

    // DSSes for writing/testing terrain and selected objects to/in stencil buffer
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState>							m_dss[DSS_COUNT];

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_brushDecalTextureSRV;

	// Need copy of depth-stencil buffer to perform depth test whole sampling when rendering brush decal
    Microsoft::WRL::ComPtr<ID3D11Texture2D>                                 m_depthStencilTexCopy;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_depthStencilSRVCopy;

    Microsoft::WRL::ComPtr<ID3D11PixelShader>                               m_decalPixelShader;

    // Selection highlighting
	using InputLayouts = std::vector<Microsoft::WRL::ComPtr<ID3D11InputLayout>>;

	std::vector<int> m_selectionIDs;
	std::unique_ptr<HighlightEffect>									    m_highlightEffect;
	std::map<std::wstring, InputLayouts>									m_highlightEffectLayouts;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>						m_selectionBoxTexture;

    //// Render targets and post process stuff
	// Views for highlight render texture
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_highlightSRV;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>                          m_highlightRTV;

    // Intermediate render targers for post processing
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_rt1SRV;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>                          m_rt1RTV;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_rt2SRV;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>                          m_rt2RTV;

	// Effect for performing post-process
    std::unique_ptr<DirectX::BasicPostProcess>                              m_blurPostProcess;

	//control variables
	bool m_grid;							//grid rendering on / off
	// Device resources.
    std::shared_ptr<DX::DeviceResources>    m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                           m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>       m_gamePad;
    std::unique_ptr<DirectX::Keyboard>      m_keyboard;
    std::unique_ptr<DirectX::Mouse>         m_mouse;

    // DirectXTK objects.
    std::unique_ptr<DirectX::CommonStates>                                  m_states;
    std::unique_ptr<DirectX::EffectFactory>                                 m_fxFactory;
    std::unique_ptr<DirectX::GeometricPrimitive>                            m_shape;
    std::unique_ptr<DirectX::Model>                                         m_model;
    std::unique_ptr<DirectX::SpriteBatch>                                   m_sprites;
    std::unique_ptr<DirectX::SpriteFont>                                    m_font;
	// Used for drawing grid and pivot
    std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>  m_batch;
    std::unique_ptr<DirectX::BasicEffect>                                   m_batchEffect;

#ifdef DXTK_AUDIO
    std::unique_ptr<DirectX::AudioEngine>                                   m_audEngine;
    std::unique_ptr<DirectX::WaveBank>                                      m_waveBank;
    std::unique_ptr<DirectX::SoundEffect>                                   m_soundEffect;
    std::unique_ptr<DirectX::SoundEffectInstance>                           m_effect1;
    std::unique_ptr<DirectX::SoundEffectInstance>                           m_effect2;
#endif

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_texture1;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_texture2;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>                               m_batchInputLayout;

#ifdef DXTK_AUDIO
    uint32_t                                                                m_audioEvent;
    float                                                                   m_audioTimerAcc;

    bool                                                                    m_retryDefault;
#endif

    DirectX::SimpleMath::Matrix                                             m_world;
    DirectX::SimpleMath::Matrix                                             m_view;
    DirectX::SimpleMath::Matrix                                             m_projection;

	Camera m_camera;
};

std::wstring StringToWCHART(std::string s);