#include "Mesh.h"
#include "Uber.h"
#include <Shlwapi.h>
#include <string>
#include "Texture.h"
#include "Shader.h"
#include <map>
#include "ResourceManager.h"
using namespace std;

Mesh* GeneratePlane(unsigned columns, unsigned rows);
Mesh* GenerateCubeSphere(unsigned gridSize);
Mesh* GenerateSphere(unsigned longitudes, unsigned latitudes);

Mesh::Mesh() {
}

Mesh::~Mesh() {
	if (vertices) delete[] vertices;
	if (indices) delete[] indices;
	if (vertexBuffer) vertexBuffer->Release();
	if (indexBuffer) indexBuffer->Release();
}

void Mesh::Release() {
	for (auto& binding : textureBindings)
		binding.texture->Release();
	textureBindings.clear();
	if (shader) {
		shader->Release();
		shader = nullptr;
	}
	Resource::Release();
}

void Mesh::Draw() {
	// give the pixel shader the textures
	for (auto& binding : textureBindings)
		Uber::I().context->PSSetShaderResources(binding.shaderSlot, 1, &binding.texture->shaderResourceView);

	// render the mesh
	Uber::I().context->DrawIndexed(indexCount, 0, 0);
}

Mesh* Mesh::LoadQuad() {
	return Mesh::LoadPlane(1, 1);
}

Mesh* Mesh::LoadPlane(unsigned columns, unsigned rows) {
	char keyString[40];
	sprintf_s(keyString, "MeshPlane%u,%u", columns, rows);
	size_t key = hash<string>()(string(keyString));

	return Uber::I().resourceManager->Load<Mesh>(key, [columns, rows] {
		return GeneratePlane(columns, rows);
	});
}

Mesh* Mesh::LoadSphere(unsigned longitudes, unsigned latitudes) {
	char keyString[40];
	sprintf_s(keyString, "MeshSphere%u,%u", longitudes, latitudes);
	size_t key = hash<string>()(string(keyString));

	return Uber::I().resourceManager->Load<Mesh>(key, [longitudes, latitudes] {
		return GenerateSphere(longitudes, latitudes);
	});
}

Mesh* Mesh::LoadCubeSphere(unsigned gridSize) {
	char keyString[40];
	sprintf_s(keyString, "MeshCubeSphere%u", gridSize);
	size_t key = hash<string>()(string(keyString));

	return Uber::I().resourceManager->Load<Mesh>(key, [gridSize] {
		return GenerateCubeSphere(gridSize);
	});
}

Mesh* Mesh::LoadFromFile(string path) {
	char* filename = PathFindFileNameA(path.c_str());
	size_t key = hash<string>()(string(filename));
	return Uber::I().resourceManager->Load<Mesh>(key, [path] {
		// write it to a file for the engine
		FILE* file = nullptr;
		errno_t error = fopen_s(&file, path.c_str(), "rb");
		assert(error == 0);
		// number of meshes
		unsigned meshCount = 0;
		fread(&meshCount, sizeof(meshCount), 1, file);
		// can't handle more than one mesh yet
		assert(meshCount == 1);

		Mesh* mesh = new Mesh();
		// vertex count
		fread(&mesh->vertexCount, sizeof(mesh->vertexCount), 1, file);
		// vertices
		mesh->vertices = new Vertex[mesh->vertexCount];
		fread(mesh->vertices, sizeof(Vertex), mesh->vertexCount, file);
		// index count
		// indices
		fread(&mesh->indexCount, sizeof(mesh->indexCount), 1, file);
		mesh->indices = new unsigned long[mesh->indexCount];
		fread(mesh->indices, sizeof(unsigned long), mesh->indexCount, file);
		fclose(file);

		// create the vertex and index buffers
		mesh->CreateBuffers();

		return mesh;
	});
}

void Mesh::CreateBuffers() {
	assert(vertices);
	assert(indices);

	D3D11_BUFFER_DESC vertexDesc = {sizeof(Vertex) * vertexCount, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0u, 0u, 0u};
	D3D11_SUBRESOURCE_DATA vertexData = {vertices, 0u, 0u};
	ThrowIfFailed(Uber::I().device->CreateBuffer(&vertexDesc, &vertexData, &vertexBuffer));
	D3D11_BUFFER_DESC indexDesc = {sizeof(unsigned long) * indexCount, D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER, 0u, 0u, 0u};
	D3D11_SUBRESOURCE_DATA indexData = {indices, 0u, 0u};
	ThrowIfFailed(Uber::I().device->CreateBuffer(&indexDesc, &indexData, &indexBuffer));
}


