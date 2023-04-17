#pragma once

#include <array>
#include <vulkan/vulkan.h>

class GfxSettings
{
public:
	static const std::array<VkValidationFeatureEnableEXT, 1> validationFeatureEnables;

	static const uint32_t FRAMES_IN_FLIGHT = 3;
	static const uint32_t SWAPCHAIN_IMAGES = 3;
};