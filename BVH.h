#pragma once
#include <d3d11_1.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <VertexTypes.h>
#include <SimpleMath.h>

#include <vector>

// Debug stuff
#include <GeometricPrimitive.h>

using namespace DirectX;

//// NOTES
// - Don't need to implement BVH on top of an Octree--traversal can seemingly be made much easier by using
//    something more akin to an adaptive binary tree (or kd-tree? not sure what slides use)
// - Storing "primitives" (data that will be contained in leaf nodes) and the BVN nodes themselves in arrays
//    means nodes can store one index, which depending on how many primitives they contain (internal nodes have none)
//    might be an index into the primitive array or the node array
//    - In the slides, the indices contained within a node (if it is a leaf) index into a "primitive indices" array,
//      which contains indices that point into the primitives array. The primitive indices array is the one that is sorted/shuffled
//      during partitioning
//    - Not sure if I can get away with just partitioning the primitive array directly or not (since in my case, the BVH will be the owner
//      and sole user of the primitive array)
//      - In the slides, the primitive array is being passed to BVH::ConstructBVH, but never stored--presumably it is also passed when querying
//        the structure. Keep in mind it may just be pseudo-code, so some stuff could be left out
// - The point of this compacting of nodes is to make them more cache friendly
// - CONSTRUCT: Creates node array and initialises root node before subdividing it
//              - The slides create an primitive index array of size N, where N is the number of primitives (presumedly)
//              - The node pool is set to be of size N * 2 - 1, is that perhaps the maximum size of the tree so now every one is used?
//                - If so, can I get around this using std::vector? (since I'm not messing with pointers anymore)
//                  - I may want to just stick with allocating all space up front if I will be creating/removing nodes at run-time
// - SUBDIVIDE: Creates the nodes children, where the split plane is determined through some heuristic
// - PARTITION: Sorts a node's children so that their spatial coherence is encoded into their position in the index array
//              - Probably sort them along the split axis
// - REFIT: Recomputes the bounding boxes of the internal nodes--can be done efficiently by traversing the node pool in reverse,
//          since a node's parents will always be to the left of them in the array

class BVH
{
    //// STEPS
    // 1. Calculate primitive AABBs
    //    - If I do ray-triangle intersection tests instead of ray-AABB tests, I could skip this, I reckon
    //      - Well, internal nodes still need AABBs...
    // 2. Determine split axis and position
    //    - What does "... and position" mean again?
    //    - The article on ABT had a lot of good information on splitting
    // 3. Partition
    //    - Can be done in-place, much like quick sort (split plane is equivalent to pivot)

    // My "primitive" equivalent
    struct Triangle
    {
        Triangle() = default;

        // - Stores three pointers to vertices/vertex positions
        //   - Means I don't have to do any extra stuff (like updating BVHs vertex positions)
        //     when refitting the structure--just recalculate the internal node's AABBs
        Triangle(const XMFLOAT3& v0, const XMFLOAT3& v1, const XMFLOAT3& v2)
        {
            v[0] = &v0;
            v[1] = &v1;
            v[2] = &v2;
        }

        const XMFLOAT3* v[3];
    };

    struct BVHNode
    {
        // The internal node's bounding box
        BoundingBox bounds;
        // If count == 0: the internal node's left child
        //     otherwise: index of the leaf node's first primitive (triangle)
        uint32_t leftFirst;
        // The number of primitives in this node (zero if internal node)
        uint32_t count;
    };

public:
    BVH() = default;

    template <size_t numVertices>
    BVH(const VertexPositionNormalTexture(&vertices)[numVertices])
    {
        Initialise(vertices);
    }

    template <size_t numVertices>
    void Initialise(const VertexPositionNormalTexture (&vertices)[numVertices])
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

                // .position is just a public variable, and I don't expect terrainGeometry to move around in memory
                // NOTE: Still bad tho, probably (definitely)
                const XMFLOAT3& bottomLeft = vertices[index].position;
                const XMFLOAT3& bottomRight = vertices[index + 1].position;
                const XMFLOAT3& topRight = vertices[index + dimensions + 1].position;
                const XMFLOAT3& topLeft = vertices[index + dimensions].position;

                m_primitives.emplace_back(bottomLeft, bottomRight, topRight);
                m_primitives.emplace_back(bottomLeft, topRight, topLeft);
            }
        }

        InitialiseNodes(m_primitives.size());
    }

    bool XM_CALLCONV Intersects(DirectX::FXMVECTOR origin, DirectX::FXMVECTOR direction, DirectX::XMVECTOR& hit) const;

    void Refit();

    void InitialiseDebugVisualiastion(ID3D11DeviceContext* context);
    void XM_CALLCONV DebugRender(ID3D11DeviceContext* context, FXMMATRIX view, CXMMATRIX projection, int depth);

private:
    void InitialiseNodes(size_t size);

    BoundingBox CalculateBounds(int first, int count) const;

    void Subdivide(BVHNode& node, int depth = 0);
    void Partition(BVHNode& node);

    bool XM_CALLCONV Intersects(const BVHNode& node, DirectX::FXMVECTOR origin, DirectX::FXMVECTOR direction, float& dist) const;

    void Refit(BVHNode& node);

    void XM_CALLCONV DebugRender(BVHNode& node, ID3D11DeviceContext* context, FXMMATRIX view, CXMMATRIX projection, int currentDepth, int depth);

    // Node array
    BVHNode* m_root = nullptr;
    // NOTE: Node pool contains more nodes than are used--perhaps dynamically create new as it is being built?
    //       - Would necessitate changing BVHNode& parameters to indices instead, since references would be invalidated
    std::vector<BVHNode> m_pool;
    size_t m_poolPtr = 1;
    
    std::vector<Triangle> m_primitives;

    // Debug visualisation stuff
    std::unique_ptr<GeometricPrimitive> m_box;
};

