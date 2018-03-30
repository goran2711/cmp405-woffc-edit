#include <string>
#include "DisplayChunk.h"
#include "Game.h"


using namespace DirectX;
using namespace SimpleMath;

DisplayChunk::DisplayChunk()
{
    //terrain size in meters. note that this is hard coded here, we COULD get it from the terrain chunk along with the other info from the tool if we want to be more flexible.
    m_terrainSize = 512;
    m_terrainHeightScale = 0.25;  //convert our 0-256 terrain to 64
    m_textureCoordStep = 1.0 / (TERRAINRESOLUTION - 1);	//-1 becuase its split into chunks. not triangles.  we want tthe last one in each row to have tex coord 1
    m_terrainPositionScalingFactor = m_terrainSize / (TERRAINRESOLUTION - 1);
}


DisplayChunk::~DisplayChunk()
{
}

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

void DisplayChunk::RenderBatch(std::shared_ptr<DX::DeviceResources>  DevResources)
{
    auto context = DevResources->GetD3DDeviceContext();

    m_terrainEffect->Apply(context);
    context->IASetInputLayout(m_terrainInputLayout.Get());

    m_batch->Begin();
    for (size_t i = 0; i < TERRAINRESOLUTION - 1; i++)	//looping through QUADS.  so we subtrack one from the terrain array or it will try to draw a quad starting with the last vertex in each row. Which wont work
    {
        for (size_t j = 0; j < TERRAINRESOLUTION - 1; j++)//same as above
        {
            m_batch->DrawQuad(m_terrainGeometry[i][j], m_terrainGeometry[i][j + 1], m_terrainGeometry[i + 1][j + 1], m_terrainGeometry[i + 1][j]); //bottom left bottom right, top right top left.
        }
    }
    m_batch->End();
}

void DisplayChunk::InitialiseBatch()
{
    //build geometry for our terrain array
    //iterate through all the triangles of our required resolution terrain.
    int index = 0;

    for (size_t i = 0; i < TERRAINRESOLUTION; i++)
    {
        for (size_t j = 0; j < TERRAINRESOLUTION; j++)
        {
            index = (TERRAINRESOLUTION * i) + j;
            m_terrainGeometry[i][j].position = Vector3(j*m_terrainPositionScalingFactor - (0.5*m_terrainSize), (float) (m_heightMap[index])*m_terrainHeightScale, i*m_terrainPositionScalingFactor - (0.5*m_terrainSize));	//This will create a terrain going from -64->64.  rather than 0->128.  So the center of the terrain is on the origin
            m_terrainGeometry[i][j].normal = Vector3(0.0f, 1.0f, 0.0f);						//standard y =up
            m_terrainGeometry[i][j].textureCoordinate = Vector2(((float) m_textureCoordStep*j)*m_tex_diffuse_tiling, ((float) m_textureCoordStep*i)*m_tex_diffuse_tiling);				//Spread tex coords so that its distributed evenly across the terrain from 0-1

        }
    }

    // initialise bvh
    //index = (TERRAINRESOLUTION * (TERRAINRESOLUTION - 1)) + (TERRAINRESOLUTION - 1);
    //Vector3 max((TERRAINRESOLUTION - 1) * m_terrainPositionScalingFactor - (0.5f * m_terrainSize),
    //            m_terrainHeightScale,
    //            (TERRAINRESOLUTION - 1) * m_terrainPositionScalingFactor - (0.5f * m_terrainSize));

    //Vector3 min((0) * m_terrainPositionScalingFactor - (0.5f * m_terrainSize),
    //            0,
    //            (0) * m_terrainPositionScalingFactor - (0.5f * m_terrainSize));

    //BoundingBox rootBB;
    //BoundingBox::CreateFromPoints(rootBB, min, max);

    m_bvh.Initialise(m_terrainGeometry);
    //m_bvh.Initialise(rootBB);

    //for (size_t i = 0; i < TERRAINRESOLUTION - 1; i++)	//looping through QUADS.  so we subtrack one from the terrain array or it will try to draw a quad starting with the last vertex in each row. Which wont work
    //{
    //    for (size_t j = 0; j < TERRAINRESOLUTION - 1; j++)//same as above
    //    {
    //        Vector3 bottomLeft = m_terrainGeometry[i][j].position;
    //        Vector3 bottomRight = m_terrainGeometry[i][j + 1].position;
    //        Vector3 topRight = m_terrainGeometry[i + 1][j + 1].position;
    //        Vector3 topLeft = m_terrainGeometry[i + 1][j].position;

    //        m_bvh.Insert({ bottomLeft, bottomRight, topRight });
    //        m_bvh.Insert({ bottomLeft, topRight, topLeft });
    //    }
    //}

    //m_bvh.Build();

    CalculateTerrainNormals();
}

