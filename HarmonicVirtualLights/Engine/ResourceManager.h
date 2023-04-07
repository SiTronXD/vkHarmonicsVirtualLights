#pragma once

#include <vector>
#include "Graphics/Mesh.h"
#include "Graphics/Texture.h"

struct GfxAllocContext;

class ResourceManager
{
private:
	std::vector<Mesh> meshes;
	std::vector<std::shared_ptr<Texture>> textures;

	const GfxAllocContext* gfxAllocContext;

public:
	ResourceManager();
	~ResourceManager();

	void init(const GfxAllocContext& gfxAllocContext);
	void cleanup();

	uint32_t addMesh(const std::string& filePath);
	uint32_t addTexture(const std::string& filePath);
	uint32_t addEmptyTexture();
	uint32_t addCubeMap(const std::vector<std::string>& filePaths);

	inline Mesh& getMesh(const uint32_t& meshID) { return this->meshes[meshID]; }
	inline Texture* getTexture(const uint32_t& textureID) { return this->textures[textureID].get(); }

	inline size_t getNumMeshes() const { return this->meshes.size(); }
	inline size_t getNumTextures() const { return this->textures.size(); }
};