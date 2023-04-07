#include "pch.h"
#include "Renderer.h"
#include "TextureCube.h"
#include "../ResourceManager.h"
#include "Vulkan/PipelineBarrier.h"
#include "Vulkan/DescriptorSet.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

const std::vector<const char*> validationLayers =
{
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> instanceExtensions =
{
#ifdef _DEBUG
	"VK_EXT_validation_features"
#endif
};

const std::vector<const char*> deviceExtensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

void Renderer::initVulkan()
{
	// Gfx alloc context
	this->gfxAllocContext.device = &this->device;
	this->gfxAllocContext.queueFamilies = &this->queueFamilies;
	this->gfxAllocContext.singleTimeCommandPool = &this->singleTimeCommandPool;
	this->gfxAllocContext.vmaAllocator = &this->vmaAllocator;

	// Print if validation layers are enabled
	if (enableValidationLayers)
	{
		Log::write("Validation layers are enabled!");
	}

	this->instance.createInstance(
		enableValidationLayers,
		instanceExtensions,
		validationLayers, 
		this->window
	);
	this->debugMessenger.createDebugMessenger(this->instance, enableValidationLayers);
	this->surface.createSurface(this->instance, *this->window);
	this->physicalDevice.pickPhysicalDevice(
		this->instance,
		this->surface,
		deviceExtensions, 
		this->queueFamilies
	);
	this->device.createDevice(
		this->physicalDevice,
		deviceExtensions, 
		validationLayers, 
		enableValidationLayers, 
		this->queueFamilies.getIndices()
	);
	this->initVma();
	this->queueFamilies.extractQueueHandles(this->getVkDevice());
	this->swapchain.createSwapchain(this->surface, this->gfxAllocContext, *this->window, this->queueFamilies);
	
	this->commandPool.create(this->device, this->queueFamilies, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	this->singleTimeCommandPool.create(this->device, this->queueFamilies, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
	this->commandBuffers.createCommandBuffers(this->device, this->commandPool, GfxSettings::FRAMES_IN_FLIGHT);
	this->swapchain.createFramebuffers();

	// Compute pipeline
	this->postProcessPipelineLayout.createPipelineLayout(
		this->device,
		{
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT },
		},
		VK_SHADER_STAGE_COMPUTE_BIT,
		sizeof(PostProcessPCD)
	);
	this->postProcessPipeline.createComputePipeline(
		this->device, 
		this->postProcessPipelineLayout, 
		"Resources/Shaders/PostProcess.comp.spv"
	);

	this->createSyncObjects();
	this->createUniformBuffers();

	this->gfxResManager.init(this->gfxAllocContext);
	this->resourceProcessor.init(*this, *this->resourceManager, this->gfxAllocContext);

	this->brdfLutTextureIndex = this->resourceProcessor.createBrdfLut();
}

void Renderer::initVma()
{
	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.vulkanApiVersion = this->instance.getApiVersion();
	allocatorInfo.instance = this->instance.getVkInstance();
	allocatorInfo.physicalDevice = this->physicalDevice.getVkPhysicalDevice();
	allocatorInfo.device = this->device.getVkDevice();
	allocatorInfo.preferredLargeHeapBlockSize = 16 * 1024 * 1024;
	if (vmaCreateAllocator(&allocatorInfo, &this->vmaAllocator) != VK_SUCCESS)
	{
		Log::error("Failed to create VMA allocator.");
	}
}

static void checkVkResult(VkResult err)
{
	if (err == 0)
		return;

	Log::error("Vulkan error: " + std::to_string(err));
}

void Renderer::initImgui()
{
	// Vulkan objects interacting with imgui
	this->imguiPipelineLayout.createImguiPipelineLayout(this->device);
	this->imguiPipeline.createImguiPipeline(
		this->device,
		this->imguiPipelineLayout,
		this->swapchain.getHdrTexture().getVkFormat(),
		Texture::getDepthBufferFormat()
	);

	//this->imguiRenderPass.createImguiRenderPass(this->device, this->swapchain.getVkFormat());
	this->imguiDescriptorPool.createImguiDescriptorPool(this->device, GfxSettings::FRAMES_IN_FLIGHT);

	// Imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	this->imguiIO = &ImGui::GetIO(); (void)this->imguiIO;
	this->imguiIO->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	this->imguiIO->ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	this->imguiIO->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

	ImGui::StyleColorsDark();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (this->imguiIO->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	ImGui_ImplGlfw_InitForVulkan(this->window->getWindowHandle(), true);
	ImGui_ImplVulkan_InitInfo initInfo = {};
	initInfo.Instance = this->instance.getVkInstance();
	initInfo.PhysicalDevice = this->physicalDevice.getVkPhysicalDevice();
	initInfo.Device = this->getVkDevice();
	initInfo.QueueFamily = this->queueFamilies.getIndices().graphicsFamily.value();
	initInfo.Queue = this->queueFamilies.getVkGraphicsQueue();
	initInfo.PipelineCache = VK_NULL_HANDLE;
	initInfo.DescriptorPool = this->imguiDescriptorPool.getVkDescriptorPool();
	initInfo.Allocator = nullptr;
	initInfo.MinImageCount = GfxSettings::FRAMES_IN_FLIGHT;
	initInfo.ImageCount = GfxSettings::FRAMES_IN_FLIGHT; // Imgui seems to use the name "ImageCount" when the parameter is instead used as frames in flight.
	initInfo.CheckVkResultFn = checkVkResult;

	// Temporary render pass object to init imgui backend
	RenderPass tempImguiRenderPass;
	tempImguiRenderPass.createImguiRenderPass(this->device, this->swapchain.getVkFormat());

	// Init imgui vulkan backend
	ImGui_ImplVulkan_Init(&initInfo, tempImguiRenderPass.getVkRenderPass());
	tempImguiRenderPass.cleanup();

	// Upload font to gpu
	CommandBuffer tempCommandBuffer;
	tempCommandBuffer.beginSingleTimeUse(this->gfxAllocContext);
	ImGui_ImplVulkan_CreateFontsTexture(tempCommandBuffer.getVkCommandBuffer());
	tempCommandBuffer.endSingleTimeUse(this->gfxAllocContext);
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void Renderer::cleanup()
{
	this->cleanupImgui();

	this->gfxResManager.cleanup();

	this->swapchain.cleanup();

	this->uniformBuffer.cleanup();

	this->imageAvailableSemaphores.cleanup();
	this->postProcessFinishedSemaphores.cleanup();
	this->inFlightFences.cleanup();

	this->singleTimeCommandPool.cleanup();
	this->commandPool.cleanup();
	this->postProcessPipeline.cleanup();
	this->postProcessPipelineLayout.cleanup();
	
	vmaDestroyAllocator(this->vmaAllocator);
	
	this->device.cleanup();
	this->debugMessenger.cleanup();
	this->surface.cleanup();

	// Destroys both physical device and instance
	this->instance.cleanup();
}

void Renderer::createUniformBuffers()
{
	VkDeviceSize bufferSize = sizeof(UBO);
	this->uniformBuffer.createDynamicCpuBuffer(
		this->gfxAllocContext,
		bufferSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
	);
}

void Renderer::createSyncObjects()
{
	this->imageAvailableSemaphores.create(
		this->device, 
		GfxSettings::FRAMES_IN_FLIGHT
	);
	this->postProcessFinishedSemaphores.create(
		this->device,
		GfxSettings::FRAMES_IN_FLIGHT
	);
	this->inFlightFences.create(
		this->device,
		GfxSettings::FRAMES_IN_FLIGHT,
		VK_FENCE_CREATE_SIGNALED_BIT
	);
}

void Renderer::draw(Scene& scene)
{
	// Wait, then reset fence
	vkWaitForFences(
		this->getVkDevice(), 
		1, 
		&this->inFlightFences[GfxState::getFrameIndex()],
		VK_TRUE, 
		UINT64_MAX
	);

	// Get next image index from the swapchain
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(
		this->getVkDevice(),
		this->swapchain.getVkSwapchain(),
		UINT64_MAX,
		this->imageAvailableSemaphores[GfxState::getFrameIndex()],
		VK_NULL_HANDLE,
		&imageIndex
	);

	// Window resize?
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		this->resizeWindow();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		Log::error("Failed to acquire swapchain image.");
	}

	this->updateUniformBuffer(scene.getCamera());

	// Only reset the fence if we are submitting work
	this->inFlightFences.reset(GfxState::getFrameIndex());

	// Record command buffer
	this->recordCommandBuffer(imageIndex, scene);

	// Update and Render additional Platform Windows
	if (this->imguiIO->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	// Info for submitting command buffer
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };

	VkSemaphore waitSemaphores[] = { this->imageAvailableSemaphores[GfxState::getFrameIndex()] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = 
		&this->commandBuffers[GfxState::getFrameIndex()].getVkCommandBuffer();

	VkSemaphore signalSemaphores[] = { this->postProcessFinishedSemaphores[GfxState::getFrameIndex()] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	// Submit command buffer
	if (vkQueueSubmit(
		this->queueFamilies.getVkGraphicsQueue(),
		1,
		&submitInfo,
		this->inFlightFences[GfxState::getFrameIndex()])
		!= VK_SUCCESS)
	{
		Log::error("Failed to submit draw command buffer.");
	}

	// Present info
	VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { this->swapchain.getVkSwapchain() };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	// Present!
	result = vkQueuePresentKHR(this->queueFamilies.getVkPresentQueue(), &presentInfo);

	// Window resize?
	if (result == VK_ERROR_OUT_OF_DATE_KHR ||
		result == VK_SUBOPTIMAL_KHR ||
		this->framebufferResized)
	{
		this->framebufferResized = false;
		this->resizeWindow();
	}
	else if (result != VK_SUCCESS)
	{
		Log::error("Failed to present swapchain image.");
	}

	// Next frame index
	GfxState::currentFrameIndex = (GfxState::currentFrameIndex + 1) % GfxSettings::FRAMES_IN_FLIGHT;
}

void Renderer::generateMemoryDump()
{
	Log::alert("Generated memory dump called \"VmaDump.json\"");

	char* vmaDump;
	vmaBuildStatsString(this->vmaAllocator, &vmaDump, VK_TRUE);

	std::ofstream file("VmaDump.json");
	file << vmaDump << std::endl;
	file.close();

	vmaFreeStatsString(this->vmaAllocator, vmaDump);
}

void Renderer::setSkyboxTexture(uint32_t skyboxTextureIndex)
{
	this->skyboxTextureIndex = skyboxTextureIndex;
}

void Renderer::updateUniformBuffer(const Camera& camera)
{
	// Create ubo struct with matrix data
	UBO ubo{};
	ubo.vp = camera.getProjectionMatrix() * camera.getViewMatrix();
	ubo.pos = glm::vec4(camera.getPosition(), 1.0f);

	// Update buffer contents
	this->uniformBuffer.updateBuffer(&ubo);
}

void Renderer::recordCommandBuffer(
	uint32_t imageIndex, 
	Scene& scene)
{
	ImDrawData* drawData = ImGui::GetDrawData();

	CommandBuffer& commandBuffer = this->commandBuffers[GfxState::getFrameIndex()];

	// Begin
	commandBuffer.resetAndBegin();

		// Record dynamic viewport
		float swapchainHeight = (float)this->swapchain.getHeight();
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = swapchainHeight;
		viewport.width = static_cast<float>(this->swapchain.getWidth());
		viewport.height = -swapchainHeight;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		commandBuffer.setViewport(viewport);

		// Record dynamic scissor
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = this->swapchain.getVkExtent();
		commandBuffer.setScissor(scissor);

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
		commandBuffer.memoryBarrier(memoryBarriers.data(), memoryBarriers.size());

		// Clear values for color and depth
		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		// Color attachment
		VkRenderingAttachmentInfo colorAttachment { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
		colorAttachment.imageView = this->swapchain.getHdrTexture().getVkImageView();
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.clearValue = clearValues[0];

		// Depth attachment
		VkRenderingAttachmentInfo depthAttachment { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
		depthAttachment.imageView = this->swapchain.getDepthTexture().getVkImageView();
		depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.clearValue = clearValues[1];

		// Begin rendering
		VkRenderingInfo renderingInfo { VK_STRUCTURE_TYPE_RENDERING_INFO };
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
				uboInfo.range = sizeof(UBO);

				// Binding 1
				const Texture* brdfLutTexture = 
					this->resourceManager->getTexture(this->brdfLutTextureIndex);
				VkDescriptorImageInfo brdfImageInfo{};
				brdfImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				brdfImageInfo.imageView = brdfLutTexture->getVkImageView();
				brdfImageInfo.sampler = brdfLutTexture->getVkSampler();

				// Binding 2
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

				VkDescriptorImageInfo albedoImageInfo{};
				albedoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				VkDescriptorImageInfo roughnessImageInfo{};
				roughnessImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				VkDescriptorImageInfo metallicImageInfo{};
				metallicImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				std::array<VkWriteDescriptorSet, 6> writeDescriptorSets
				{
					DescriptorSet::writeBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uboInfo),
					DescriptorSet::writeImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &brdfImageInfo),
					DescriptorSet::writeImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &prefilteredImageInfo),

					DescriptorSet::writeImage(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr),
					DescriptorSet::writeImage(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr),
					DescriptorSet::writeImage(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr)
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

						// Binding 3
						const Texture* albedoTexture = 
							this->resourceManager->getTexture(material.albedoTextureId);
						albedoImageInfo.imageView = albedoTexture->getVkImageView();
						albedoImageInfo.sampler = albedoTexture->getVkSampler();

						// Binding 4
						const Texture* roughnessTexture =
							this->resourceManager->getTexture(material.roughnessTextureId);
						roughnessImageInfo.imageView = roughnessTexture->getVkImageView();
						roughnessImageInfo.sampler = roughnessTexture->getVkSampler();

						// Binding 5
						const Texture* metallicTexture =
							this->resourceManager->getTexture(material.metallicTextureId);
						metallicImageInfo.imageView = metallicTexture->getVkImageView();
						metallicImageInfo.sampler = metallicTexture->getVkSampler();

						// Push descriptor set update
						writeDescriptorSets[3].pImageInfo = &albedoImageInfo;
						writeDescriptorSets[4].pImageInfo = &roughnessImageInfo;
						writeDescriptorSets[5].pImageInfo = &metallicImageInfo;
						commandBuffer.pushDescriptorSet(
							this->gfxResManager.getGraphicsPipelineLayout(),
							0,
							writeDescriptorSets.size(),
							writeDescriptorSets.data()
						);

						// Push constant data
						PCD pushConstantData{};
						pushConstantData.modelMat = transform.modelMat;
						pushConstantData.materialProperties.x = material.roughness;
						pushConstantData.materialProperties.y = material.metallic;
						commandBuffer.pushConstant(
							this->gfxResManager.getGraphicsPipelineLayout(), 
							&pushConstantData
						);

						// Render mesh
						const Mesh& currentMesh = this->resourceManager->getMesh(meshComp.meshId);

						// Record binding vertex/index buffer
						commandBuffer.bindVertexBuffer(currentMesh.getVertexBuffer());
						commandBuffer.bindIndexBuffer(currentMesh.getIndexBuffer(), GfxState::getFrameIndex());

						// Record draw
						commandBuffer.drawIndexed(currentMesh.getNumIndices());
					}
				);
			}

		// End rendering
		commandBuffer.endRendering();

		// ---------- Imgui ----------

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
			drawData,
			commandBuffer.getVkCommandBuffer(),
			this->imguiPipeline.getVkPipeline()
		);

		// End rendering
		commandBuffer.endRendering();


		// ---------- Compute Post Process ----------

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
			computeWriteDescriptorSets.size(),
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
			(void*) &postProcessData
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

	// Stop recording
	commandBuffer.end();
}

void Renderer::resizeWindow()
{
	this->swapchain.recreate();
}

void Renderer::cleanupImgui()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	this->imguiPipelineLayout.cleanup();
	this->imguiPipeline.cleanup();
	this->imguiDescriptorPool.cleanup();
}

