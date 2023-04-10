#pragma once

#include "Buffer.h"

class IndexBuffer : public Buffer
{
private:
public:
	IndexBuffer();
	~IndexBuffer();

	void createIndexBuffer(
		const GfxAllocContext& gfxAllocContext,
		const std::vector<uint32_t>& indices);
};