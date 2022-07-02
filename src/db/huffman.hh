#pragma once

#include <array>
#include <cstdint>
#include <queue>
#include <vector>

namespace huffman
{

struct Symbol
{
	uint32_t symbol, count;
};

struct Node
{
	Symbol sym;
	int index;
	int parent, left, right;
};

template <std::size_t N>
struct Tree
{
	// binary tree will never contain more than N - 1
	// internal nodes, where N is the number of leaves
	std::array<Node, 2 * N - 1> nodes;

	void build_tree()
	{
		auto cmp = [](const Node &a, const Node &b) { return a.sym.count < b.sym.count; };
		std::priority_queue<Node, std::vector<Node>, decltype(cmp)> queue(cmp, nodes.begin(), nodes.end());
		std::size_t index = N;

		while (queue.size() >= 2)
		{
			auto a = queue.top();
			queue.pop();
			auto b = queue.top();
			queue.pop();

			Node parent;
			parent.sym.count = a.sym.count + b.sym.count;
			parent.left  = a.index;
			parent.right = b.index;
			parent.index = index++;

			nodes[index] = parent;
			nodes[a.index].parent = index;
			nodes[b.index].parent = index;

			queue.push(parent);
		}
	}
};

// Adapted from:
//   https://aquarchitect.github.io/swift-algorithm-club/Huffman%20Coding/

} // huffman