Renderer::Renderer()
	: window(nullptr),
	resourceManager(nullptr),
	imguiIO(nullptr),
	vmaAllocator(nullptr),
	brdfLutTextureIndex(~0u),
	skyboxTextureIndex(~0u)
{
}

Renderer::~Renderer()
{
}

void Renderer::init(ResourceManager& resourceManager)
{
	this->resourceManager = &resourceManager;

	this->initVulkan();
	this->initImgui();
}

void Renderer::initForScene(Scene& scene)
{
	this->resourceProcessor.prefilterCubeMaps();

	// Add skybox as entity to scene
	{
		// Material
		Material skyboxMaterial{};
		std::strcpy(skyboxMaterial.vertexShader, "Skybox.vert.spv");
		std::strcpy(skyboxMaterial.fragmentShader, "Skybox.frag.spv");
		skyboxMaterial.albedoTextureId = this->skyboxTextureIndex;

		// Mesh
		MeshComponent skyboxMesh{};
		skyboxMesh.meshId = this->resourceManager->addMesh("Resources/Models/invertedCube.obj");

		// Entity
		uint32_t skybox = scene.createEntity();
		scene.setComponent<MeshComponent>(skybox, skyboxMesh);
		scene.setComponent<Material>(skybox, skyboxMaterial);
	}

	// Materials
	uint32_t whiteTextureId = this->resourceManager->addTexture("Resources/Textures/white.png");
	auto tView = scene.getRegistry().view<Material>();
	tView.each([&](Material& material)
		{
			// Pipelines for materials
			material.pipelineIndex = this->gfxResManager.getMaterialPipelineIndex(material);

			// Load default material textures
			if(material.roughnessTextureId == ~0u)
				material.roughnessTextureId = whiteTextureId;
			if (material.metallicTextureId == ~0u)
				material.metallicTextureId = whiteTextureId;
		}
	);

	// Sort materials based on pipeline indices
	scene.getRegistry().sort<Material>(
		[](const auto& lhs, const auto& rhs)
		{
			return lhs.pipelineIndex < rhs.pipelineIndex;
		}
	);
}

void Renderer::setWindow(Window& window)
{
	this->window = &window;
}

void Renderer::startCleanup()
{
	// Wait for device before cleanup
	this->device.waitIdle();
}
