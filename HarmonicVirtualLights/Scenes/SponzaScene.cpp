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
	uint32_t brdfId1 = this->getResourceManager().addBRDF("Resources/BRDFs/red-fabric.shbrdf");

	uint32_t whiteTextureId = this->getResourceManager().addTexture("Resources/Textures/white.png");

	// Sponza entity
	{
		uint32_t sponzaEntity = this->createEntity();
		this->setComponent<MeshComponent>(sponzaEntity, MeshComponent());
		this->setComponent<Material>(sponzaEntity, Material());

		Transform& transform = this->getComponent<Transform>(sponzaEntity);
		transform.modelMat = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));

		Material& material = this->getComponent<Material>(sponzaEntity);
		material.albedoTextureId = whiteTextureId;

		MeshComponent& modelMesh = this->getComponent<MeshComponent>(sponzaEntity);
		modelMesh.meshId = 
			this->getResourceManager().addMesh("Resources/Models/sponzaSmall.obj", material);

		SubmeshMaterial submeshMaterial{};
		submeshMaterial.brdfIndex = brdfId1;
		this->getResourceManager().getMaterialSet(material.materialSetIndex).applySubmeshMaterial(7, submeshMaterial);
	}

	// Initial camera setup
	this->camera.setPosition(glm::vec3(-1.0f, 0.5f, 1.0f));
	this->camera.setRotation(SMath::PI * 1.0f, -SMath::PI * 0.1f);
}

void SponzaScene::update()
{
	this->camera.update();
}
