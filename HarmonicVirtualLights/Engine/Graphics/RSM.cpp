#include "pch.h"
#include "RSM.h"

RSM::RSM()
	: position(1.0f),
	forwardDir(0.0f, 0.0f, 1.0f),
	projectionMatrix(1.0f),
	viewMatrix(1.0f)
{
}

void RSM::init(const GfxAllocContext& gfxAllocContext)
{
	SamplerSettings samplerSettings{};
	samplerSettings.filter = VK_FILTER_NEAREST;

	this->positionTexture.createAsRenderableSampledTexture(
		gfxAllocContext, 
		RSM::TEX_SIZE,
		RSM::TEX_SIZE,
		RSM::POSITION_FORMAT,
		(VkImageUsageFlagBits) 0,
		samplerSettings
	);
	this->normalTexture.createAsRenderableSampledTexture(
		gfxAllocContext,
		RSM::TEX_SIZE,
		RSM::TEX_SIZE,
		RSM::NORMAL_FORMAT,
		(VkImageUsageFlagBits)0,
		samplerSettings
	);
	this->brdfIndexTexture.createAsRenderableSampledTexture(
		gfxAllocContext,
		RSM::TEX_SIZE,
		RSM::TEX_SIZE,
		RSM::BRDF_INDEX_FORMAT,
		(VkImageUsageFlagBits)0,
		samplerSettings
	);
	this->depthTexture.createAsDepthSampledTexture(
		gfxAllocContext,
		RSM::TEX_SIZE,
		RSM::TEX_SIZE
	);

	this->highResShadowMapTexture.createAsDepthSampledTexture(
		gfxAllocContext,
		RSM::HIGH_RES_SHADOW_MAP_SIZE,
		RSM::HIGH_RES_SHADOW_MAP_SIZE
	);

	// Light cam ubo
	this->lightCamUbo.createDynamicCpuBuffer(
		gfxAllocContext,
		sizeof(LightCamUBO)
	);
	this->position = glm::vec3(2.0f, 2.0f, 2.0f) * 0.5f;
	this->forwardDir = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - this->position);
	this->projectionMatrix = glm::perspective(
		glm::radians(90.0f),
		1.0f,
		0.1f,
		100.0f
	);
	this->viewMatrix = glm::lookAt(
		this->position,
		this->position + this->forwardDir,
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
}

void RSM::update()
{
	// Update cam buffer
	LightCamUBO ubo{};
	ubo.vp = this->projectionMatrix * this->viewMatrix;
	ubo.pos = glm::vec4(this->position, (float) RSM::TEX_SIZE);
	ubo.dir = glm::vec4(this->forwardDir, 0.0f);
	this->lightCamUbo.updateBuffer(&ubo);
}

void RSM::cleanup()
{
	this->lightCamUbo.cleanup();

	this->highResShadowMapTexture.cleanup();

	this->depthTexture.cleanup();
	this->brdfIndexTexture.cleanup();
	this->normalTexture.cleanup();
	this->positionTexture.cleanup();
}
