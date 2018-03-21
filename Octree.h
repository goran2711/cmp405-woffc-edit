#pragma once
#include <d3d11_1.h>
#include <SimpleMath.h>
#include <memory>
#include <vector>

// WARNING: I may need to add MAX_DEPTH to the tree (however, a Node can only contain one quad atm)

namespace DirectX
{
    class VertexPositionNormalTexture;
}

class Octree
{
    struct Triangle;

    struct Node
    {
        std::unique_ptr<Node> children[8];

        // Bounding box that encompasses all this node's children
        DirectX::BoundingBox boundingBox;

        bool isLeaf = true;
        std::vector<Triangle> triangles;
    };

    constexpr static uint8_t RIGHT_HALF = (1 << 2);
    constexpr static uint8_t TOP_HALF   = (1 << 1);
    constexpr static uint8_t FAR_HALF   = (1 << 0);

public:
    struct Triangle
    {
        DirectX::SimpleMath::Vector3 v0, v1, v2;
    };

    // Octree will merge these bounding boxes to create the root node
    // It will then insert all of these boxes into the quadtree
    Octree(const DirectX::BoundingBox& rootBoundingBox, DirectX::VertexPositionNormalTexture* vertices, size_t numVertices);

    // Insert new triangle into the octree
    void Insert(const Triangle& newTri);

    // Build the BVH (tighten the bounding volumes)
    void Build();

private:
    void Insert(Node& parentNode, const Triangle& newTri);

    // Computes the bounds of a NODE (not its contents)--imagine the root node as a massive square--
    // this is the function that computes the four sub-squares as the bounds of its four children
    //  This step is necessary for determining which side(child) of the tree(node) a newly inserted quad should go to
    DirectX::BoundingBox ComputeChildBounds(int childIdx, const DirectX::BoundingBox& parentBoundingBox) const;

    void Build(Node& node);

    DirectX::BoundingBox ComputeTriangleBoundingBox(const Triangle& triangle) const;


    Node m_root;
};

