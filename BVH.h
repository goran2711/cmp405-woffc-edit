#pragma once
#include <d3d11_1.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <VertexTypes.h>
// For visualising BVH
#include <GeometricPrimitive.h>

#include <vector>

class BVH
{
    struct Triangle
    {
        Triangle() = default;

        // - Stores three pointers to vertices/vertex positions
        //   - Means I don't have to do any extra stuff (like updating BVHs vertex positions)
        //     when refitting the structure--just recalculate the internal node's AABBs
        // NOTE: Poor design, really.
        Triangle(const DirectX::XMFLOAT3& v0, const DirectX::XMFLOAT3& v1, const DirectX::XMFLOAT3& v2)
        {
            v[0] = &v0;
            v[1] = &v1;
            v[2] = &v2;
        }

        const DirectX::XMFLOAT3* v[3];
    };

    struct BVHNode
    {
        // The internal node's bounding box
        DirectX::BoundingBox bounds;
        // If count == 0: the internal node's left child
        //     otherwise: index of the leaf node's first primitive (triangle)
        uint32_t leftFirst;
        // The number of primitives in this node (zero if internal node)
        uint32_t count;
    };

public:
    BVH() = default;

    template <size_t numVertices>
    BVH(const DirectX::VertexPositionNormalTexture(&vertices)[numVertices])
    {
        Initialise(vertices);
    }

    template <size_t numVertices>
    void Initialise(const DirectX::VertexPositionNormalTexture (&vertices)[numVertices])
    {
        // Assuming square terrain
        const int dimensions = (int) std::sqrt(float(numVertices));
        const int numQuads = numVertices - (dimensions * 2 - 1);
        const int numTriangles = numQuads * 2;

        // Initialise primitive (triangle) array
        m_primitives.reserve(numTriangles);
        for (int z = 0; z < dimensions - 1; ++z)
        {
            for (int x = 0; x < dimensions - 1; ++x)
            {
                const int index = (z * dimensions) + x;

                // I don't expect terrainGeometry to move around in memory
                // NOTE: Poor design
                const DirectX::XMFLOAT3& bottomLeft = vertices[index].position;
                const DirectX::XMFLOAT3& bottomRight = vertices[index + 1].position;
                const DirectX::XMFLOAT3& topRight = vertices[index + dimensions + 1].position;
                const DirectX::XMFLOAT3& topLeft = vertices[index + dimensions].position;

                m_primitives.emplace_back(bottomLeft, bottomRight, topRight);
                m_primitives.emplace_back(bottomLeft, topRight, topLeft);
            }
        }

        InitialiseNodes(m_primitives.size());
    }

    bool XM_CALLCONV Intersects(DirectX::FXMVECTOR origin, DirectX::FXMVECTOR direction, DirectX::XMVECTOR& hit) const;

    void Refit();

    void InitialiseDebugVisualiastion(ID3D11DeviceContext* context);
    void XM_CALLCONV DebugRender(ID3D11DeviceContext* context, DirectX::FXMMATRIX view, DirectX::CXMMATRIX projection, int depth);

private:
    void InitialiseNodes(size_t size);

    DirectX::BoundingBox CalculateBounds(int first, int count) const;

    void Subdivide(BVHNode& node, int depth = 0);
    void Partition(BVHNode& node);

    bool XM_CALLCONV Intersects(const BVHNode& node, DirectX::FXMVECTOR origin, DirectX::FXMVECTOR direction, float& dist) const;

    void Refit(BVHNode& node);

    void XM_CALLCONV DebugRender(BVHNode& node, ID3D11DeviceContext* context, DirectX::FXMMATRIX view, DirectX::CXMMATRIX projection, int currentDepth, int depth);

    // Node array
    BVHNode* m_root = nullptr;
    // NOTE: Node pool contains more nodes than are used--perhaps dynamically create new as it is being built?
    //       - Would necessitate changing BVHNode& parameters to indices instead, since references would be invalidated as nodes were added
    std::vector<BVHNode> m_pool;
    size_t m_poolPtr = 1;
    
    std::vector<Triangle> m_primitives;

    // Debug visualisation stuff
    std::unique_ptr<DirectX::GeometricPrimitive> m_box;
};

