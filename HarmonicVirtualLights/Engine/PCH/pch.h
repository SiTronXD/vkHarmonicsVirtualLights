#pragma once

#include <set>
#include <vector>
#include <array>
#include <iostream>
#include <string>
#include <algorithm>
#include <fstream>
#include <vk_mem_alloc.h>

#include "../Application/Input.h"
#include "../Application/Time.h"
#include "../Application/Window.h"
#include "../Dev/Log.h"
#include "../Graphics/Shaders/Shader.h"
#include "../Graphics/Shaders/ComputeShader.h"
#include "../Graphics/Shaders/VertexShader.h"
#include "../Graphics/Shaders/FragmentShader.h"
#include "../Graphics/Texture/TextureData.h"
#include "../Graphics/Texture/TextureDataFloat.h"
#include "../Graphics/Texture/TextureDataUchar.h"
#include "../Graphics/Vulkan/Legacy/RenderPass.h"
#include "../Graphics/Vulkan/Device.h"
#include "../Graphics/Vulkan/CommandBuffer.h"
#include "../Graphics/Vulkan/CommandBufferArray.h"
#include "../Graphics/Vulkan/CommandPool.h"
#include "../Graphics/Vulkan/DebugMessenger.h"
#include "../Graphics/Vulkan/FenceArray.h"
#include "../Graphics/Vulkan/PhysicalDevice.h"
#include "../Graphics/Vulkan/QueueFamilies.h"
#include "../Graphics/Vulkan/SemaphoreArray.h"
#include "../Graphics/Vulkan/Surface.h"
#include "../Graphics/Vulkan/VulkanInstance.h"
#include "../Graphics/Vulkan/PipelineBarrier.h"
#include "../Graphics/Vulkan/DescriptorSet.h"
#include "../Graphics/GfxAllocContext.h"
#include "../Graphics/GfxSettings.h"
#include "../Graphics/GfxState.h"
#include "../Graphics/ShaderStructs.h"
#include "../SMath.h"