//// make a triangle
//unsigned vertexCount = 3;
//unsigned indexCount = 3;
//VertexTextureType* vertices = new VertexTextureType[vertexCount];
//unsigned long* indices = new unsigned long[indexCount];
//vertices[0].position = XMFLOAT3(-1.0f, -1.0f, 0.0f);  // bottom left
//vertices[0].texture = XMFLOAT2(0.0f, 1.0f);
////vertices[0].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
//vertices[1].position = XMFLOAT3(0.0f, 1.0f, 0.0f);  // top middle
//vertices[1].texture = XMFLOAT2(0.5f, 0.0f);
////vertices[1].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
//vertices[2].position = XMFLOAT3(1.0f, -1.0f, 0.0f);  // bottom right
//vertices[2].texture = XMFLOAT2(1.0f, 1.0f);
////vertices[2].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
//indices[0] = 0;  // bottom left
//indices[1] = 1;  // top middle
//indices[2] = 2;  // bottom right

// private functions

Mesh* GeneratePlane(unsigned columns, unsigned rows) {
	Mesh* mesh = new Mesh();
	mesh->vertexCount = (columns + 1) * (rows + 1);
	mesh->indexCount = columns * rows * 3 * 2;
	mesh->vertices = new Vertex[mesh->vertexCount];
	mesh->indices = new unsigned long[mesh->indexCount];
	float xStep = 1.0f / static_cast<float>(columns);
	float yStep = 1.0f / static_cast<float>(rows);
	unsigned long v = 0;
	unsigned long i = 0;
	for (unsigned r = 0; r <= rows; ++r) {
		for (unsigned c = 0; c <= columns; ++c) {
			mesh->vertices[v].position = XMFLOAT4(c * xStep, r * yStep, 0.0f, 1.0f);
			mesh->vertices[v].normal = XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f);
			mesh->vertices[v].tangent = XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
			mesh->vertices[v].texture = XMFLOAT3(c * xStep, r * yStep, 0.0f);
			if (c < columns && r < rows) {
				mesh->indices[i++] = v;
				mesh->indices[i++] = v + 1;
				mesh->indices[i++] = v + (columns + 1) + 1;
				mesh->indices[i++] = v;
				mesh->indices[i++] = v + (columns + 1) + 1;
				mesh->indices[i++] = v + (columns + 1);
			}
			++v;
		}
	}

	// create the vertex and index buffers
	mesh->CreateBuffers();

	return mesh;
}

// make a sphere
Mesh* GenerateSphere(unsigned longitudes, unsigned latitudes) {
	Mesh* mesh = new Mesh();
	mesh->vertexCount = (latitudes + 1) * (longitudes + 1);
	mesh->indexCount = (latitudes - 1) * (longitudes + 1) * 2 * 3;
	mesh->vertices = new Vertex[mesh->vertexCount];
	mesh->indices = new unsigned long[mesh->indexCount];
	const float latStep = PI / latitudes;
	const float lonStep = TWOPI / longitudes;
	unsigned long v = 0;
	for (unsigned lat = 0; lat <= latitudes; ++lat) {
		for (unsigned lon = 0; lon <= longitudes; ++lon) {
			const float alat = lat * latStep;
			const float alon = lon * lonStep;
			mesh->vertices[v].position = XMFLOAT4(sin(alat) * cos(alon), cos(alat), sin(alat) * sin(alon), 1.0f);
			mesh->vertices[v].normal = mesh->vertices[v].position;
			mesh->vertices[v].normal.w = 0.0f;
			mesh->vertices[v].tangent = XMFLOAT4(sin(alat) * cos(alon + lonStep) - sin(alat) * cos(alon), 0.0f, sin(alat) * sin(alon + lonStep) - sin(alat) * sin(alon), 0.0f);
			mesh->vertices[v++].texture = XMFLOAT3((float)lon / longitudes, -cos(alat) * 0.5f + 0.5f, 0.0f);
		}
	}
	unsigned index = 0;
	for (unsigned lat = 0; lat < latitudes; ++lat) {
		for (unsigned lon = 0; lon < longitudes; ++lon) {
			if (lat != latitudes - 1) {
				mesh->indices[index++] = lat * (longitudes + 1) + (lon % (longitudes + 1));
				mesh->indices[index++] = (lat + 1) * (longitudes + 1) + ((lon + 1) % (longitudes + 1));
				mesh->indices[index++] = (lat + 1) * (longitudes + 1) + (lon % (longitudes + 1));
			}
			if (lat != 0) {
				mesh->indices[index++] = lat * (longitudes + 1) + (lon % (longitudes + 1));
				mesh->indices[index++] = lat * (longitudes + 1) + ((lon + 1) % (longitudes + 1));
				mesh->indices[index++] = (lat + 1) * (longitudes + 1) + ((lon + 1) % (longitudes + 1));
			}
		}
	}

	// create the vertex and index buffers
	mesh->CreateBuffers();

	return mesh;
}

