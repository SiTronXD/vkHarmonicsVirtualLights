#include "pch.h"
#include "TestScene.h"
#include "../Engine/ResourceManager.h"
#include "../Engine/Graphics/Renderer.h"

#include <imgui/imgui.h>

TestScene::TestScene()
{
}

TestScene::~TestScene()
{
}

void TestScene::init()
{
	this->camera.init(this->getWindow());

	// Set skybox cube map
	this->getRenderer().setSkyboxTexture(
		this->getResourceManager().addCubeMap({ "Resources/Textures/grace_cross.hdr" })
	);

	uint32_t brdfId0 = this->getResourceManager().addBRDF("Resources/BRDFs/pink-fabric.shbrdf");
	uint32_t brdfId1 = this->getResourceManager().addBRDF("Resources/BRDFs/red-fabric.shbrdf");

	uint32_t sphereMeshId = this->getResourceManager().addMesh("Resources/Models/sphereTest.obj");
	uint32_t boxMeshId = this->getResourceManager().addMesh("Resources/Models/box.obj");
	uint32_t whiteTextureId = this->getResourceManager().addTexture("Resources/Textures/white.png");

	// Test entity
	{
		uint32_t testEntity = this->createEntity();
		this->setComponent<MeshComponent>(testEntity, MeshComponent());
		this->setComponent<Material>(testEntity, Material());

		Transform& transform = this->getComponent<Transform>(testEntity);
		transform.modelMat = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));

		MeshComponent& modelMesh = this->getComponent<MeshComponent>(testEntity);
		modelMesh.meshId = sphereMeshId;

		Material& material = this->getComponent<Material>(testEntity);
		material.albedoTextureId = whiteTextureId;
		material.roughnessTextureId = whiteTextureId;
		material.metallicTextureId = whiteTextureId;
		material.brdfId = brdfId0;
	}

	// Walls
	glm::mat4 wallTransforms[3] =
	{
		glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(3.0f, 1.0f, 3.0f)),
		glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(3.0f, 3.0f, 1.0f)),
		glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 3.0f, 3.0f))
	};
	for(uint32_t i = 0; i < 3; ++i)
	{
		uint32_t testEntity = this->createEntity();
		this->setComponent<MeshComponent>(testEntity, MeshComponent());
		this->setComponent<Material>(testEntity, Material());

		Transform& transform = this->getComponent<Transform>(testEntity);
		transform.modelMat = wallTransforms[i];

		MeshComponent& modelMesh = this->getComponent<MeshComponent>(testEntity);
		modelMesh.meshId = boxMeshId;

		Material& material = this->getComponent<Material>(testEntity);
		material.albedoTextureId = whiteTextureId;
		material.roughnessTextureId = whiteTextureId;
		material.metallicTextureId = whiteTextureId;
		material.brdfId = brdfId1;
	}

	// Initial camera setup
	this->camera.setPosition(glm::vec3(2.0f, 2.0f, 2.0f));
	this->camera.setRotation(SMath::PI * 1.25f, -SMath::PI * 0.25f);
}

void TestScene::update()
{
	this->camera.update();
}
