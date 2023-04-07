#pragma once

#include <unordered_map>

struct Material;

class GfxResourceManager
{
private:
	std::unordered_map<std::string, uint32_t> materialToPipeline;
	std::vector<Pipeline> pipelines;

	PipelineLayout graphicsPipelineLayout;

	const GfxAllocContext* gfxAllocContext;

public:
	void init(const GfxAllocContext& gfxAllocContext);
	void cleanup();

	uint32_t getMaterialPipelineIndex(const Material& material);

	inline const PipelineLayout& getGraphicsPipelineLayout() const { return this->graphicsPipelineLayout; }
	inline const Pipeline& getPipeline(uint32_t index) const { return this->pipelines[index]; }
};