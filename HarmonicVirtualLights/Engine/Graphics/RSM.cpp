#include "pch.h"
#include "RSM.h"

RSM::RSM()
	: width(0), height(0),
	position(1.0f),
	projectionMatrix(1.0f),
	viewMatrix(1.0f)
{
}

void RSM::init(const GfxAllocContext& gfxAllocContext)
{
	this->width = 256;
	this->height = 256;

	this->positionTexture.createAsRenderableSampledTexture(
		gfxAllocContext, 
		this->width,
		this->height,
		RSM::POSITION_FORMAT,
		(VkImageUsageFlagBits) 0
	);
	this->normalTexture.createAsRenderableSampledTexture(
		gfxAllocContext,
		this->width,
		this->height,
		RSM::NORMAL_FORMAT,
		(VkImageUsageFlagBits)0
	);
	/*this->brdfIndexTexture.createAsRenderableSampledTexture(
		gfxAllocContext,
		this->width,
		this->height,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		(VkImageUsageFlagBits)0
	);
	this->emissionFunctionTexture.createAsRenderableSampledTexture(
		gfxAllocContext,
		this->width,
		this->height,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		(VkImageUsageFlagBits)0
	);*/
	this->depthTexture.createAsDepthTexture(
		gfxAllocContext,
		this->width,
		this->height
	);

	// Cam ubo
	this->camUbo.createDynamicCpuBuffer(
		gfxAllocContext,
		sizeof(CamUBO),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
	);
	this->position = glm::vec3(2.0f, 2.0f, 2.0f);
	this->projectionMatrix = glm::perspective(
		glm::radians(90.0f),
		(float) this->width / (float) this->height,
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
	ubo.pos = glm::vec4(this->position, 1.0f);
	this->camUbo.updateBuffer(&ubo);
}

void RSM::cleanup()
{
	this->camUbo.cleanup();

	this->depthTexture.cleanup();
	//this->emissionFunctionTexture.cleanup();
	//this->brdfIndexTexture.cleanup();
	this->normalTexture.cleanup();
	this->positionTexture.cleanup();
}
