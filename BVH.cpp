#include "BVH.h"
#include <tuple>
#include <algorithm>
#include <numeric>

// bad macros are bad
#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

namespace
{
    // Have to add/subtract one to get around some overflow(?) weirdness
    const XMVECTOR MAX = XMVectorSplatOne() * std::numeric_limits<float>::max();
    const XMVECTOR MIN = XMVectorSplatOne() * std::numeric_limits<float>::lowest();
}

bool BVH::Intersects(const SimpleMath::Ray& ray, SimpleMath::Vector3& hit) const
{
    float dist;
    if (!ray.Intersects(m_root->bounds, dist))
        return false;

    if (Intersects(*m_root, ray, dist))
    {
        hit = ray.position + (ray.direction * dist);
        return true;
    }

    return false;
}

void BVH::Refit()
{
    // A node's parent is always going to be to the left
    for (int i = m_poolPtr - 1; i >= 0; --i)
    {
        BVHNode& node = m_pool[i];

        if (node.count > 0)
            // Recalculate leaf node bounds from the primitives within
            node.bounds = CalculateBounds(node.leftFirst, node.count);
        else
        {
            BVHNode& childL = m_pool[node.leftFirst + 0];
            BVHNode& childR = m_pool[node.leftFirst + 1];

            // Recalculate internal node bounds from its children
            BoundingBox::CreateMerged(node.bounds, childL.bounds, childR.bounds);
        }
    }
}

void BVH::InitialiseDebugVisualiastion(ID3D11DeviceContext* context)
{
    static const XMFLOAT3 size(2.f, 2.f, 2.f);
    m_box = GeometricPrimitive::CreateBox(context, size);
}

void XM_CALLCONV BVH::DebugRender(ID3D11DeviceContext* context, FXMMATRIX view, CXMMATRIX projection, int depth)
{
    DebugRender(*m_root, context, view, projection, 0, depth);
}

void BVH::InitialiseIndexArray(size_t size)
{
    m_indices.resize(size);
    std::iota(m_indices.begin(), m_indices.end(), 0);
}

void BVH::InitialiseNodes(size_t size)
{
    // Initialise node pool and root
    m_pool.resize(size * 2 - 1);
    m_root = &m_pool[0];

    // Root starts as a leaf with all the primitives within it
    m_root->leftFirst = 0;
    m_root->count = size;

    m_root->bounds = CalculateBounds(m_root->leftFirst, m_root->count);

    // Subdivide root
    Subdivide(*m_root);
}

