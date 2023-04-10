#pragma once

#include <string>
#include "Buffer/VertexBuffer.h"

class MeshData
{
private:
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

public:
	MeshData();
	~MeshData();

	void create(
		std::vector<Vertex>& vertices, 
		std::vector<uint32_t>& indices);

	bool loadOBJ(const std::string& filePath);

	inline std::vector<Vertex>& getVertices() { return this->vertices; }
	inline std::vector<uint32_t>& getIndices() { return this->indices; }
};