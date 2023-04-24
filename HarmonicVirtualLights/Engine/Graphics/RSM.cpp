#include "pch.h"
#include "RSM.h"

RSM::RSM()
	: position(1.0f),
	projectionMatrix(1.0f),
	viewMatrix(1.0f)
{
}

void RSM::init(const GfxAllocContext& gfxAllocContext)
{
	this->positionTexture.createAsRenderableSampledTexture(
		gfxAllocContext, 
		RSM::TEX_SIZE,
		RSM::TEX_SIZE,
		RSM::POSITION_FORMAT
	);
	this->normalTexture.createAsRenderableSampledTexture(
		gfxAllocContext,
		RSM::TEX_SIZE,
		RSM::TEX_SIZE,
		RSM::NORMAL_FORMAT
	);
	this->brdfIndexTexture.createAsRenderableSampledTexture(
		gfxAllocContext,
		RSM::TEX_SIZE,
		RSM::TEX_SIZE,
		RSM::BRDF_INDEX_FORMAT
	);
	/*this->emissionFunctionTexture.createAsRenderableSampledTexture(
		gfxAllocContext,
		this->width,
		this->height,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		(VkImageUsageFlagBits)0
	);*/
	this->depthTexture.createAsDepthSampledTexture(
		gfxAllocContext,
		RSM::TEX_SIZE,
		RSM::TEX_SIZE
	);

	// Cam ubo
	this->camUbo.createDynamicCpuBuffer(
		gfxAllocContext,
		sizeof(CamUBO)
	);
	this->position = glm::vec3(2.0f, 2.0f, 2.0f);
	this->projectionMatrix = glm::perspective(
		glm::radians(90.0f),
		1.0f,
		0.1f,
		100.0f
	);
	this->viewMatrix = glm::lookAt(
		this->position,
		glm::vec3(0.0f, 0.0f, 0.0f),//this->position + this->forwardDir,
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
}

void RSM::update()
{
	// Update cam buffer
	CamUBO ubo{};
	ubo.vp = this->projectionMatrix * this->viewMatrix;
	ubo.pos = glm::vec4(this->position, (float) RSM::TEX_SIZE);
	this->camUbo.updateBuffer(&ubo);
}

void RSM::cleanup()
{
	this->camUbo.cleanup();

	this->depthTexture.cleanup();
	//this->emissionFunctionTexture.cleanup();
	this->brdfIndexTexture.cleanup();
	this->normalTexture.cleanup();
	this->positionTexture.cleanup();
}
