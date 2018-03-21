#pragma once
#include <d3d11_1.h>
#include <SimpleMath.h>
#include <memory>
#include <vector>

// WARNING: I may need to add MAX_DEPTH to the tree (however, a Node can only contain one quad atm)

class Octree
{
    struct Data;

    struct Node
    {
        std::unique_ptr<Node> children[8];

        // Bounding box that encompasses all this node's children
        std::shared_ptr<DirectX::BoundingBox> boundingBox;

        bool isLeaf = true;
        std::vector<Data> vertices;
    };

    constexpr static uint8_t RIGHT_HALF = (1 << 2);
    constexpr static uint8_t TOP_HALF   = (1 << 1);
    constexpr static uint8_t FAR_HALF   = (1 << 0);

public:
    struct Data
    {
        DirectX::SimpleMath::Vector3 bottomLeft;
        DirectX::SimpleMath::Vector3 bottomRight;
        DirectX::SimpleMath::Vector3 topLeft;
        DirectX::SimpleMath::Vector3 topRight;

        std::shared_ptr<DirectX::BoundingBox> boundingBox;
    };

    // Octree will merge these bounding boxes to create the root node
    // It will then insert all of these boxes into the quadtree
    explicit Octree(const std::vector<DirectX::BoundingBox>& boundingBoxes);

    // Insert new quad into the quad tree
    void Insert(const Data& newQuad);

private:
    void Insert(Node& parentNode, const Data& newQuad);

    // Computes the bounds of a NODE (not its contents)--imagine the root node as a massive square--
    // this is the function that computes the four sub-squares as the bounds of its four children
    //  This step is necessary for determining which side(child) of the tree(node) a newly inserted quad should go to
    DirectX::BoundingBox ComputeChildBounds(int childIdx, const DirectX::BoundingBox& parentBoundingBox) const;

    Node m_root;
};

