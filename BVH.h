#pragma once
#include <d3d11_1.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <VertexTypes.h>
#include <SimpleMath.h>

#include <vector>
#include <numeric>

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

    // NOTE: with terrain, vertices will be a 2d array... sooo boo
    template <size_t rows, size_t columns>
    BVH(const VertexPositionNormalTexture (&vertices)[rows][columns])
    {
        Initialise(vertices);
    }

    template <size_t rows, size_t columns>
    void Initialise(const VertexPositionNormalTexture(&vertices)[rows][columns])
    {
        // Initialise primitive (triangle) array
        m_primitives.reserve(((rows - 1) * (columns - 1)) * 2);
        for (int z = 0; z < rows - 1; ++z)
        {
            for (int x = 0; x < columns - 1; ++x)
            {
                // .position is just a public variable, and I don't expect terrainGeometry to move around in memory
                // NOTE: Still bad tho, probably (definitely)
                const XMFLOAT3& bottomLeft = vertices[z][x].position;
                const XMFLOAT3& bottomRight = vertices[z][x + 1].position;
                const XMFLOAT3& topRight = vertices[z + 1][x + 1].position;
                const XMFLOAT3& topLeft = vertices[z + 1][x].position;

                m_primitives.emplace_back(bottomLeft, bottomRight, topRight);
                m_primitives.emplace_back(bottomLeft, topRight, topLeft);
            }
        }

        //// TODO: Could move functionality that does not rely on the 2D "vertices" array into the .cpp file, so I don't
        //         have to include headers such as <numeric> in the .h
        // Initialise index array
        m_indices.resize(m_primitives.size());
        std::iota(m_indices.begin(), m_indices.end(), 0);

        // Initialise node pool and root
        m_pool.resize(m_primitives.size() * 2 - 1);
        m_root = &m_pool[0];

        // Root starts as a leaf with all the primitives within it
        m_root->leftFirst = 0;
        m_root->count = m_primitives.size();

        m_root->bounds = CalculateBounds(m_root->leftFirst, m_root->count);

        // Subdivide root
        Subdivide(*m_root);
    }

    bool Intersects(const SimpleMath::Ray& ray, SimpleMath::Vector3& hit) const;

private:
    BoundingBox CalculateBounds(int first, int count) const;

    void Subdivide(BVHNode& node, int depth = 0);
    void Partition(BVHNode& node);

    using ChildCounts = std::tuple<uint32_t, uint32_t>;
    ChildCounts XM_CALLCONV Split(uint32_t first, uint32_t last, FXMVECTOR splitPos, uint8_t splitAxis, std::vector<int>& sortedIndices);

    bool Intersects(BVHNode& node, const SimpleMath::Ray& ray, float& dist) const;

    // Node array
    BVHNode* m_root = nullptr;
    // NOTE: Node pool contains more nodes than are used--perhaps dynamically create new as it is being built?
    //       - Would necessitate changing BVHNode& parameters to indices instead, since references would be invalidated
    // TODO: Figure out why I cannot access vector elements when function is marked const (don't stick with mutable)
    mutable std::vector<BVHNode> m_pool;
    size_t m_poolPtr = 1;
    
    std::vector<Triangle> m_primitives;
    std::vector<int> m_indices;
};

