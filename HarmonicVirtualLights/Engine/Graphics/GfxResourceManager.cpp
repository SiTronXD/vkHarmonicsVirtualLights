#include "pch.h"
#include "GfxResourceManager.h"
#include "Swapchain.h"
#include "Texture.h"
#include "../Components.h"

void GfxResourceManager::init(const GfxAllocContext& gfxAllocContext)
{
	this->gfxAllocContext = &gfxAllocContext;

	this->graphicsPipelineLayout.createPipelineLayout(
		*this->gfxAllocContext->device,
		{
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT },				// Cam UBO
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },	// BRDF LUT
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },	// Prefiltered cube map
			
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },	// Albedo
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },	// Roughness
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },	// Metallic
		},
		VK_SHADER_STAGE_VERTEX_BIT,
		sizeof(PCD)
	);
}

void GfxResourceManager::cleanup()
{
	for (size_t i = 0; i < this->pipelines.size(); ++i)
		this->pipelines[i].cleanup();
	this->pipelines.clear();
	this->pipelines.shrink_to_fit();

	this->graphicsPipelineLayout.cleanup();
}

uint32_t GfxResourceManager::getMaterialPipelineIndex(const Material& material)
{
	std::string matStr = "";
	material.matToStr(matStr);

	// Check if index already exists
	if (this->materialToPipeline.count(matStr) != 0)
		return this->materialToPipeline[matStr];

	// Add new pipeline
	uint32_t newIndex = uint32_t(this->pipelines.size());
	this->pipelines.push_back(Pipeline());
	this->materialToPipeline.insert({ matStr, newIndex });

	// Initialize new pipeline
	Pipeline& newPipeline = this->pipelines[newIndex];
	newPipeline.createGraphicsPipeline(
		*this->gfxAllocContext->device, 
		this->graphicsPipelineLayout,
		Swapchain::HDR_FORMAT,
		Texture::getDepthBufferFormat(),
		"Resources/Shaders/" + std::string(material.vertexShader),
		"Resources/Shaders/" + std::string(material.fragmentShader)
	);

	return newIndex;
}
