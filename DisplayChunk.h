#pragma once
#include "pch.h"
#include "DeviceResources.h"
#include "ChunkObject.h"

#include "BVH.h"

class DisplayChunk
{
public:
    static constexpr int TERRAINRESOLUTION = 128;
    static constexpr int NUM_VERTICES = TERRAINRESOLUTION * TERRAINRESOLUTION;
    static constexpr int NUM_QUADS = (TERRAINRESOLUTION * TERRAINRESOLUTION) - (TERRAINRESOLUTION * 2 - 1);
    static constexpr float TEXCOORD_STEP = 1.f / (TERRAINRESOLUTION - 1);

	void PopulateChunkData(ChunkObject * SceneChunk);
    void XM_CALLCONV RenderBatch(ID3D11DeviceContext* context, FXMMATRIX view, CXMMATRIX projection);
    void InitialiseRendering(DX::DeviceResources* deviceResources);
    void InitialiseBatch();	//initial setup, base coordinates etc based on scale
    void LoadHeightMap(ID3D11Device* device);
    void SaveHeightMap();			//saves the heigtmap back to file.
	void UpdateTerrain();			//updates the geometry based on the heigtmap
	void GenerateHeightmap();		//creates or alters the heightmap

    void XM_CALLCONV ManipulateTerrain(DirectX::FXMVECTOR pos, int brushSize, bool elevate);

    void RefitBVH();

	std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionNormalTexture>>  m_batch;
	std::unique_ptr<DirectX::BasicEffect>       m_terrainEffect;

	ID3D11ShaderResourceView *					m_texture_diffuse;				//diffuse texture
	Microsoft::WRL::ComPtr<ID3D11InputLayout>   m_terrainInputLayout;

    bool XM_CALLCONV CursorIntersectsTerrain(long mouseX, long mouseY, const DirectX::SimpleMath::Viewport& viewport, DirectX::FXMMATRIX projection, DirectX::CXMMATRIX view, DirectX::CXMMATRIX world, DirectX::XMVECTOR& wsCoord) const;

private:
    void CalculateTerrainNormals();
	
    std::vector<uint16_t> m_indices;
    DirectX::VertexPositionNormalTexture m_terrainGeometry[NUM_VERTICES];
    BYTE m_heightMap[NUM_VERTICES];

    BVH m_bvh;

    float	m_terrainHeightScale = 0.25f;	//convert our 0-256 terrain to 64
    int		m_terrainSize = 512;				//size of terrain in metres
    float   m_terrainPositionScalingFactor = m_terrainSize / (float) (TERRAINRESOLUTION - 1);	//factor we multiply the position by to convert it from its native resolution( 0- Terrain Resolution) to full scale size in metres dictated by m_Terrainsize

	std::string m_name;
	int m_chunk_x_size_metres;
	int m_chunk_y_size_metres;
	int m_chunk_base_resolution;
	std::string m_heightmap_path;
	std::string m_tex_diffuse_path;
	std::string m_tex_splat_alpha_path;
	std::string m_tex_splat_1_path;
	std::string m_tex_splat_2_path;
	std::string m_tex_splat_3_path;
	std::string m_tex_splat_4_path;
	bool m_render_wireframe;
	bool m_render_normals;
	int m_tex_diffuse_tiling;
	int m_tex_splat_1_tiling;
	int m_tex_splat_2_tiling;
	int m_tex_splat_3_tiling;
	int m_tex_splat_4_tiling;

};

