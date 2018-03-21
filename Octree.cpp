#include "Octree.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

void Octree::Insert(const Data& newQuad)
{
    Insert(m_root, newQuad);
}

void Octree::Insert(Node& parentNode, const Data& newQuad)
{
    if (parentNode.isLeaf)
    {
        // If the leaf node is empty (or we have reached max depth), then assign the new quad to this node
        // NOTE: We prefer to have only one child in each leaf node
        // WARNING: May need MAX_DEPTH here
        if (parentNode.vertices.empty())
        {
            parentNode.vertices.push_back(newQuad);

            parentNode.boundingBox = parentNode.vertices.back().boundingBox;
        }
        // If the node is not empty, this node will become an internal node, so the current contents need to be
        // moved (re-inserted) further down the tree
        else
        {
            // This node is no longer a leaf
            parentNode.isLeaf = false;

            // Move the node's contents to one of its child nodes
            while (!parentNode.vertices.empty())
            {
                Insert(parentNode, parentNode.vertices.back());
                parentNode.vertices.pop_back();
            }

            // Try inserting the new object again
            Insert(parentNode, newQuad);
        }
    }
    // If the node is not a leaf (it is an internal node) we need to figure out which of its children this new object is going to go to
    else
    {
        int childIdx = 0;

        // Child is in the right half of the voxel
        if (newQuad.boundingBox->Center.x > parentNode.boundingBox->Center.x)
            childIdx += RIGHT_HALF;
        // Child is in the upper half of the voxel
        if (newQuad.boundingBox->Center.y > parentNode.boundingBox->Center.y)
            childIdx += TOP_HALF;
        // Child is in the far half of the voxel
        if (newQuad.boundingBox->Center.z > parentNode.boundingBox->Center.z)
            childIdx += FAR_HALF;

        // Create the child node if it doesn't exist yet
        if (!parentNode.children[childIdx])
            parentNode.children[childIdx] = std::make_unique<Node>();

        // TODO: Calculate the bounds of the new voxel
    }
}

BoundingBox Octree::ComputeChildBounds(int childIdx, const BoundingBox& parentBoundingBox) const
{
    Vector3 min;
    Vector3 max;

    


    BoundingBox childBounds;


    return childBounds;
}
