#pragma once
#include <vector>
class Mesh;
using namespace std;

class Model {
public:
	Model();
	~Model();

	vector<Mesh> meshes;
};

