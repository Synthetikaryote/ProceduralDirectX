// model loading
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include <vector>

using namespace std;

struct Vector4 {
	float x, y, z, w;
	Vector4() {}
	Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
};
struct Vector3 {
	float x, y, z;
	Vector3() {}
	Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
};

struct Vertex {
	Vector4 position;
	Vector4 normal;
	Vector3 texture;
};

struct Mesh {
	vector<Vertex> vertices;
	vector<unsigned> indices;
};

int main(int argc, char** argv) {
	if (argc < 3) {
		printf("Usage: %s [input] [output]", argv[0]);
		return -1;
	}

	// AssImp usage http://assimp.sourceforge.net/lib_html/usage.html
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(argv[1],
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType);
	if (!scene) {
		printf("Couldn't open %s", argv[1]);
		return -1;
	}

	vector<Mesh> meshes;
	if (scene->HasMeshes()) {
		meshes.resize(scene->mNumMeshes);
		for (unsigned i = 0; i < scene->mNumMeshes; ++i) {
			auto* aiMesh = scene->mMeshes[i];
			auto& mesh = meshes[i];
			if (aiMesh->HasPositions() && aiMesh->HasNormals() && aiMesh->HasTextureCoords(0) && aiMesh->HasFaces()) {
				auto& verts = mesh.vertices;
				verts.resize(aiMesh->mNumVertices);
				for (unsigned j = 0; j < aiMesh->mNumVertices; ++j) {
					verts[j].position = Vector4(aiMesh->mVertices[j].x, aiMesh->mVertices[j].y, aiMesh->mVertices[j].z, 1.f);
					verts[j].normal = Vector4(aiMesh->mNormals[j].x, aiMesh->mNormals[j].y, aiMesh->mNormals[j].z, 0.f);
					verts[j].texture = Vector3(aiMesh->mTextureCoords[0][j].x, aiMesh->mTextureCoords[0][j].y, aiMesh->mTextureCoords[0][j].z);
				}
				unsigned index = 0;
				mesh.indices.resize(aiMesh->mNumFaces * 3);
				for (unsigned j = 0; j < aiMesh->mNumFaces; ++j)
					for (unsigned k = 0; k < 3; ++k)
						mesh.indices[index++] = aiMesh->mFaces[j].mIndices[k];
			}
		}
	}

	// write it to a file for the engine
	FILE* file = nullptr;
	errno_t error = fopen_s(&file, argv[2], "w");
	if (error != 0) {
		printf("Couldn't open the output file %s", argv[2]);
		return -1;
	}
	fprintf(file, "Number of meshes: %u\n", meshes.size());
	for (auto mesh : meshes) {
		fprintf(file, "Number of vertices: %u\n", mesh.vertices.size());
		fprintf(file, "Number of indices: %u\n", mesh.indices.size());
		fprintf(file, "Vertices:\n");
		for (auto& v : mesh.vertices) {
			fprintf(file, "%f %f %f %f %f %f %f %f\n",
				v.position.x, v.position.y, v.position.z,
				v.normal.x, v.normal.y, v.normal.z,
				v.texture.x, v.texture.y);
		}
		fprintf(file, "Indices:\n");
		for (auto i : mesh.indices) {
			fprintf(file, "%u ", i);
		}
		fprintf(file, "\n");
	}
}
