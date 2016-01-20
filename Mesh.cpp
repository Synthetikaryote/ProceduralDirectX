#include "Mesh.h"
#include "Uber.h"

Mesh* GenerateCubeSphere(unsigned subdivisions = 2);

Mesh::Mesh() {
}


Mesh::~Mesh() {
	if (vertexBuffer) vertexBuffer->Release();
	if (indexBuffer) indexBuffer->Release();
}

Mesh* Mesh::LoadCubeSphere(unsigned subdivisions) {
	char keyString[40];
	sprintf_s(keyString, "MeshCubeSphere%u", subdivisions);
	size_t key = hash<string>()(string(keyString));

	return Uber::I().resourceManager->Load<Mesh>(key, [subdivisions] {
		return GenerateCubeSphere(subdivisions);
	});
}
Mesh* GenerateCubeSphere(unsigned subdivisions) {
	Mesh* mesh = new Mesh();
	// (quadrilateralized spherical cube)
	// base cube's 8 vertices + the subdivisions on each edge + the subdivisions in each face + special edges for uv fixing
	mesh->vertexCount = 8 + 12 * subdivisions + 6 * subdivisions * subdivisions + 12 * (subdivisions + 2);
	// for each face, there are 3 indices per triangle, 2 triangles per quad, and (subdivisions + 1)^2 quads
	mesh->indexCount = 6 * 3 * 2 * (subdivisions + 1) * (subdivisions + 1);
	VertexShaderInput* vertices = new VertexShaderInput[mesh->vertexCount];
	struct loc {
		unsigned x, y, z, s;
		bool operator<(const loc &o) const {
			return (x < o.x || (x == o.x && (y < o.y || (y == o.y && (z < o.z || (z == o.z && s < o.s))))));
		}
	};
	map<loc, unsigned> indexRef;
	unsigned long* indices = new unsigned long[mesh->indexCount];
	unsigned v = 0;
	float yaw = 0.f;
	float pitch = 0.f;
	unsigned w = subdivisions + 1;
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
				vertices[v].position = XMFLOAT4(
					x * sqrt(1.f - y2 / 2.f - z2 / 2.f + y2 * z2 / 3.f),
					y * sqrt(1.f - x2 / 2.f - z2 / 2.f + x2 * z2 / 3.f),
					z * sqrt(1.f - x2 / 2.f - y2 / 2.f + x2 * y2 / 3.f),
					1.f);
				vertices[v].normal = vertices[v].position;
				vertices[v].normal.w = 0.0f;
				vertices[v].texture = XMFLOAT3(-x, y, z);
				++v;
			}
		}
	}
	// indices
	unsigned index = 0;
	auto getIndex = [indexRef, w](unsigned i, unsigned c, unsigned r) {
		unsigned x = 0, y = 0, z = 0, s = 0;
		switch (i) {
			case 0: { x = w, y = r, z = w - c, s = i; break; } // right
			case 1: { x = 0, y = r, z = c; s = i; break; } // left
			case 2: { x = c, y = 0, z = r, s = i; break;  } // up
			case 3: { x = c, y = w, z = w - r, s = i; break; } // down
			case 4: { x = c, y = r, z = w; s = i; break; } // front
			case 5: { x = w - c, y = r, z = 0, s = i; break; } // back
		}
		return indexRef.at({x, y, z, s});
	};
	for (unsigned i = 0; i < 6; ++i) {
		for (unsigned r = 0; r < w; ++r) {
			for (unsigned c = 0; c < w; ++c) {
				indices[index++] = getIndex(i, c, r);
				indices[index++] = getIndex(i, c + 1, r);
				indices[index++] = getIndex(i, c + 1, r + 1);
				indices[index++] = indices[index - 3];
				indices[index++] = indices[index - 2];
				indices[index++] = getIndex(i, c, r + 1);
			}
		}
	}

	// create the vertex and index buffers
	D3D11_BUFFER_DESC vertexDesc = {sizeof(VertexShaderInput) * mesh->vertexCount, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0u, 0u, 0u};
	D3D11_SUBRESOURCE_DATA vertexData = {vertices, 0u, 0u};
	ThrowIfFailed(Uber::I().device->CreateBuffer(&vertexDesc, &vertexData, &mesh->vertexBuffer));
	D3D11_BUFFER_DESC indexDesc = {sizeof(unsigned long) * mesh->indexCount, D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER, 0u, 0u, 0u};
	D3D11_SUBRESOURCE_DATA indexData = {indices, 0u, 0u};
	ThrowIfFailed(Uber::I().device->CreateBuffer(&indexDesc, &indexData, &mesh->indexBuffer));
	delete[] vertices;
	delete[] indices;

	return mesh;
}