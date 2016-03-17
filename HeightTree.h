#pragma once

class HeightTree {
public:
	unsigned s = 64;
	float height = 0.0f;
	float averageBelow = 0.0f;
	HeightTree* heightTrees[4];
	HeightTree();
	~HeightTree();

	float GetHeight(unsigned x, unsigned y, unsigned s);
	void SetHeight(unsigned x, unsigned y, unsigned s, float v);
};

