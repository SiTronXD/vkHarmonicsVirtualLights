#include "pch.h"
#include "ResourceManager.h"
#include "Graphics/Mesh.h"
#include "Graphics/MeshData.h"
#include "Graphics/TextureCube.h"
#include "Graphics/Texture2D.h"

ResourceManager::ResourceManager()
	: gfxAllocContext(nullptr)
{
}

ResourceManager::~ResourceManager()
{
}

void ResourceManager::init(const GfxAllocContext& gfxAllocContext)
{
	this->gfxAllocContext = &gfxAllocContext;
}

void ResourceManager::cleanup()
{
	// Meshes
	for (size_t i = 0; i < this->meshes.size(); ++i)
	{
		this->meshes[i].cleanup();
	}
	this->meshes.clear();

	// Textures
	for (size_t i = 0; i < this->textures.size(); ++i)
	{
		this->textures[i]->cleanup();
	}
	this->textures.clear();
	this->textures.shrink_to_fit();
}

uint32_t ResourceManager::addMesh(const std::string& filePath)
{
	uint32_t createdMeshIndex = uint32_t(this->meshes.size());

	// Mesh
	this->meshes.push_back(Mesh());
	Mesh& createdMesh = this->meshes[createdMeshIndex];

	// Load mesh data
	MeshData meshData;
	meshData.loadOBJ(filePath);

	// Create mesh from mesh data
	createdMesh.createMesh(
		*this->gfxAllocContext, 
		meshData
	);

	return createdMeshIndex;
}

uint32_t ResourceManager::addTexture(const std::string& filePath)
{
	// Texture Resource
	uint32_t createdTextureIndex = this->addEmptyTexture();
	Texture2D* createdTexture = static_cast<Texture2D*>(this->textures[createdTextureIndex].get());

	// Load texture
	createdTexture->createFromFile(
		*this->gfxAllocContext, 
		filePath
	);

	return createdTextureIndex;
}

uint32_t ResourceManager::addEmptyTexture()
{
	uint32_t createdTextureIndex = uint32_t(this->textures.size());

	// Add texture
	this->textures.push_back(std::shared_ptr<Texture2D>(new Texture2D()));

	return createdTextureIndex;
}

uint32_t ResourceManager::addCubeMap(const std::vector<std::string>& filePaths)
{
	// HDR for one single file
	if (filePaths.size() == 1)
	{
		if (filePaths[0].substr(filePaths[0].size() - 4, 4) != ".hdr")
		{
			Log::error("Unsupported float texture for cube map. Expected .hdr");
			return ~0u;
		}
	}
	// Non-HDR for 6 files
	else if (filePaths.size() != 6)
	{
		Log::error("The number of file paths is not 6 when adding cube map resource.");
		return ~0u;
	}

	uint32_t createdTextureIndex = uint32_t(this->textures.size());

	// Load and create cube map resource
	std::shared_ptr<TextureCube> createdCubeMap(new TextureCube());
	createdCubeMap->createCubeMapFromFile(
		*this->gfxAllocContext,
		filePaths
	);

	// Create prefiltered map for the loaded cube map
	uint32_t prefilteredTextureIndex = createdTextureIndex + 1;
	std::shared_ptr<TextureCube> prefilteredCubeMap(new TextureCube());
	prefilteredCubeMap->createAsRenderableSampledCubeMap(
		*this->gfxAllocContext, 
		createdCubeMap->getWidth(),
		createdCubeMap->getHeight(),
		8,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_IMAGE_USAGE_STORAGE_BIT
	);

	// Set index
	createdCubeMap->setPrefilteredMapIndex(
		prefilteredTextureIndex
	);

	// Add resources to list
	this->textures.push_back(createdCubeMap);
	this->textures.push_back(prefilteredCubeMap);

	return createdTextureIndex;
}
