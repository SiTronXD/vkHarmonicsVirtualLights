#include "pch.h"
#include "ResourceProcessor.h"
#include "Renderer.h"
#include "Texture/Texture2D.h"
#include "Texture/TextureCube.h"
#include "../ResourceManager.h"
#include "Vulkan/DescriptorSet.h"

ResourceProcessor::ResourceProcessor()
	: renderer(nullptr), resourceManager(nullptr), gfxAllocContext(nullptr)
{
}

void ResourceProcessor::init(
	Renderer& renderer,
	ResourceManager& resourceManager, 
	const GfxAllocContext& gfxAllocContext)
{
	this->renderer = &renderer;
	this->resourceManager = &resourceManager;
	this->gfxAllocContext = &gfxAllocContext;
}

void ResourceProcessor::prefilterCubeMaps()
{
	const Device& device = *this->gfxAllocContext->device;

	// Prefilter compute pipeline
	PipelineLayout prefilterComputePipelineLayout;
	prefilterComputePipelineLayout.createPipelineLayout(
		device,
		{
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT } ,
		},
		VK_SHADER_STAGE_COMPUTE_BIT,
		sizeof(PrefilterPCD)
		);
	Pipeline prefilterComputePipeline;
	prefilterComputePipeline.createComputePipeline(
		device,
		prefilterComputePipelineLayout,
		"Resources/Shaders/PrefilterEnvMap.comp.spv"
	);

	PrefilterPCD prefilterPcd{};

	// Find prefilter maps
	for (size_t i = 0; i < this->resourceManager->getNumTextures(); ++i)
	{
		// Is cube map
		TextureCube* texCube = dynamic_cast<TextureCube*>(this->resourceManager->getTexture(i));
		if (texCube == nullptr)
			continue;

		// Has prefiltered map
		uint32_t prefilterIndex = texCube->getPrefilteredMapIndex();
		if (prefilterIndex != ~0u)
		{
			TextureCube* prefilterMap = static_cast<TextureCube*>(this->resourceManager->getTexture(prefilterIndex));
			if (prefilterMap->getWidth() % 16 != 0 || prefilterMap->getHeight() % 16 != 0)
			{
				Log::error("Prefiltered environment map face size is not divisible by 16.");
				continue;
			}

			CommandBuffer commandBuffer;
			commandBuffer.init(device);
			commandBuffer.beginSingleTimeUse(*this->gfxAllocContext);

			// Memory barrier before
			VkImageSubresourceRange subresourceRange{};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = prefilterMap->getMipLevels();
			subresourceRange.baseArrayLayer = 0;
			subresourceRange.layerCount = 6;
			commandBuffer.memoryBarrier(
				VK_ACCESS_NONE,
				VK_ACCESS_SHADER_WRITE_BIT,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_GENERAL,
				prefilterMap->getVkImage(),
				subresourceRange
			);

			// Pipeline
			commandBuffer.bindPipeline(prefilterComputePipeline);

			// Descriptor set
			VkDescriptorImageInfo envImageInfo{};
			envImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			envImageInfo.imageView = texCube->getVkImageView();
			envImageInfo.sampler = texCube->getVkSampler();

			VkDescriptorImageInfo prefilterImageInfo{};
			prefilterImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			prefilterImageInfo.imageView = prefilterMap->getVkImageMipView(0);

			std::array<VkWriteDescriptorSet, 2> prefilterWriteDs
			{
				DescriptorSet::writeImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &envImageInfo),
				DescriptorSet::writeImage(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &prefilterImageInfo)
			};

			uint32_t mipWidth = prefilterMap->getWidth();
			uint32_t mipHeight = prefilterMap->getHeight();

			prefilterPcd.resolution = 
				glm::uvec4(prefilterMap->getWidth(), prefilterMap->getHeight(), mipWidth, mipHeight);

			// One dispatch per mip for varying levels of roughness
			for (uint32_t mip = 0; mip < prefilterMap->getMipLevels(); ++mip)
			{
				// Output mip level
				prefilterImageInfo.imageView = prefilterMap->getVkImageMipView(mip);
				commandBuffer.pushDescriptorSet(
					prefilterComputePipelineLayout,
					0,
					prefilterWriteDs.size(),
					prefilterWriteDs.data()
				);

				// Roughness
				prefilterPcd.resolution = glm::uvec4(texCube->getWidth(), texCube->getHeight(), mipWidth, mipHeight);
				prefilterPcd.roughness.x = float(mip) / float(std::max(prefilterMap->getMipLevels(), 2u) - 1);
				commandBuffer.pushConstant(prefilterComputePipelineLayout, &prefilterPcd);

				// Dispatch
				commandBuffer.dispatch(
					(mipWidth + 16 - 1) / 16,
					(mipHeight + 16 - 1) / 16,
					6
				);

				mipWidth = std::max(mipWidth / 2, 1u);
				mipHeight = std::max(mipHeight / 2, 1u);
			}

			// Memory barrier after
			commandBuffer.memoryBarrier(
				VK_ACCESS_SHADER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				prefilterMap->getVkImage(),
				subresourceRange
			);

			// End
			commandBuffer.endSingleTimeUse(*this->gfxAllocContext);
		}
	}

	// Cleanup
	this->renderer->startCleanup();
	prefilterComputePipeline.cleanup();
	prefilterComputePipelineLayout.cleanup();
}

