#include "HeightTree.h"
#include <cassert>
#include <algorithm>

HeightTree::HeightTree() {
	for (auto& tree : heightTrees) {
		tree = nullptr;
	}
}


HeightTree::~HeightTree() {
}

float HeightTree::GetHeight(unsigned x, unsigned y, unsigned s) {
	if (s == this->s)
		return height + averageBelow;
	unsigned halfS = this->s / 2;
	auto tree = heightTrees[(y / halfS) * 2 + x / halfS];
	if (tree == nullptr)
		return height + averageBelow;
	return height + tree->GetHeight(x % halfS, y % halfS, s);
}

void HeightTree::SetHeight(unsigned x, unsigned y, unsigned s, float v) {
	if (x == 0 && y == 0 && s == this->s || this->s == 1)
		height = v;
	else {
		unsigned halfS = this->s / 2;
		auto& tree = heightTrees[(y / halfS) * 2 + x / halfS];
		if (tree == nullptr) {
			tree = new HeightTree();
			tree->s = halfS;
			tree->height = 0.0f;
		}
		// set the height and update the average for this height
		float heightBefore = tree->height + tree->averageBelow;
		tree->SetHeight(x - (x / halfS) * halfS, y - (y / halfS) * halfS, s, v - height);
		float heightAfter = tree->height + tree->averageBelow;
		float diff = heightAfter - heightBefore;
		averageBelow += diff * 0.25f;
	}
}