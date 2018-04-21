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
    const XMVECTOR MAX = XMVectorSplatOne() * std::numeric_limits<float>::max();
    const XMVECTOR MIN = XMVectorSplatOne() * std::numeric_limits<float>::lowest();
}

bool BVH::Intersects(FXMVECTOR origin, FXMVECTOR direction, XMVECTOR& hit) const
{
    float dist;
    if (!m_root->bounds.Intersects(origin, direction, dist))
        return false;

    if (Intersects(*m_root, origin, direction, dist))
    {
        hit = origin + (direction * dist);
        return true;
    }

    return false;
}

void BVH::Refit()
{
    Refit(*m_root);
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
    XMVECTOR min = MAX;
    XMVECTOR max = MIN;

    // Find the two corner vertices that make up the bounding box
    for (int i = first; i < first + count; ++i)
    {
        const Triangle& triangle = m_primitives[i];

        for (int j = 0; j < 3; ++j)
        {
            XMVECTOR vertex = XMLoadFloat3(triangle.v[j]);

            min = XMVectorMin(min, vertex);
            max = XMVectorMax(max, vertex);
        }
    }

    BoundingBox bounds;
    BoundingBox::CreateFromPoints(bounds, min, max);

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
    // Store for convenience
    const uint32_t first = node.leftFirst;
    const uint32_t last = first + node.count;

    //// Partition primitives according to some heuristic
    // Determine where to split
    XMVECTOR splitPos = XMLoadFloat3(&node.bounds.Center);

    // In this case heuristic just splits 50/50 along the largest axis
    // NOTE: This could probably be done much better
    float extentX = node.bounds.Extents.x;
    float extentY = node.bounds.Extents.y;
    float extentZ = node.bounds.Extents.z;
    uint8_t splitAxis = (extentX > extentY && extentX > extentZ ? 0 : (extentY > extentZ ? 1 : 2));

    // Perform the partition
    auto pivot = std::partition(m_primitives.begin() + first, m_primitives.begin() + last,
    [&](const Triangle& triangle) {
        // Calculate triangle center
        XMVECTOR center = XMVectorZero();
        for (int j = 0; j < 3; ++j)
            center += XMLoadFloat3(triangle.v[j]);
        center /= 3;
        
        // If triangle is on the left of the split position, is belongs to the left child, otherwise it belongs to the right child
        return center.m128_f32[splitAxis] < splitPos.m128_f32[splitAxis];
    });

    // Make parent internal node
    node.leftFirst = m_poolPtr;
    node.count = 0;

    // Create children
    BVHNode& childL = m_pool[m_poolPtr++];
    childL.leftFirst = first;
    childL.count = std::distance(m_primitives.begin() + first, pivot);
    childL.bounds = CalculateBounds(childL.leftFirst, childL.count);

    BVHNode& childR = m_pool[m_poolPtr++];
    childR.leftFirst = first + childL.count;
    childR.count = std::distance(pivot, m_primitives.begin() + last);
    childR.bounds = CalculateBounds(childR.leftFirst, childR.count);
}

bool BVH::Intersects(const BVHNode& node, FXMVECTOR origin, FXMVECTOR direction, float& dist) const
{
    float tempDist;
    if (!node.bounds.Intersects(origin, direction, tempDist))
        return false;

    dist = std::numeric_limits<float>::max();
    // Node is a leaf
    if (node.count > 0)
    {
        // Check for intersection with triangles
        for (int i = 0; i < node.count; ++i)
        {
            const Triangle& triangle = m_primitives[node.leftFirst + i];

            XMVECTOR t0 = XMLoadFloat3(triangle.v[0]);
            XMVECTOR t1 = XMLoadFloat3(triangle.v[1]);
            XMVECTOR t2 = XMLoadFloat3(triangle.v[2]);

            if (TriangleTests::Intersects(origin, direction, t0, t1, t2, tempDist))
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
        if (Intersects(m_pool[node.leftFirst + 0], origin, direction, tempDist))
        {
            if (tempDist < dist)
                dist = tempDist;
        }

        if (Intersects(m_pool[node.leftFirst + 1], origin, direction, tempDist))
        {
            if (tempDist < dist)
                dist = tempDist;
        }
    }

    return (dist < std::numeric_limits<float>::max());
}

void BVH::Refit(BVHNode& node)
{
    if (node.count > 0)
        // Update leaf node bounding box (to fit primitives)
        node.bounds = CalculateBounds(node.leftFirst, node.count);
    else
    {
        BVHNode& childL = m_pool[node.leftFirst + 0];
        BVHNode& childR = m_pool[node.leftFirst + 1];

        Refit(childL);
        Refit(childR);

        // Update internal node bounding box (to fit children)
        BoundingBox::CreateMerged(node.bounds, childL.bounds, childR.bounds);
    }
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
