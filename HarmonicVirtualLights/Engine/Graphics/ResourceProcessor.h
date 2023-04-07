#pragma once

#include "GfxAllocContext.h"

class Renderer;
class ResourceManager;

class ResourceProcessor
{
private:
	Renderer* renderer;
	ResourceManager* resourceManager;

	const GfxAllocContext* gfxAllocContext;

public:
	ResourceProcessor();

	void init(
		Renderer& renderer,
		ResourceManager& resourceManager, 
		const GfxAllocContext& gfxAllocContext);

	void prefilterCubeMaps();

	uint32_t createBrdfLut();
};