uint32_t ResourceProcessor::createBrdfLut()
{
	const Device& device = *this->gfxAllocContext->device;

	// Sampler settings
	SamplerSettings brdfSamplerSettings{};
	brdfSamplerSettings.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	// Create BRDF LUT texture
	uint32_t brdfLutTextureIndex = this->resourceManager->addEmptyTexture();
	Texture2D* brdfLutTexture = 
		static_cast<Texture2D*>(this->resourceManager->getTexture(brdfLutTextureIndex));
	brdfLutTexture->createAsRenderableSampledTexture(
		*this->gfxAllocContext,
		512,
		512,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_IMAGE_USAGE_STORAGE_BIT,
		brdfSamplerSettings
	);

	// BRDF compute pipeline
	PipelineLayout brdfComputePipelineLayout;
	brdfComputePipelineLayout.createPipelineLayout(
		device,
		{ { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT } }
	);
	Pipeline brdfComputePipeline;
	brdfComputePipeline.createComputePipeline(
		device,
		brdfComputePipelineLayout,
		"Resources/Shaders/BrdfLut.comp.spv"
	);

	// Start compute shader
	{
		CommandBuffer commandBuffer;
		commandBuffer.init(device);
		commandBuffer.beginSingleTimeUse(*this->gfxAllocContext);

		// Memory barrier before
		commandBuffer.memoryBarrier(
			VK_ACCESS_NONE,
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			brdfLutTexture->getVkImage(),
			VK_IMAGE_ASPECT_COLOR_BIT
		);

		// Pipeline
		commandBuffer.bindPipeline(brdfComputePipeline);

		// Descriptor set
		VkDescriptorImageInfo pbrImageInfo{};
		pbrImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		pbrImageInfo.imageView = brdfLutTexture->getVkImageView();
		std::array<VkWriteDescriptorSet, 1> pbrLutWriteDs
		{
			DescriptorSet::writeImage(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &pbrImageInfo)
		};
		commandBuffer.pushDescriptorSet(
			brdfComputePipelineLayout,
			0,
			pbrLutWriteDs.size(),
			pbrLutWriteDs.data()
		);

		// Dispatch
		commandBuffer.dispatch(
			512 / 16,
			512 / 16
		);

		// Memory barrier after
		commandBuffer.memoryBarrier(
			VK_ACCESS_SHADER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			brdfLutTexture->getVkImage(),
			VK_IMAGE_ASPECT_COLOR_BIT
		);

		// End
		commandBuffer.endSingleTimeUse(*this->gfxAllocContext);
	}

	// Cleanup
	this->renderer->startCleanup();
	brdfComputePipeline.cleanup();
	brdfComputePipelineLayout.cleanup();

	return brdfLutTextureIndex;
}