void DisplayChunk::LoadHeightMap(std::shared_ptr<DX::DeviceResources>  DevResources)
{
    auto device = DevResources->GetD3DDevice();
    auto devicecontext = DevResources->GetD3DDeviceContext();

    //load in heightmap .raw
    FILE *pFile = NULL;

    // Open The File In Read / Binary Mode.

    pFile = fopen(m_heightmap_path.c_str(), "rb");
    // Check To See If We Found The File And Could Open It
    if (pFile == NULL)
    {
        // Display Error Message And Stop The Function
        MessageBox(NULL, L"Can't Find The Height Map!", L"Error", MB_OK);
        return;
    }

    // Here We Load The .RAW File Into Our pHeightMap Triangle Array
    // We Are Only Reading In '1', And The Size Is (Width * Height)
    fread(m_heightMap, 1, TERRAINRESOLUTION*TERRAINRESOLUTION, pFile);

    fclose(pFile);

    //load in texture diffuse

    //load the diffuse texture
    std::wstring texturewstr = StringToWCHART(m_tex_diffuse_path);
    HRESULT rs;
    rs = CreateDDSTextureFromFile(device, texturewstr.c_str(), NULL, &m_texture_diffuse);	//load tex into Shader resource	view and resource

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

    m_batch = std::make_unique<PrimitiveBatch<VertexPositionNormalTexture>>(devicecontext);
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

    fwrite(m_heightMap, sizeof(BYTE), TERRAINRESOLUTION*TERRAINRESOLUTION, pFile);
    fclose(pFile);

}

void DisplayChunk::UpdateTerrain()
{
    //all this is doing is transferring the height from the heigtmap into the terrain geometry.
    int index;
    for (size_t i = 0; i < TERRAINRESOLUTION; i++)
    {
        for (size_t j = 0; j < TERRAINRESOLUTION; j++)
        {
            index = (TERRAINRESOLUTION * i) + j;
            m_terrainGeometry[i][j].position.y = (float) (m_heightMap[index])*m_terrainHeightScale;
        }
    }
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
    static const float MAGNITUDE = 1.f;
    
    // Transform to grid coordinates
    int x = (XMVectorGetX(clickPos) + (0.5f * m_terrainSize)) / m_terrainPositionScalingFactor;
    int z = (XMVectorGetZ(clickPos) + (0.5f * m_terrainSize)) / m_terrainPositionScalingFactor;

    const int brushSizeH = (brushSize / 2) / m_terrainPositionScalingFactor;


    // FIX: Not sure why I have to add these -1 and +2 offsets here to get a satisfying result... (and even then it's not perfect)
    int minX = std::max(0, x - brushSizeH - 1);
    int minZ = std::max(0, z - brushSizeH - 1);

    int maxX = std::min(TERRAINRESOLUTION - 1, x + brushSizeH + 2);
    int maxZ = std::min(TERRAINRESOLUTION - 1, z + brushSizeH + 2);

    // Distance from what we will use as the center vertex, to the clicked position
    float centerDistance = Vector3::DistanceSquared(m_terrainGeometry[z][x].position, clickPos);

    // Move vertices
    for (int i = minZ; i < maxZ; ++i)
    {
        for (int j = minX; j < maxX; ++j)
        {
            const float currentHeight = m_terrainGeometry[i][j].position.y;

            // Calculate the weight of the displacement for the current vertex based on its distance from the point the user clicked
            float weight = std::min(1.f, centerDistance / Vector3::DistanceSquared(m_terrainGeometry[i][j].position, clickPos)) * MAGNITUDE;

            float newHeight = 0.f;
            if (elevate)
                newHeight = std::min(currentHeight + weight, 255.f * m_terrainHeightScale);
            else
                newHeight = std::max(currentHeight - weight, 0.f);

            m_terrainGeometry[i][j].position.y = newHeight;
        }
    }

    CalculateTerrainNormals();
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
    int index1, index2, index3, index4;
    Vector3 upDownVector, leftRightVector, normalVector;

    for (int i = 0; i < (TERRAINRESOLUTION - 1); i++)
    {
        for (int j = 0; j < (TERRAINRESOLUTION - 1); j++)
        {
            upDownVector.x = (m_terrainGeometry[i + 1][j].position.x - m_terrainGeometry[i - 1][j].position.x);
            upDownVector.y = (m_terrainGeometry[i + 1][j].position.y - m_terrainGeometry[i - 1][j].position.y);
            upDownVector.z = (m_terrainGeometry[i + 1][j].position.z - m_terrainGeometry[i - 1][j].position.z);

            leftRightVector.x = (m_terrainGeometry[i][j - 1].position.x - m_terrainGeometry[i][j + 1].position.x);
            leftRightVector.y = (m_terrainGeometry[i][j - 1].position.y - m_terrainGeometry[i][j + 1].position.y);
            leftRightVector.z = (m_terrainGeometry[i][j - 1].position.z - m_terrainGeometry[i][j + 1].position.z);


            leftRightVector.Cross(upDownVector, normalVector);	//get cross product
            normalVector.Normalize();			//normalise it.

            m_terrainGeometry[i][j].normal = normalVector;	//set the normal for this point based on our result
        }
    }
}
