#pragma once
#include <vector>
class Mesh;
class Transform;
using namespace std;

class Model {
public:
	Model();
	~Model();

	vector<Mesh*> meshes;
	Transform* transform;
};

