// model loading
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include <directxmath.h>
#include <d3d11_2.h>
#include <vector>
using namespace DirectX;
using namespace std;

struct Vertex {
	XMFLOAT4 position;
	XMFLOAT4 normal;
	XMFLOAT4 tangent;
	XMFLOAT3 texture;
};

struct Mesh {
	vector<Vertex> vertices;
	vector<unsigned long> indices;
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
		//aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType |
		aiProcess_FlipUVs);
	if (!scene) {
		printf("Couldn't open %s", argv[1]);
		return -1;
	}

	float scale = 0.01f;
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
					verts[j].position = XMFLOAT4(aiMesh->mVertices[j].x * scale, aiMesh->mVertices[j].y * scale, aiMesh->mVertices[j].z * scale, 1.f);
					verts[j].normal = XMFLOAT4(aiMesh->mNormals[j].x, aiMesh->mNormals[j].y, aiMesh->mNormals[j].z, 0.f);
					verts[j].normal = XMFLOAT4(aiMesh->mTangents[j].x, aiMesh->mTangents[j].y, aiMesh->mTangents[j].z, 0.f);
					verts[j].texture = XMFLOAT3(aiMesh->mTextureCoords[0][j].x, aiMesh->mTextureCoords[0][j].y, aiMesh->mTextureCoords[0][j].z);
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
	errno_t error = fopen_s(&file, argv[2], "wb");
	if (error != 0) {
		printf("Couldn't open the output file %s", argv[2]);
		return -1;
	}
	// number of meshes
	unsigned meshCount = meshes.size();
	fwrite(&meshCount, sizeof(meshCount), 1, file);
	for (auto mesh : meshes) {
		// vertex count
		unsigned vertexCount = mesh.vertices.size();
		fwrite(&vertexCount, sizeof(vertexCount), 1, file);
		// vertices
		fwrite(&mesh.vertices[0], sizeof(Vertex), vertexCount, file);
		// index count
		unsigned indexCount = mesh.indices.size();
		// indices
		fwrite(&indexCount, sizeof(indexCount), 1, file);
		fwrite(&mesh.indices[0], sizeof(unsigned long), indexCount, file);
	}
	fclose(file);
}
