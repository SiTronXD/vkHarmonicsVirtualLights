#pragma once

#include <entt/entt.hpp>

struct Transform
{
	glm::mat4 modelMat = glm::mat4(1.0f);
};

struct MeshComponent
{
	uint32_t meshId = ~0u;
};

#define SHADER_NAME_CHAR_SIZE 32
struct Material
{
	char vertexShader[SHADER_NAME_CHAR_SIZE]{};
	char fragmentShader[SHADER_NAME_CHAR_SIZE]{};

	float roughness = 1.0f;
	float metallic = 1.0f;

	uint32_t albedoTextureId = ~0u;
	uint32_t roughnessTextureId = ~0u;
	uint32_t metallicTextureId = ~0u;

	uint32_t rsmPipelineIndex = ~0u;
	uint32_t pipelineIndex = ~0u;

	Material()
	{
		// Default shaders
		std::strcpy(this->vertexShader, "Pbr.vert.spv");
		std::strcpy(this->fragmentShader, "Pbr.frag.spv");
	}

	void matToStr(std::string& output) const
	{
		output = std::string(this->vertexShader) + ";" + std::string(this->fragmentShader);
	}
};