#include "pch.h"
#include "Renderer.h"
#include "../ResourceManager.h"
#include "Texture/TextureCube.h"

void Renderer::renderRSM(CommandBuffer& commandBuffer, Scene& scene)
{
	VkExtent2D rsmExtent{ RSM::TEX_SIZE, RSM::TEX_SIZE };

	// Viewport
	float swapchainHeight = (float)rsmExtent.height;
	VkViewport rsmViewport{};
	rsmViewport.x = 0.0f;
	rsmViewport.y = swapchainHeight;
	rsmViewport.width = static_cast<float>(rsmExtent.width);
	rsmViewport.height = -swapchainHeight;
	rsmViewport.minDepth = 0.0f;
	rsmViewport.maxDepth = 1.0f;
	commandBuffer.setViewport(rsmViewport);

	// Scissor
	VkRect2D rsmScissor{};
	rsmScissor.offset = { 0, 0 };
	rsmScissor.extent = rsmExtent;
	commandBuffer.setScissor(rsmScissor);

	// Transition layouts for render targets
	std::array<VkImageMemoryBarrier2, 4> memoryBarriers =
	{
		// Position
		PipelineBarrier::imageMemoryBarrier2(
			VK_ACCESS_NONE,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			this->rsm.getPositionTexture().getVkImage(),
			VK_IMAGE_ASPECT_COLOR_BIT
		),

		// Normal
		PipelineBarrier::imageMemoryBarrier2(
			VK_ACCESS_NONE,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			this->rsm.getNormalTexture().getVkImage(),
			VK_IMAGE_ASPECT_COLOR_BIT
		),

		// Brdf index
		PipelineBarrier::imageMemoryBarrier2(
			VK_ACCESS_NONE,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			this->rsm.getBrdfIndexTexture().getVkImage(),
			VK_IMAGE_ASPECT_COLOR_BIT
		),

		// Depth
		PipelineBarrier::imageMemoryBarrier2(
			VK_ACCESS_NONE,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, // Stage from "previous frame"
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,	// Stage from "current frame"
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			this->rsm.getDepthTexture().getVkImage(),
			VK_IMAGE_ASPECT_DEPTH_BIT
		)
	};
	commandBuffer.memoryBarrier(memoryBarriers.data(), uint32_t(memoryBarriers.size()));

	// Clear values for color and depth
	std::array<VkClearValue, 4> clearValues{};
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[1].color = { { 64.0f, 64.0f, 64.0f, 1.0f } };
	clearValues[2].color.uint32[0] = 0u; clearValues[2].color.uint32[1] = 0u; clearValues[2].color.uint32[2] = 0u; clearValues[2].color.uint32[3] = 0u;
	clearValues[3].depthStencil = { 1.0f, 0 };

	// Color attachments
	std::array<VkRenderingAttachmentInfo, 3> colorAttachments{};
	colorAttachments[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachments[0].imageView = this->rsm.getPositionTexture().getVkImageView();
	colorAttachments[0].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachments[0].clearValue = clearValues[0];

	colorAttachments[1].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachments[1].imageView = this->rsm.getNormalTexture().getVkImageView();
	colorAttachments[1].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachments[1].clearValue = clearValues[1];

	colorAttachments[2].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachments[2].imageView = this->rsm.getBrdfIndexTexture().getVkImageView();
	colorAttachments[2].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachments[2].clearValue = clearValues[2];

	// Depth attachment
	VkRenderingAttachmentInfo depthAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	depthAttachment.imageView = this->rsm.getDepthTexture().getVkImageView();
	depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.clearValue = clearValues[3];

	// Begin rendering
	VkRenderingInfo renderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO };
	renderingInfo.renderArea = rsmScissor;
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = uint32_t(colorAttachments.size());
	renderingInfo.pColorAttachments = colorAttachments.data();
	renderingInfo.pDepthAttachment = &depthAttachment;
	commandBuffer.beginRendering(renderingInfo);

	{
		uint32_t currentPipelineIndex = ~0u;
		uint32_t numPipelineSwitches = 0;

		// Common descriptor set bindings

		// Binding 0
		VkDescriptorBufferInfo rsmUboInfo{};
		rsmUboInfo.buffer = this->rsm.getLightCamUbo().getVkBuffer(GfxState::getFrameIndex());
		rsmUboInfo.range = this->rsm.getLightCamUbo().getBufferSize();

		std::array<VkWriteDescriptorSet, 1> writeDescriptorSets
		{
			DescriptorSet::writeBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &rsmUboInfo)
		};

		// Loop through entities with mesh components
		auto view = scene.getRegistry().view<Material, MeshComponent, Transform>();
		view.each([&](
			const Material& material,
			const MeshComponent& meshComp,
			const Transform& transform)
			{
				if (material.castShadows)
				{
					// Switch pipeline if necessary
					if (currentPipelineIndex != material.rsmPipelineIndex)
					{
						commandBuffer.bindPipeline(
							this->gfxResManager.getPipeline(material.rsmPipelineIndex)
						);

						currentPipelineIndex = material.rsmPipelineIndex;
						numPipelineSwitches++;
					}

					// Push descriptor set update
					commandBuffer.pushDescriptorSet(
						this->gfxResManager.getGraphicsPipelineLayout(), // TODO: fix with shader reflection
						0,
						uint32_t(writeDescriptorSets.size()),
						writeDescriptorSets.data()
					);

					// Push constant data
					PCD pushConstantData{};
					pushConstantData.modelMat = transform.modelMat;
					pushConstantData.brdfProperties.x = material.brdfId;
					commandBuffer.pushConstant(
						this->gfxResManager.getGraphicsPipelineLayout(), // TODO: fix with shader reflection
						&pushConstantData
					);

					// Render mesh
					const Mesh& currentMesh = this->resourceManager->getMesh(meshComp.meshId);

					// Record binding vertex/index buffer
					commandBuffer.bindVertexBuffer(currentMesh.getVertexBuffer());
					commandBuffer.bindIndexBuffer(currentMesh.getIndexBuffer());

					// Record draw
					commandBuffer.drawIndexed(currentMesh.getNumIndices());
				}
			}
		);
	}

	// End rendering
	commandBuffer.endRendering();

	// Transition layouts for RSM render targets
	std::array<VkImageMemoryBarrier2, 3> rsmEndMemoryBarriers =
	{
		// Position
		PipelineBarrier::imageMemoryBarrier2(
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			this->rsm.getPositionTexture().getVkImage(),
			VK_IMAGE_ASPECT_COLOR_BIT
		),

		// Normal
		PipelineBarrier::imageMemoryBarrier2(
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			this->rsm.getNormalTexture().getVkImage(),
			VK_IMAGE_ASPECT_COLOR_BIT
		),

		// Brdf index
		PipelineBarrier::imageMemoryBarrier2(
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			this->rsm.getBrdfIndexTexture().getVkImage(),
			VK_IMAGE_ASPECT_COLOR_BIT
		)
	};
	commandBuffer.memoryBarrier(rsmEndMemoryBarriers.data(), uint32_t(rsmEndMemoryBarriers.size()));
}

