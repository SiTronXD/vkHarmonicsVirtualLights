#include "pch.h"
#include "DragonScene.h"
#include "../Engine/ResourceManager.h"
#include "../Engine/Graphics/Renderer.h"

DragonScene::DragonScene()
{
}

DragonScene::~DragonScene()
{
}

void DragonScene::init()
{
	this->camera.init(this->getWindow());

	// Set skybox cube map
	this->getRenderer().setSkyboxTexture(
		this->getResourceManager().addCubeMap({ "Resources/Textures/grace_cross.hdr" })
	);

	// Assets
	uint32_t brdfPinkFabric = this->getResourceManager().addBRDF("Resources/BRDFs/pink-fabric.shbrdf");
	uint32_t brdfRed = this->getResourceManager().addBRDF("Resources/BRDFs/red-fabric.shbrdf");
	uint32_t brdfGreen = this->getResourceManager().addBRDF("Resources/BRDFs/green-acrylic.shbrdf");
	uint32_t brdfBlue = this->getResourceManager().addBRDF("Resources/BRDFs/blue-fabric.shbrdf");
	uint32_t brdfPlant = this->getResourceManager().addBRDF("Resources/BRDFs/green-plastic.shbrdf");
	uint32_t brdfMetal = this->getResourceManager().addBRDF("Resources/BRDFs/silver-metallic-paint.shbrdf");

	uint32_t whiteTextureId = this->getResourceManager().addTexture("Resources/Textures/white.png");

	// Scene entity
	{
		uint32_t sceneEntity = this->createEntity();
		this->setComponent<MeshComponent>(sceneEntity, MeshComponent());
		this->setComponent<Material>(sceneEntity, Material());

		Transform& transform = this->getComponent<Transform>(sceneEntity);
		transform.modelMat = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));

		Material& material = this->getComponent<Material>(sceneEntity);
		material.albedoTextureId = whiteTextureId;

		MeshComponent& modelMesh = this->getComponent<MeshComponent>(sceneEntity);
		modelMesh.meshId =
			this->getResourceManager().addMesh("Resources/Models/sponzaSmall.obj", material);
	}

	// Dragon
	{
		uint32_t dragonEntity = this->createEntity();
		this->setComponent<MeshComponent>(dragonEntity, MeshComponent());
		this->setComponent<Material>(dragonEntity, Material());

		Transform& transform = this->getComponent<Transform>(dragonEntity);
		transform.modelMat =
			glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, -0.1f)) * 
			glm::rotate(glm::mat4(1.0f), (-0.5f + 0.17f) * SMath::PI, glm::vec3(0.0f, 1.0f, 0.0f)) *
			glm::scale(glm::mat4(1.0f), glm::vec3(0.9f));

		Material& material = this->getComponent<Material>(dragonEntity);
		material.albedoTextureId = whiteTextureId;

		MeshComponent& modelMesh = this->getComponent<MeshComponent>(dragonEntity);
		modelMesh.meshId =
			this->getResourceManager().addMesh("Resources/Models/dragon_vrip.obj", material);
	}

	// Initial camera setup
	this->camera.setPosition(glm::vec3(-0.5f, 0.7f, -0.15f));
	this->camera.setRotation(-SMath::PI * 1.5f, -SMath::PI * 0.0f);

	// Initial light setup
	this->getRenderer().setSpotlightOrientation(
		glm::vec3(2.0f, 1.0f, 0.0f),
		glm::vec3(0.0f)
	);
}

void DragonScene::update()
{
	this->camera.update();
}
