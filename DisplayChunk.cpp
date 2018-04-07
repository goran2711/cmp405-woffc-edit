#include <string>
#include "DisplayChunk.h"
#include "Game.h"
#include <locale>
#include <codecvt>


using namespace DirectX;
using namespace DirectX::SimpleMath;


void DisplayChunk::PopulateChunkData(ChunkObject * SceneChunk)
{
    m_name = SceneChunk->name;
    m_chunk_x_size_metres = SceneChunk->chunk_x_size_metres;
    m_chunk_y_size_metres = SceneChunk->chunk_y_size_metres;
    m_chunk_base_resolution = SceneChunk->chunk_base_resolution;
    m_heightmap_path = SceneChunk->heightmap_path;
    m_tex_diffuse_path = SceneChunk->tex_diffuse_path;
    m_tex_splat_alpha_path = SceneChunk->tex_splat_alpha_path;
    m_tex_splat_1_path = SceneChunk->tex_splat_1_path;
    m_tex_splat_2_path = SceneChunk->tex_splat_2_path;
    m_tex_splat_3_path = SceneChunk->tex_splat_3_path;
    m_tex_splat_4_path = SceneChunk->tex_splat_4_path;
    m_render_wireframe = SceneChunk->render_wireframe;
    m_render_normals = SceneChunk->render_normals;
    m_tex_diffuse_tiling = SceneChunk->tex_diffuse_tiling;
    m_tex_splat_1_tiling = SceneChunk->tex_splat_1_tiling;
    m_tex_splat_2_tiling = SceneChunk->tex_splat_2_tiling;
    m_tex_splat_3_tiling = SceneChunk->tex_splat_3_tiling;
    m_tex_splat_4_tiling = SceneChunk->tex_splat_4_tiling;
}

void XM_CALLCONV DisplayChunk::RenderBatch(ID3D11DeviceContext* context, FXMMATRIX view, CXMMATRIX projection)
{
    m_terrainEffect->Apply(context);
    context->IASetInputLayout(m_terrainInputLayout.Get());

    m_batch->Begin();
    m_batch->DrawIndexed(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, m_indices.data(), m_indices.size(), m_terrainGeometry, NUM_VERTICES);
    m_batch->End();

    //m_bvh.DebugRender(context, view, projection, 3);
}

void DisplayChunk::InitialiseBatch()
{
    //build geometry for our terrain array
    //iterate through all the vertices of our required resolution terrain.
    const float terrainSizeH = m_terrainSize * 0.5f;

    for (size_t z = 0; z < TERRAINRESOLUTION; z++)
    {
        for (size_t x = 0; x < TERRAINRESOLUTION; x++)
        {
            int index = (TERRAINRESOLUTION * z) + x;

            //This will create a terrain going from -64->64.  rather than 0->128.  So the center of the terrain is on the origin
            m_terrainGeometry[index].position = {
                (x * m_terrainPositionScalingFactor) - terrainSizeH,
                float(m_heightMap[index]) * m_terrainHeightScale,
                (z * m_terrainPositionScalingFactor) - terrainSizeH
            };

            m_terrainGeometry[index].normal = Vector3::UnitY;
            //Spread tex coords so that its distributed evenly across the terrain from 0-1
            m_terrainGeometry[index].textureCoordinate = { x * TEXCOORD_STEP * m_tex_diffuse_tiling, z * TEXCOORD_STEP * m_tex_diffuse_tiling };
        }
    }

    // Initialise indices
    for (size_t z = 0; z < TERRAINRESOLUTION - 1; z++)
    {
        for (size_t x = 0; x < TERRAINRESOLUTION - 1; x++)
        {
            int bottomL = (TERRAINRESOLUTION * z) + x;
            int bottomR = (TERRAINRESOLUTION * z) + x + 1;
            int topR = (TERRAINRESOLUTION * (z + 1)) + x + 1;
            int topL = (TERRAINRESOLUTION * (z + 1)) + x;

            // First triangle
            m_indices.push_back(bottomL);
            m_indices.push_back(bottomR);
            m_indices.push_back(topR);

            // Second triangle
            m_indices.push_back(bottomL);
            m_indices.push_back(topR);
            m_indices.push_back(topL);
        }
    }

    CalculateTerrainNormals();

    // initialise bvh
    m_bvh.Initialise(m_terrainGeometry);
}