void Renderer::renderShadowMap(CommandBuffer& commandBuffer, Scene& scene)
{
	// Bind dummy pipeline before begin rendering
	commandBuffer.bindPipeline(this->gfxResManager.getOneShadowMapPipeline());

	VkExtent2D shadowMapExtent{ RSM::HIGH_RES_SHADOW_MAP_SIZE, RSM::HIGH_RES_SHADOW_MAP_SIZE };

	// Viewport
	float targetHeight = (float)shadowMapExtent.height;
	VkViewport shadowMapViewport{};
	shadowMapViewport.x = 0.0f;
	shadowMapViewport.y = targetHeight;
	shadowMapViewport.width = static_cast<float>(shadowMapExtent.width);
	shadowMapViewport.height = -targetHeight;
	shadowMapViewport.minDepth = 0.0f;
	shadowMapViewport.maxDepth = 1.0f;
	commandBuffer.setViewport(shadowMapViewport);

	// Scissor
	VkRect2D shadowMapScissor{};
	shadowMapScissor.offset = { 0, 0 };
	shadowMapScissor.extent = shadowMapExtent;
	commandBuffer.setScissor(shadowMapScissor);

	// Transition layout
	commandBuffer.memoryBarrier(
		VK_ACCESS_NONE,
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, // Stage from "previous frame"
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,	// Stage from "current frame"
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		this->rsm.getHighResShadowMapTexture().getVkImage(),
		VK_IMAGE_ASPECT_DEPTH_BIT
	);

	// Clear values for color and depth
	std::array<VkClearValue, 1> clearValues{};
	clearValues[0].depthStencil = { 1.0f, 0 };

	// Depth attachment
	VkRenderingAttachmentInfo depthAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	depthAttachment.imageView = this->rsm.getHighResShadowMapTexture().getVkImageView();
	depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.clearValue = clearValues[0];

	// Begin rendering
	VkRenderingInfo renderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO };
	renderingInfo.renderArea = shadowMapScissor;
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = uint32_t(0);
	renderingInfo.pDepthAttachment = &depthAttachment;
	commandBuffer.beginRendering(renderingInfo);

	{
		uint32_t currentPipelineIndex = ~0u;
		uint32_t numPipelineSwitches = 0;

		// Common descriptor set bindings

		// Binding 0
		VkDescriptorBufferInfo shadowMapUboInfo{};
		shadowMapUboInfo.buffer = this->rsm.getLightCamUbo().getVkBuffer(GfxState::getFrameIndex());
		shadowMapUboInfo.range = this->rsm.getLightCamUbo().getBufferSize();

		std::array<VkWriteDescriptorSet, 1> writeDescriptorSets
		{
			DescriptorSet::writeBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &shadowMapUboInfo)
		};

		// Loop through entities with mesh components
		auto view = scene.getRegistry().view<Material, MeshComponent, Transform>();
		view.each([&](
			const Material& material,
			const MeshComponent& meshComp,
			const Transform& transform)
			{
				if (material.castShadows)
				{
					// Switch pipeline if necessary
					if (currentPipelineIndex != material.shadowMapPipelineIndex)
					{
						commandBuffer.bindPipeline(
							this->gfxResManager.getPipeline(material.shadowMapPipelineIndex)
						);

						currentPipelineIndex = material.shadowMapPipelineIndex;
						numPipelineSwitches++;
					}

					// Push descriptor set update
					commandBuffer.pushDescriptorSet(
						this->gfxResManager.getGraphicsPipelineLayout(), // TODO: fix with shader reflection
						0,
						uint32_t(writeDescriptorSets.size()),
						writeDescriptorSets.data()
					);

					// Push constant data
					PCD pushConstantData{};
					pushConstantData.modelMat = transform.modelMat;
					commandBuffer.pushConstant(
						this->gfxResManager.getGraphicsPipelineLayout(), // TODO: fix with shader reflection
						&pushConstantData
					);

					// Render mesh
					const Mesh& currentMesh = this->resourceManager->getMesh(meshComp.meshId);

					// Record binding vertex/index buffer
					commandBuffer.bindVertexBuffer(currentMesh.getVertexBuffer());
					commandBuffer.bindIndexBuffer(currentMesh.getIndexBuffer());

					// Record draw
					commandBuffer.drawIndexed(currentMesh.getNumIndices());
				}
			}
		);
	}

	// End rendering
	commandBuffer.endRendering();

	// Transition layout
	commandBuffer.memoryBarrier(
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		this->rsm.getHighResShadowMapTexture().getVkImage(),
		VK_IMAGE_ASPECT_DEPTH_BIT
	);
}

