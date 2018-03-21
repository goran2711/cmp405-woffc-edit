#include "Octree.h"
#include "VertexTypes.h"
#include <queue>

using namespace DirectX;
using namespace DirectX::SimpleMath;

Octree::Octree(const BoundingBox& rootBoundingBox, VertexPositionNormalTexture* vertices, size_t numVertices)
{
    m_root.boundingBox = rootBoundingBox;

    //// Insert nodes
    //for (size_t i = 0; i < numVertices; i += 3)
    //{

    //}
}

void Octree::Initialise(const BoundingBox& rootBoundingBox)
{
    m_root.boundingBox = rootBoundingBox;
}

void Octree::Insert(const Triangle& newTri)
{
    Insert(m_root, newTri);
}

void Octree::Build()
{
    Build(m_root);
}

bool Octree::Intersects(const Ray& ray, Vector3& hit) const
{
    float dist;
    if (!ray.Intersects(m_root.boundingBox, dist))
        return false;

    int triTests = 0;
    int boxTests = 0;

    std::queue<const Node*> candidates;
    candidates.push(&m_root);

    Vector3 nearestHit;
    float minDist = FLT_MAX;
    while (!candidates.empty())
    {
        const Node* node = candidates.front();
        candidates.pop();

        // If it's a leaf, check for intersection with triangles inside
        if (node->isLeaf)
        {
            for (const auto tri : node->triangles)
            {
                ++triTests;

                dist = 0.f;
                if (ray.Intersects(tri.v0, tri.v1, tri.v2, dist))
                {
                    if (dist >= minDist)
                        continue;

                    nearestHit = ray.position + (ray.direction * dist);
                    minDist = dist;
                }
            }
        }
        // If it's an internal node, check for intersection with the child voxels
        else
        {
            for (const auto& child : node->children)
            {
                if (!child)
                    continue;

                ++boxTests;
                if (ray.Intersects(child->boundingBox, dist))
                    candidates.push(child.get());
            }
        }
    }

    if (minDist < FLT_MAX)
    {
        hit = nearestHit;
        return true;
    }

    hit = Vector3::Zero;
    return false;
}

void Octree::Insert(Node& parentNode, const Triangle& newTri)
{
    if (parentNode.isLeaf)
    {
        // If the leaf node is empty (or we have reached max depth), then assign the new quad to this node
        // NOTE: We prefer to have only one child in each leaf node
        // WARNING: May need MAX_DEPTH here
        if (parentNode.triangles.empty())
            parentNode.triangles.push_back(newTri);
        // If the node is not empty, this node will become an internal node, so the current contents need to be
        // moved (re-inserted) further down the tree
        else
        {
            // This node is no longer a leaf
            parentNode.isLeaf = false;

            // Move the node's contents to one of its child nodes
            while (!parentNode.triangles.empty())
            {
                Insert(parentNode, parentNode.triangles.back());
                parentNode.triangles.pop_back();
            }

            // Try inserting the new object again
            Insert(parentNode, newTri);
        }
    }
    // If the node is not a leaf (it is an internal node) we need to figure out which of its children this new object is going to go to
    else
    {
        int childIdx = 0;

        Vector3 center = (newTri.v0 + newTri.v1 + newTri.v2) / 3;

        // Child is in the right half of the voxel
        if (center.x > parentNode.boundingBox.Center.x)
            childIdx += RIGHT_HALF;
        // Child is in the upper half of the voxel
        if (center.y > parentNode.boundingBox.Center.y)
            childIdx += TOP_HALF;
        // Child is in the far half of the voxel
        if (center.z > parentNode.boundingBox.Center.z)
            childIdx += FAR_HALF;

        // Create and initialise the child node if it doesn't exist yet
        if (!parentNode.children[childIdx])
        {
            parentNode.children[childIdx] = std::make_unique<Node>();

            // Calculate the octree bounds of the new child
            parentNode.children[childIdx]->boundingBox = ComputeChildBounds(childIdx, parentNode.boundingBox);
        }

        // Insert the new object into this child node
        Insert(*parentNode.children[childIdx], newTri);
    }
}

BoundingBox Octree::ComputeChildBounds(int childIdx, const BoundingBox& parentBoundingBox) const
{
    Vector3 min;
    Vector3 max;

    // If child is in right half
    min.x = (childIdx & RIGHT_HALF) ? parentBoundingBox.Center.x : parentBoundingBox.Center.x - parentBoundingBox.Extents.x;
    max.x = (childIdx & RIGHT_HALF) ? parentBoundingBox.Center.x + parentBoundingBox.Extents.x : parentBoundingBox.Center.x;

    // If child is in upper half
    min.y = (childIdx & TOP_HALF) ? parentBoundingBox.Center.y : parentBoundingBox.Center.y - parentBoundingBox.Extents.y;
    max.y = (childIdx & TOP_HALF) ? parentBoundingBox.Center.y + parentBoundingBox.Extents.y : parentBoundingBox.Center.y;

    // If child is in near half
    min.z = (childIdx & FAR_HALF) ? parentBoundingBox.Center.z : parentBoundingBox.Center.z - parentBoundingBox.Extents.z;
    max.z = (childIdx & FAR_HALF) ? parentBoundingBox.Center.z + parentBoundingBox.Extents.z : parentBoundingBox.Center.z;

    BoundingBox childBounds;
    BoundingBox::CreateFromPoints(childBounds, min, max);

    return childBounds;
}

void Octree::Build(Node& node)
{
    if (node.isLeaf && !node.triangles.empty()) // can triangles even be empty as this point? (not w/o DEPTH_MAX)
    {
        // Update the node's bounding box to encompass its contents
        auto it = node.triangles.begin();

        // Initialise node bounding box
        node.boundingBox = ComputeTriangleBoundingBox(*it);

        // Expand node bounding box
        ++it;
        for (it; it != node.triangles.end(); ++it)
            BoundingBox::CreateMerged(node.boundingBox, node.boundingBox, ComputeTriangleBoundingBox(*it));
    }
    else
    {
        // Update the node's bounding box to encompass its children
        for (auto& child : node.children)
        {
            if (!child)
                continue;

            // WARNING: In my reference implementation, I can making another call to ComputeChildBounds here

            Build(*child);

            // Merge this parent node's bounding box with that of its child
            BoundingBox::CreateMerged(node.boundingBox, node.boundingBox, child->boundingBox);
        }
    }
}

DirectX::BoundingBox Octree::ComputeTriangleBoundingBox(const Triangle& triangle) const
{
    Vector3 min = Vector3::Min(triangle.v0, triangle.v1);
    min = Vector3::Min(min, triangle.v2);

    Vector3 max = Vector3::Max(triangle.v0, triangle.v1);
    max = Vector3::Max(max, triangle.v2);

    BoundingBox boundingBox;
    BoundingBox::CreateFromPoints(boundingBox, min, max);

    return boundingBox;
}
