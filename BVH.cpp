#include "BVH.h"
#include <numeric>
#include <tuple>

// Macros are the worst...
#ifdef max
#undef max
#endif

BoundingBox BVH::CalculateBounds(int first, int count) const
{
    assert(first < m_indices.size());

    BoundingBox bounds;

    XMVECTOR min = XMVectorSplatOne() * std::numeric_limits<float>::max();
    XMVECTOR max = -min;

    // Initialise bounding box
    {
        const Triangle& triangle = m_primitives[ m_indices[first] ];

        for (int j = 0; j < 3; ++j)
        {
            XMVECTOR vertex = XMLoadFloat3(triangle.v[j]);

            min = XMVectorMin(min, vertex);
            max = XMVectorMax(max, vertex);
        }

        BoundingBox::CreateFromPoints(bounds, min, max);
    }

    // Expand bounding box
    for (int i = first + 1; i < first + count; ++i)
    {
        const Triangle& triangle = m_primitives[ m_indices[i] ];

        min = XMVectorSplatOne() * std::numeric_limits<float>::max();
        max = -min;

        for (int j = 0; j < 3; ++j)
        {
            XMVECTOR vertex = XMLoadFloat3(triangle.v[j]);
            
            min = XMVectorMin(min, vertex);
            max = XMVectorMax(max, vertex);
        }

        BoundingBox newBounds;
        BoundingBox::CreateFromPoints(newBounds, min, max);

        BoundingBox::CreateMerged(bounds, bounds, newBounds);
    }

    return bounds;
}

void BVH::Subdivide(BVHNode& node)
{
    // NOTE: Exit condition: stop subdividing if this new leaf has a minimum number of primitives
    //       (can also add if we reached a certain depth)
    if (node.count < 2) // Allow minimum 1 primitive for each child
        return;

    // TODO: Partition node
    //       - Pay careful attention to node's leftFirst and count--they need to be set appropriately (to poolPtr and 0) when initialising children (CalculateBounds, etc.)
    //         and subdividing them, but at the same time they need to be correct when sorting children
    // NOTE: Partition turns the node into an internal node
    Partition(node);

    // Subdivide node's children
    Subdivide(m_pool[node.leftFirst + 0]);  // Left child
    Subdivide(m_pool[node.leftFirst + 1]);  // Right child
}

void BVH::Partition(BVHNode& node)
{
    // Store for later
    const uint32_t first = node.leftFirst;
    const uint32_t last = first + node.count;

    //// TODO: Split and sort primitives
    ///        - Pretty confused about this part at the moment
    // Determine where to split
    XMVECTOR splitPos = XMLoadFloat3(&node.bounds.Center);

    // NOTE: Gross..
    float extentX = node.bounds.Extents.x;
    float extentY = node.bounds.Extents.y;
    float extentZ = node.bounds.Extents.z;
    uint8_t splitAxis = (extentX > extentY && extentX > extentZ ? 0 : (extentY > extentZ ? 1 : 2));
    
    ChildCounts childCounts = Split(first, last, splitPos, splitAxis);

    // Make parent internal node
    node.leftFirst = m_poolPtr;
    node.count = 0;

    // Create children
    BVHNode& childL = m_pool[m_poolPtr++];
    childL.leftFirst = first;
    childL.count = std::get<0>(childCounts);
    childL.bounds = CalculateBounds(childL.leftFirst, childL.count);

    BVHNode& childR = m_pool[m_poolPtr++];
    childR.leftFirst = first + childL.count;
    childR.count = std::get<1>(childCounts);
    childR.bounds = CalculateBounds(childR.leftFirst, childR.count);
}

auto XM_CALLCONV BVH::Split(uint32_t first, uint32_t last, FXMVECTOR splitPos, uint8_t splitAxis) -> ChildCounts
{
    assert(first < m_primitives.size());

    // Splits and sorts the index array
    uint32_t countLeft = 0;
    uint32_t countRight = 0;

    // TODO: Compare the center of my triangles to the splitPos
    for (int i = first; i < last; ++i)
    {
        const Triangle& triangle = m_primitives[ m_indices[i] ];

        // Calculate triangle center
        XMVECTOR center = XMVectorZero();
        for (int j = 0; j < 3; ++j)
            center += XMLoadFloat3(triangle.v[j]);
        center /= 3;

        // If triangle is on the left of the split position, it should be added to the left child--
        // meaning m_indices[i] should be moved to the countLeft side of the array
        if (center.m128_f32[splitAxis] < splitPos.m128_f32[splitAxis])
            m_indices[countLeft++] = i;
        else
            m_indices[last - 1 - countRight++] = i;
    }

    return std::make_tuple(countLeft, countRight);
}
