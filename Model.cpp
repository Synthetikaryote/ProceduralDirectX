#include "Model.h"
#include "Mesh.h"
#include "Transform.h"

Model::Model() {
	transform = new Transform();
}


Model::~Model() {
	for (auto* mesh : meshes)
		mesh->Release();
	delete transform;
}
