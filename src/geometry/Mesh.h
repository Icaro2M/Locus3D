#pragma once

#include <vector>

#include "graphics/buffers/VertexArray.h"
#include "graphics/buffers/VertexBuffer.h"
#include "graphics/buffers/IndexBuffer.h"

class Mesh
{
private:
	VertexArray m_VAO;
	VertexBuffer* m_VBO;
	IndexBuffer* m_IBO;

	std::vector<float> m_Vertices;
	std::vector<unsigned int> m_Indices;

public:
	Mesh(
		const float* vertices,
		unsigned int vertexBufferSize,
		const unsigned int* indices,
		unsigned int indexCount
	);

	~Mesh();

	void bind() const;
	void unbind() const;

	const IndexBuffer& getIndexBuffer() const;

	const std::vector<float>& getVertices() const;
	const std::vector<unsigned int>& getIndices() const;
};