#pragma once

#include "../SMath.h"

struct CamUBO
{
	glm::mat4 vp;
	glm::vec4 pos;
};

// Push constant data
struct PCD
{
	glm::mat4 modelMat;
	glm::vec4 materialProperties; // vec4(roughness, metallic, 0, 0)
};

struct PostProcessPCD
{
	glm::uvec4 resolution; // uvec4(width, height, 0, 0)
};

struct PrefilterPCD
{
	glm::uvec4 resolution; // uvec4(mip 0 width, mip 0 height, mip N width, mip N height)
	glm::vec4 roughness; // vec4(roughness, 0.0f, 0.0f, 0.0f)
};