// (quadrilateralized spherical cube)
Mesh* GenerateCubeSphere(unsigned gridSize) {
	assert(gridSize > 0);
	Mesh* mesh = new Mesh();
	unsigned subdivisions = gridSize - 1;
	// base cube's 8 vertices + the subdivisions on each edge + the subdivisions in each face + special edges for uv fixing
	mesh->vertexCount = 8 + 12 * subdivisions + 6 * subdivisions * subdivisions + 12 * (subdivisions + 2);
	// for each face, there are 3 indices per triangle, 2 triangles per quad, and gridSize^2 quads
	mesh->indexCount = 6 * 3 * 2 * gridSize * gridSize;
	mesh->vertices = new Vertex[mesh->vertexCount];
	struct loc {
		unsigned x, y, z, s;
		bool operator<(const loc &o) const {
			return (x < o.x || (x == o.x && (y < o.y || (y == o.y && (z < o.z || (z == o.z && s < o.s))))));
		}
	};
	map<loc, unsigned> indexRef;
	unsigned v = 0;
	float yaw = 0.f;
	float pitch = 0.f;
	unsigned w = gridSize;
	const float step = 1.f / w;
	loc p;
	// vertices
	for (unsigned i = 0; i < 6; ++i) {
		for (unsigned r = 0; r < w + 1; ++r) {
			for (unsigned c = 0; c < w + 1; ++c) {
				switch (i) {
					case 0: { p = {w, r, w - c, i}; break; } // right
					case 1: { p = {0, r, c, i}; break; } // left
					case 2: { p = {c, 0, r, i}; break; } // up
					case 3: { p = {c, w, w - r, i}; break; } // down
					case 4: { p = {c, r, w, i}; break; } // front
					case 5: { p = {w - c, r, 0, i}; break; } // back
				}
				indexRef[p] = v;
				float x = p.x * 2.f / w - 1, y = (w - p.y) * 2.f / w - 1, z = (w - p.z) * 2.f / w - 1;
				float x2 = x * x, y2 = y * y, z2 = z * z;
				mesh->vertices[v].position = XMFLOAT4(
					x * sqrt(1.f - y2 / 2.f - z2 / 2.f + y2 * z2 / 3.f),
					y * sqrt(1.f - x2 / 2.f - z2 / 2.f + x2 * z2 / 3.f),
					z * sqrt(1.f - x2 / 2.f - y2 / 2.f + x2 * y2 / 3.f),
					1.f);
				mesh->vertices[v].normal = mesh->vertices[v].position;
				mesh->vertices[v].normal.w = 0.0f;
				mesh->vertices[v].texture = XMFLOAT3(-x, y, z);
				++v;
			}
		}
	}
	// indices
	mesh->indices = new unsigned long[mesh->indexCount];
	unsigned index = 0;
	auto getIndex = [indexRef, w](unsigned i, unsigned c, unsigned r) {
		unsigned x = 0, y = 0, z = 0;
		switch (i) {
			case 0: { x = w, y = r, z = w - c; break; } // right
			case 1: { x = 0, y = r, z = c; break; } // left
			case 2: { x = c, y = 0, z = r; break;  } // up
			case 3: { x = c, y = w, z = w - r; break; } // down
			case 4: { x = c, y = r, z = w; break; } // front
			case 5: { x = w - c, y = r, z = 0; break; } // back
		}
		return indexRef.at({x, y, z, i});
	};
	for (unsigned i = 0; i < 6; ++i) {
		for (unsigned r = 0; r < w; ++r) {
			for (unsigned c = 0; c < w; ++c) {
				mesh->indices[index++] = getIndex(i, c, r);
				mesh->indices[index++] = getIndex(i, c + 1, r);
				mesh->indices[index++] = getIndex(i, c + 1, r + 1);
				mesh->indices[index++] = mesh->indices[index - 3];
				mesh->indices[index++] = mesh->indices[index - 2];
				mesh->indices[index++] = getIndex(i, c, r + 1);
			}
		}
	}

	// create the vertex and index buffers
	mesh->CreateBuffers();

	return mesh;
}