BoundingBox BVH::CalculateBounds(int first, int count) const
{
    assert(first < m_indices.size());

    BoundingBox bounds;

    XMVECTOR min = MAX;
    XMVECTOR max = MIN;

    // Initialise bounding box
    {
        const Triangle& triangle = m_primitives[m_indices[first]];

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
        const Triangle& triangle = m_primitives[m_indices[i]];

        min = MAX;
        max = MIN;

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

void BVH::Subdivide(BVHNode& node, int depth)
{
    // NOTE: Exit condition: stop subdividing if this new leaf has a minimum number of primitives
    //       (can also add if we reached a certain depth)
    // BUG: Stack overflow with nodes that have .count == 2 and don't split before depth > 20
    // FIX: Very suceptible to stack overflows at the moment--a better splitting strategy may help
    if (node.count < 4 || depth > 16)
        return;

    // Partiton node (creates its two children)
    // NOTE: Partition turns the node into an internal node
    Partition(node);

    // Subdivide node's children
    Subdivide(m_pool[node.leftFirst + 0], depth + 1);  // Left child
    Subdivide(m_pool[node.leftFirst + 1], depth + 1);  // Right child
}

void BVH::Partition(BVHNode& node)
{
    // Store for later
    const uint32_t first = node.leftFirst;
    const uint32_t last = first + node.count;

    //// Split and sort primitives
    // Determine where to split
    XMVECTOR splitPos = XMLoadFloat3(&node.bounds.Center);

    // Just split 50/50 along the largest axis
    // NOTE: Gross..
    float extentX = node.bounds.Extents.x;
    float extentY = node.bounds.Extents.y;
    float extentZ = node.bounds.Extents.z;
    uint8_t splitAxis = (extentX > extentY && extentX > extentZ ? 0 : (extentY > extentZ ? 1 : 2));

    // NOTE: Wasteful on memory
    std::vector<int> sortedIndices(m_indices.size());
    ChildCounts childCounts = Split(first, last, splitPos, splitAxis, sortedIndices);

    std::copy(sortedIndices.cbegin() + first, sortedIndices.cbegin() + last, m_indices.begin() + first);

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

auto XM_CALLCONV BVH::Split(uint32_t first, uint32_t last, FXMVECTOR splitPos, uint8_t splitAxis, std::vector<int>& sortedIndices) -> ChildCounts
{
    assert(first < m_primitives.size());

    // Splits and sorts the index array
    uint32_t countLeft = 0;
    uint32_t countRight = 0;

    // Compare the centre of the triangles to the splitPos
    for (int i = first; i < last; ++i)
    {
        const Triangle& triangle = m_primitives[m_indices[i]];

        // Calculate triangle center
        XMVECTOR center = XMVectorZero();
        for (int j = 0; j < 3; ++j)
            center += XMLoadFloat3(triangle.v[j]);
        center /= 3;

        // If triangle is on the left of the split position, it should be added to the left child--
        // meaning m_indices[i] should be moved to the countLeft side of the array
        if (center.m128_f32[splitAxis] < splitPos.m128_f32[splitAxis])
            sortedIndices[first + countLeft++] = m_indices[i];
        else
            sortedIndices[last - 1 - countRight++] = m_indices[i];
    }

    return std::make_tuple(countLeft, countRight);
}

bool BVH::Intersects(BVHNode& node, const SimpleMath::Ray& ray, float& dist) const
{
    float tempDist;
    if (!ray.Intersects(node.bounds, tempDist))
        return false;

    dist = std::numeric_limits<float>::max();
    // Node is a leaf
    if (node.count > 0)
    {
        // Check for intersection with triangles
        for (int i = 0; i < node.count; ++i)
        {
            const Triangle& triangle = m_primitives[m_indices[node.leftFirst + i]];

            if (ray.Intersects(*triangle.v[0], *triangle.v[1], *triangle.v[2], tempDist))
            {
                if (tempDist < dist)
                    dist = tempDist;
            }
        }
    }
    // Node is internal
    else
    {
        // Check for intersection with children
        if (Intersects(m_pool[node.leftFirst + 0], ray, tempDist))
        {
            if (tempDist < dist)
                dist = tempDist;
        }

        if (Intersects(m_pool[node.leftFirst + 1], ray, tempDist))
        {
            if (tempDist < dist)
                dist = tempDist;
        }
    }

    return (dist < std::numeric_limits<float>::max());
}

void XM_CALLCONV BVH::DebugRender(BVHNode& node, ID3D11DeviceContext* context, FXMMATRIX view, CXMMATRIX projection, int currentDepth, int depth)
{
    // Only render nodes at the requested depth
    if (currentDepth == depth)
    {
        XMVECTOR position = XMLoadFloat3(&node.bounds.Center);
        XMVECTOR extents = XMLoadFloat3(&node.bounds.Extents);

        XMMATRIX world = XMMatrixIdentity() * XMMatrixScalingFromVector(extents) * XMMatrixTranslationFromVector(position);
        m_box->Draw(world, view, projection, Colors::Red, nullptr, true);
    }
    else if (currentDepth > depth)
        return;

    // Leaf nodes have no children to render
    if (node.count > 0)
        return;

    DebugRender(m_pool[node.leftFirst + 0], context, view, projection, currentDepth + 1, depth);
    DebugRender(m_pool[node.leftFirst + 1], context, view, projection, currentDepth + 1, depth);
}
