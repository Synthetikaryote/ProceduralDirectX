#pragma once
#include <vector>
#include "Mesh.h"
using namespace std;

class Model {
public:
	Model();
	~Model();

	vector<Mesh> meshes;
};