void DisplayChunk::InitialiseRendering(DX::DeviceResources* deviceResources)
{
    ID3D11Device* device = deviceResources->GetD3DDevice();
    ID3D11DeviceContext* context = deviceResources->GetD3DDeviceContext();

    //setup terrain effect
    m_terrainEffect = std::make_unique<BasicEffect>(device);
    m_terrainEffect->EnableDefaultLighting();
    m_terrainEffect->SetLightingEnabled(true);
    m_terrainEffect->SetTextureEnabled(true);
    m_terrainEffect->SetTexture(m_texture_diffuse);

    void const* shaderByteCode;
    size_t byteCodeLength;

    m_terrainEffect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

    //setup batch
    DX::ThrowIfFailed(
        device->CreateInputLayout(VertexPositionNormalTexture::InputElements,
                                  VertexPositionNormalTexture::InputElementCount,
                                  shaderByteCode,
                                  byteCodeLength,
                                  m_terrainInputLayout.ReleaseAndGetAddressOf())
    );

    m_batch = std::make_unique<PrimitiveBatch<VertexPositionNormalTexture>>(context, m_indices.size() + 1, NUM_VERTICES + 1);

    m_bvh.InitialiseDebugVisualiastion(context);
}

void DisplayChunk::LoadHeightMap(ID3D11Device* device)
{
    //load in heightmap .raw
    FILE *pFile = NULL;

    // Open The File In Read / Binary Mode.
    errno_t ret = fopen_s(&pFile, m_heightmap_path.c_str(), "rb");
    // Check To See If We Found The File And Could Open It
    if (ret != 0 || pFile == NULL)
    {
        // Display Error Message And Stop The Function
        MessageBox(NULL, L"Can't Find The Height Map!", L"Error", MB_OK);
        return;
    }

    // Here We Load The .RAW File Into Our pHeightMap Data Array
    // We Are Only Reading In '1', And The Size Is (Width * Height)
    fread(m_heightMap, 1, NUM_VERTICES, pFile);

    fclose(pFile);

    //load the diffuse texture
    std::wstring_convert<std::codecvt_utf8<wchar_t>> convertToWide;
    std::wstring texturewstr = convertToWide.from_bytes(m_tex_diffuse_path);
    HRESULT rs = CreateDDSTextureFromFile(device, texturewstr.c_str(), NULL, &m_texture_diffuse);	//load tex into Shader resource	view and resource
}

void DisplayChunk::SaveHeightMap()
{
    FILE *pFile = NULL;

    // Open The File In Read / Binary Mode.
    pFile = fopen(m_heightmap_path.c_str(), "wb+");;
    // Check To See If We Found The File And Could Open It
    if (pFile == NULL)
    {
        // Display Error Message And Stop The Function
        MessageBox(NULL, L"Can't Find The Height Map!", L"Error", MB_OK);
        return;
    }

    fwrite(m_heightMap, sizeof(BYTE), NUM_VERTICES, pFile);
    fclose(pFile);

}

void DisplayChunk::UpdateTerrain()
{
    //all this is doing is transferring the height from the heigtmap into the terrain geometry.
    for (size_t i = 0; i < NUM_VERTICES; ++i)
        m_terrainGeometry[i].position.y = float(m_heightMap[i]) * m_terrainHeightScale;

    CalculateTerrainNormals();
}

void DisplayChunk::GenerateHeightmap()
{
    //insert how YOU want to update the heigtmap here! :D
}