void Renderer::renderScene(CommandBuffer& commandBuffer, Scene& scene)
{
	// Dynamic viewport
	float swapchainHeight = (float)this->swapchain.getHeight();
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = swapchainHeight;
	viewport.width = static_cast<float>(this->swapchain.getWidth());
	viewport.height = -swapchainHeight;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	commandBuffer.setViewport(viewport);

	// Dynamic scissor
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = this->swapchain.getVkExtent();
	commandBuffer.setScissor(scissor);

	// Bind dummy pipeline before begin rendering
	commandBuffer.bindPipeline(this->gfxResManager.getOneHdrPipeline());

	// Render to HDR buffer
	
	// Transition layouts for color and depth
	std::array<VkImageMemoryBarrier2, 2> memoryBarriers =
	{
		// HDR
		PipelineBarrier::imageMemoryBarrier2(
			VK_ACCESS_NONE,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			this->swapchain.getHdrTexture().getVkImage(),
			VK_IMAGE_ASPECT_COLOR_BIT
		),

		// Depth
		PipelineBarrier::imageMemoryBarrier2(
			VK_ACCESS_NONE,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, // Stage from "previous frame"
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,	// Stage from "current frame"
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			this->swapchain.getDepthTexture().getVkImage(),
			VK_IMAGE_ASPECT_DEPTH_BIT
		)
	};
	commandBuffer.memoryBarrier(memoryBarriers.data(), uint32_t(memoryBarriers.size()));

	// Clear values for color and depth
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };

	// Color attachment
	VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	colorAttachment.imageView = this->swapchain.getHdrTexture().getVkImageView();
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.clearValue = clearValues[0];

	// Depth attachment
	VkRenderingAttachmentInfo depthAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	depthAttachment.imageView = this->swapchain.getDepthTexture().getVkImageView();
	depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.clearValue = clearValues[1];

	// Begin rendering
	VkRenderingInfo renderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO };
	renderingInfo.renderArea = { { 0, 0 }, this->swapchain.getVkExtent() };
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachment;
	renderingInfo.pDepthAttachment = &depthAttachment;
	commandBuffer.beginRendering(renderingInfo);

	{
		uint32_t currentPipelineIndex = ~0u;
		uint32_t numPipelineSwitches = 0;

		// Common descriptor set bindings

		// Binding 0
		VkDescriptorBufferInfo uboInfo{};
		uboInfo.buffer = this->uniformBuffer.getVkBuffer(GfxState::getFrameIndex());
		uboInfo.range = this->uniformBuffer.getBufferSize();

		// Binding 1
		VkDescriptorBufferInfo lightCamUboInfo{};
		lightCamUboInfo.buffer = this->rsm.getLightCamUbo().getVkBuffer(GfxState::getFrameIndex());
		lightCamUboInfo.range = this->rsm.getLightCamUbo().getBufferSize();

		// Binding 2
		const Texture* brdfLutTexture =
			this->resourceManager->getTexture(this->brdfLutTextureIndex);
		VkDescriptorImageInfo brdfImageInfo{};
		brdfImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		brdfImageInfo.imageView = brdfLutTexture->getVkImageView();
		brdfImageInfo.sampler = brdfLutTexture->getVkSampler();

		// Binding 3
		const Texture* prefilteredEnvMap =
			this->resourceManager->getTexture(
				static_cast<TextureCube*>(
					this->resourceManager->getTexture(this->skyboxTextureIndex)
					)->getPrefilteredMapIndex()
			);
		VkDescriptorImageInfo prefilteredImageInfo{};
		prefilteredImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		prefilteredImageInfo.imageView = prefilteredEnvMap->getVkImageView();
		prefilteredImageInfo.sampler = prefilteredEnvMap->getVkSampler();

		// Binding 4
		VkDescriptorBufferInfo shCoeffSboInfo{};
		shCoeffSboInfo.buffer = this->shCoefficientBuffer.getVkBuffer();
		shCoeffSboInfo.range = this->shCoefficientBuffer.getBufferSize();

		// Binding 5
		const Texture& rsmDepthTex = this->rsm.getHighResShadowMapTexture();
		VkDescriptorImageInfo rsmDepthImageInfo{};
		rsmDepthImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		rsmDepthImageInfo.imageView = rsmDepthTex.getVkImageView();
		rsmDepthImageInfo.sampler = rsmDepthTex.getVkSampler();

		// Binding 6
		const Texture& rsmPositionTex = this->rsm.getPositionTexture();
		VkDescriptorImageInfo rsmPositionImageInfo{};
		rsmPositionImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		rsmPositionImageInfo.imageView = rsmPositionTex.getVkImageView();
		rsmPositionImageInfo.sampler = rsmPositionTex.getVkSampler();

		// Binding 7
		const Texture& rsmNormalTex = this->rsm.getNormalTexture();
		VkDescriptorImageInfo rsmNormalImageInfo{};
		rsmNormalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		rsmNormalImageInfo.imageView = rsmNormalTex.getVkImageView();
		rsmNormalImageInfo.sampler = rsmNormalTex.getVkSampler();

		// Binding 8
		const Texture& rsmBRDFTex = this->rsm.getBrdfIndexTexture();
		VkDescriptorImageInfo rsmBRDFImageInfo{};
		rsmBRDFImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		rsmBRDFImageInfo.imageView = rsmBRDFTex.getVkImageView();
		rsmBRDFImageInfo.sampler = rsmBRDFTex.getVkSampler();

		VkDescriptorImageInfo albedoImageInfo{};
		albedoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VkDescriptorImageInfo roughnessImageInfo{};
		roughnessImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		VkDescriptorImageInfo metallicImageInfo{};
		metallicImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		std::array<VkWriteDescriptorSet, 12> writeDescriptorSets
		{
			DescriptorSet::writeBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uboInfo),
			DescriptorSet::writeBuffer(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &lightCamUboInfo),

			DescriptorSet::writeImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &brdfImageInfo),
			DescriptorSet::writeImage(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &prefilteredImageInfo),

			DescriptorSet::writeBuffer(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &shCoeffSboInfo),

			DescriptorSet::writeImage(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &rsmDepthImageInfo),
			DescriptorSet::writeImage(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &rsmPositionImageInfo),
			DescriptorSet::writeImage(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &rsmNormalImageInfo),
			DescriptorSet::writeImage(8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &rsmBRDFImageInfo),

			DescriptorSet::writeImage(9, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr),
			DescriptorSet::writeImage(10, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr),
			DescriptorSet::writeImage(11, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr)
		};

		// Loop through entities with mesh components
		auto view = scene.getRegistry().view<Material, MeshComponent, Transform>();
		view.each([&](
			const Material& material,
			const MeshComponent& meshComp,
			const Transform& transform)
			{
				// Switch pipeline if necessary
				if (currentPipelineIndex != material.pipelineIndex)
				{
					commandBuffer.bindPipeline(
						this->gfxResManager.getPipeline(material.pipelineIndex)
					);

					currentPipelineIndex = material.pipelineIndex;
					numPipelineSwitches++;
				}

		// Binding 9
		const Texture* albedoTexture =
			this->resourceManager->getTexture(material.albedoTextureId);
		albedoImageInfo.imageView = albedoTexture->getVkImageView();
		albedoImageInfo.sampler = albedoTexture->getVkSampler();

		// Binding 10
		const Texture* roughnessTexture =
			this->resourceManager->getTexture(material.roughnessTextureId);
		roughnessImageInfo.imageView = roughnessTexture->getVkImageView();
		roughnessImageInfo.sampler = roughnessTexture->getVkSampler();

		// Binding 11
		const Texture* metallicTexture =
			this->resourceManager->getTexture(material.metallicTextureId);
		metallicImageInfo.imageView = metallicTexture->getVkImageView();
		metallicImageInfo.sampler = metallicTexture->getVkSampler();

		// Push descriptor set update
		writeDescriptorSets[9].pImageInfo = &albedoImageInfo;
		writeDescriptorSets[10].pImageInfo = &roughnessImageInfo;
		writeDescriptorSets[11].pImageInfo = &metallicImageInfo;
		commandBuffer.pushDescriptorSet(
			this->gfxResManager.getGraphicsPipelineLayout(),
			0,
			uint32_t(writeDescriptorSets.size()),
			writeDescriptorSets.data()
		);

		// Push constant data
		PCD pushConstantData{};
		pushConstantData.modelMat = transform.modelMat;
		pushConstantData.materialProperties.x = material.roughness;
		pushConstantData.materialProperties.y = material.metallic;
		pushConstantData.brdfProperties.x = material.brdfId;
		commandBuffer.pushConstant(
			this->gfxResManager.getGraphicsPipelineLayout(),
			&pushConstantData
		);

		// Render mesh
		const Mesh& currentMesh = this->resourceManager->getMesh(meshComp.meshId);

		// Record binding vertex/index buffer
		commandBuffer.bindVertexBuffer(currentMesh.getVertexBuffer());
		commandBuffer.bindIndexBuffer(currentMesh.getIndexBuffer());

		// Record draw
		commandBuffer.drawIndexed(currentMesh.getNumIndices());
			}
		);
	}

	// End rendering
	commandBuffer.endRendering();
}

