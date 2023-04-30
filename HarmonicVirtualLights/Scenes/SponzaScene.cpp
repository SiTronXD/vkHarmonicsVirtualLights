#include "pch.h"
#include "SponzaScene.h"
#include "../Engine/ResourceManager.h"
#include "../Engine/Graphics/Renderer.h"

SponzaScene::SponzaScene() {}

SponzaScene::~SponzaScene() {}

void SponzaScene::init()
{
	this->camera.init(this->getWindow());

	// Set skybox cube map
	this->getRenderer().setSkyboxTexture(
		this->getResourceManager().addCubeMap({ "Resources/Textures/grace_cross.hdr" })
	);

	// Assets
	uint32_t brdfId0 = this->getResourceManager().addBRDF("Resources/BRDFs/pink-fabric.shbrdf");

	uint32_t sponzaMeshId = this->getResourceManager().addMesh("Resources/Models/sponzaSmall.obj");

	uint32_t whiteTextureId = this->getResourceManager().addTexture("Resources/Textures/white.png");

	// Sponza entity
	{
		uint32_t sponzaEntity = this->createEntity();
		this->setComponent<MeshComponent>(sponzaEntity, MeshComponent());
		this->setComponent<Material>(sponzaEntity, Material());

		Transform& transform = this->getComponent<Transform>(sponzaEntity);
		transform.modelMat = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));

		MeshComponent& modelMesh = this->getComponent<MeshComponent>(sponzaEntity);
		modelMesh.meshId = sponzaMeshId;

		Material& material = this->getComponent<Material>(sponzaEntity);
		material.albedoTextureId = whiteTextureId;
		material.brdfId = brdfId0;
	}

	// Initial camera setup
	this->camera.setPosition(glm::vec3(-1.0f, 0.5f, 1.0f));
	this->camera.setRotation(SMath::PI * 1.0f, -SMath::PI * 0.1f);
}

void SponzaScene::update()
{
	this->camera.update();
}