void XM_CALLCONV DisplayChunk::ManipulateTerrain(FXMVECTOR clickPos, int brushSize, bool elevate)
{
    // TODO: Some check to ensure it is within bounds (well.. cursor-terrain test will never be true outside the terrain, sooo...)

    // Controls the speed at which vertices are displaced
    static const float MAGNITUDE = 3.f;
    
    // Transform to grid coordinates
    int hitX = (XMVectorGetX(clickPos) + (0.5f * m_terrainSize)) / m_terrainPositionScalingFactor;
    int hitZ = (XMVectorGetZ(clickPos) + (0.5f * m_terrainSize)) / m_terrainPositionScalingFactor;

    const int brushSizeH = (brushSize / 2) / m_terrainPositionScalingFactor;

    // FIX: Not sure why I have to add these -1 and +2 offsets here to get a satisfying result... (and even then it's not perfect)
    int minX = std::max(0, hitX - brushSizeH - 1);
    int minZ = std::max(0, hitZ - brushSizeH - 1);

    int maxX = std::min(TERRAINRESOLUTION - 1, hitX + brushSizeH + 2);
    int maxZ = std::min(TERRAINRESOLUTION - 1, hitZ + brushSizeH + 2);

    XMVECTOR pos = clickPos;
    pos = XMVectorSetY(pos, 0.f);

    // Distance from what we will use as the center vertex, to the clicked position
    int hitIdx = (hitZ * TERRAINRESOLUTION) + hitX;
    Vector3 nearestPos = m_terrainGeometry[hitIdx].position;
    nearestPos.y = 0.f;

    float centerDistance = Vector3::Distance(nearestPos, pos);

    // Move vertices
    for (int z = minZ; z < maxZ; ++z)
    {
        for (int x = minX; x < maxX; ++x)
        {
            const int index = (z * TERRAINRESOLUTION) + x;
            const float currentHeight = m_terrainGeometry[index].position.y;

            // Calculate the weight of the displacement for the current vertex based on its distance from the point the user clicked
            // FIX: Need a better way to calculate weight--right now, this method acts weird without the std::min, and std::min is just a band-aid,
            //      it's still not perfect
            //      - Could the problem be that I'm selecting the nearest vertex in grid space? Grid space distance ignores height
            Vector3 p = m_terrainGeometry[index].position;
            p.y = 0.f;

            float weight = std::min(1.f, centerDistance / Vector3::Distance(p, pos));

            float displacement = weight * MAGNITUDE;

            float newHeight = 0.f;
            if (elevate)
                newHeight = std::min(currentHeight + displacement, 255.f * m_terrainHeightScale);
            else
                newHeight = std::max(currentHeight - displacement, 0.f);

            m_terrainGeometry[index].position.y = newHeight;
        }
    }

    CalculateTerrainNormals();
}

void DisplayChunk::RefitBVH()
{
    m_bvh.Refit();
}

bool XM_CALLCONV DisplayChunk::CursorIntersectsTerrain(long mouseX, long mouseY, const SimpleMath::Viewport & viewport, FXMMATRIX projection, CXMMATRIX view, CXMMATRIX world, XMVECTOR& wsCoord) const
{
    // Create ray
    Vector3 nearPoint(mouseX, mouseY, 0.f);
    Vector3 farPoint(mouseX, mouseY, 1.f);
    Vector3 nearPointUnprojected = viewport.Unproject(nearPoint, projection, view, world);
    Vector3 farPointUnprojected = viewport.Unproject(farPoint, projection, view, world);

    Vector3 direction(farPointUnprojected - nearPointUnprojected);
    direction.Normalize();

    Ray ray(nearPointUnprojected, direction);

    // Query BVH
    Vector3 hit;
    if (m_bvh.Intersects(ray, hit))
    {
        wsCoord = hit;
        return true;
    }

    wsCoord = XMVectorZero();
    return false;
}

void DisplayChunk::CalculateTerrainNormals()
{
    // Lambda for testing if two indices are on the same terrain row
    static const auto OnSameRow = [](int i0, int i1)
    {
        return (i0 / TERRAINRESOLUTION) == (i1 / TERRAINRESOLUTION);
    };

    for (int i = 0; i < NUM_QUADS; i++)
    {
        // Neighbour index calculation and range checks
        int upIdx = (i + TERRAINRESOLUTION >= (TERRAINRESOLUTION * TERRAINRESOLUTION) ? i : i + TERRAINRESOLUTION);
        int downIdx = (i - TERRAINRESOLUTION < 0 ? i : i - TERRAINRESOLUTION);

        int leftIdx = (OnSameRow(i, i - 1) ? i - 1 : i);
        int rightIdx = (OnSameRow(i, i + 1) ? i + 1 : i);

        // Normal calculation
        Vector3 upDownVector(
            (m_terrainGeometry[upIdx].position.x - m_terrainGeometry[downIdx].position.x),
            (m_terrainGeometry[upIdx].position.y - m_terrainGeometry[downIdx].position.y),
            (m_terrainGeometry[upIdx].position.z - m_terrainGeometry[downIdx].position.z)
        );

        Vector3 leftRightVector(
            (m_terrainGeometry[leftIdx].position.x - m_terrainGeometry[rightIdx].position.x),
            (m_terrainGeometry[leftIdx].position.y - m_terrainGeometry[rightIdx].position.y),
            (m_terrainGeometry[leftIdx].position.z - m_terrainGeometry[rightIdx].position.z)
        );

        Vector3 normalVector = leftRightVector.Cross(upDownVector);
        normalVector.Normalize();

        m_terrainGeometry[i].normal = normalVector;
    }
}