void Renderer::renderImgui(CommandBuffer& commandBuffer, ImDrawData* imguiDrawData)
{
	// Imgui using dynamic rendering
	VkRenderingAttachmentInfo imguiColorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	imguiColorAttachment.imageView = this->swapchain.getHdrTexture().getVkImageView();
	imguiColorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	imguiColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	imguiColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	VkRenderingInfo imguiRenderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO };
	imguiRenderingInfo.renderArea = { { 0, 0 }, this->swapchain.getVkExtent() };
	imguiRenderingInfo.layerCount = 1;
	imguiRenderingInfo.colorAttachmentCount = 1;
	imguiRenderingInfo.pColorAttachments = &imguiColorAttachment;

	// Bind pipeline before begin rendering because of formats
	commandBuffer.bindPipeline(this->imguiPipeline);

	// Begin rendering for imgui
	commandBuffer.beginRendering(imguiRenderingInfo);

	// Record imgui primitives into it's command buffer
	ImGui_ImplVulkan_RenderDrawData(
		imguiDrawData,
		commandBuffer.getVkCommandBuffer(),
		this->imguiPipeline.getVkPipeline()
	);

	// End rendering
	commandBuffer.endRendering();
}

void Renderer::computePostProcess(CommandBuffer& commandBuffer, uint32_t imageIndex)
{
	// Transition HDR and swapchain image
	VkImageMemoryBarrier2 postProcessMemoryBarriers[2] =
	{
		// HDR
		PipelineBarrier::imageMemoryBarrier2(
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_GENERAL,
			this->swapchain.getHdrTexture().getVkImage(),
			VK_IMAGE_ASPECT_COLOR_BIT
		),

		// Swapchain image
		PipelineBarrier::imageMemoryBarrier2(
			VK_ACCESS_NONE,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			this->swapchain.getVkImage(imageIndex),
			VK_IMAGE_ASPECT_COLOR_BIT
		)
	};
	commandBuffer.memoryBarrier(postProcessMemoryBarriers, 2);

	// Compute pipeline
	commandBuffer.bindPipeline(this->postProcessPipeline);

	// Binding 0
	VkDescriptorImageInfo inputImageInfo{};
	inputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	inputImageInfo.imageView = this->swapchain.getHdrTexture().getVkImageView();

	// Binding 1
	VkDescriptorImageInfo outputImageInfo{};
	outputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	outputImageInfo.imageView = this->swapchain.getVkImageView(imageIndex);

	// Descriptor set
	std::array<VkWriteDescriptorSet, 2> computeWriteDescriptorSets
	{
		DescriptorSet::writeImage(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &inputImageInfo),
		DescriptorSet::writeImage(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &outputImageInfo)
	};
	commandBuffer.pushDescriptorSet(
		this->postProcessPipelineLayout,
		0,
		uint32_t(computeWriteDescriptorSets.size()),
		computeWriteDescriptorSets.data()
	);

	// Push constant
	PostProcessPCD postProcessData{};
	postProcessData.resolution =
		glm::uvec4(
			this->swapchain.getVkExtent().width,
			this->swapchain.getVkExtent().height,
			0u,
			0u
		);
	commandBuffer.pushConstant(
		this->postProcessPipelineLayout,
		(void*)&postProcessData
	);

	// Run compute shader
	commandBuffer.dispatch(
		(postProcessData.resolution.x + 16 - 1) / 16,
		(postProcessData.resolution.y + 16 - 1) / 16
	);

	// Transition swapchain image layout for presentation
	commandBuffer.memoryBarrier(
		VK_ACCESS_SHADER_WRITE_BIT,
		VK_ACCESS_NONE,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		this->swapchain.getVkImage(imageIndex),
		VK_IMAGE_ASPECT_COLOR_BIT
	);
}