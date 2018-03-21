#pragma once
#include <d3d11_1.h>
#include <SimpleMath.h>
#include <memory>
#include <vector>
#include <array>

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
        std::array<std::unique_ptr<Node>, 8> children;

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

    Octree() = default;

    // Octree will merge these bounding boxes to create the root node
    // It will then insert all of these boxes into the quadtree
    Octree(const DirectX::BoundingBox& rootBoundingBox, DirectX::VertexPositionNormalTexture* vertices, size_t numVertices);

    void Initialise(const DirectX::BoundingBox& rootBoundingBox);

    // Insert new triangle into the octree
    void Insert(const Triangle& newTri);

    // Build the BVH (tighten the bounding volumes)
    void Build();

    bool Intersects(const DirectX::SimpleMath::Ray& ray, DirectX::SimpleMath::Vector3& hit) const;

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

