#include "pch.h"
#include "TestScene.h"
#include "../Engine/ResourceManager.h"
#include "../Engine/Graphics/Renderer.h"

#include <imgui/imgui.h>

TestScene::TestScene()
	: testEntity(~0u)
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
	/*this->getRenderer().setSkyboxTexture(
		this->getResourceManager().addCubeMap(
			{
				"Resources/Textures/graceCathedral_Right.png",
				"Resources/Textures/graceCathedral_Left.png",
				"Resources/Textures/graceCathedral_Top.png",
				"Resources/Textures/graceCathedral_Bottom.png",
				"Resources/Textures/graceCathedral_Front.png",
				"Resources/Textures/graceCathedral_Back.png"
			}
		)
	);*/

	uint32_t brdfId0 = this->getResourceManager().addBRDF("Resources/BRDFs/pink-fabric.shbrdf");
	brdfId0 = this->getResourceManager().addBRDF("Resources/BRDFs/pink-fabric.shbrdf");

	uint32_t brdfId1 = this->getResourceManager().addBRDF("Resources/BRDFs/pink-fabric.shbrdf");

	uint32_t meshId = this->getResourceManager().addMesh("Resources/Models/sphereTest.obj");
	//uint32_t meshId = this->getResourceManager().addMesh("Resources/Models/suzanne.obj");
	uint32_t albedoTextureId = this->getResourceManager().addTexture("Resources/Textures/rustediron-streaks2-bl/rustediron-streaks_basecolor.png");
	uint32_t roughnessTextureId = this->getResourceManager().addTexture("Resources/Textures/rustediron-streaks2-bl/rustediron-streaks_roughness.png");
	uint32_t metallicTextureId = this->getResourceManager().addTexture("Resources/Textures/rustediron-streaks2-bl/rustediron-streaks_metallic.png");

	// Test entity, modifiable material properties through imgui
	{
		this->testEntity = this->createEntity();
		this->setComponent<MeshComponent>(this->testEntity, MeshComponent());
		this->setComponent<Material>(this->testEntity, Material());

		Transform& transform = this->getComponent<Transform>(this->testEntity);
		transform.modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));

		MeshComponent& modelMesh = this->getComponent<MeshComponent>(this->testEntity);
		modelMesh.meshId = meshId;

		Material& material = this->getComponent<Material>(this->testEntity);
		material.albedoTextureId = albedoTextureId;
		material.roughnessTextureId = roughnessTextureId;
		material.metallicTextureId = metallicTextureId;
	}

	// Add objects with varying properties
	int maxX = 5;
	int maxY = 5;
	for (int y = 0; y < maxY; ++y)
	{
		for (int x = 0; x < maxX; ++x)
		{
			Material newMat{};
			newMat.roughness = (float) x / (float) (maxX - 1);
			newMat.metallic = (float) y / (float) (maxY - 1);
			newMat.albedoTextureId = albedoTextureId;
			newMat.roughnessTextureId = roughnessTextureId;
			newMat.metallicTextureId = metallicTextureId;

			uint32_t newEntity = this->createEntity();
			this->setComponent<MeshComponent>(newEntity, MeshComponent());
			this->setComponent<Material>(newEntity, newMat);

			Transform& transform = this->getComponent<Transform>(newEntity);
			transform.modelMat = 
				glm::translate(glm::mat4(1.0f), glm::vec3(x - maxX / 2, y - maxY / 2, 0.0f)) * 
				glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));

			MeshComponent& modelMesh = this->getComponent<MeshComponent>(newEntity);
			modelMesh.meshId = meshId;
		}
	}
}

void TestScene::update()
{
	this->camera.update();

	// Modifiable material for test entity
	Material& mat = this->getComponent<Material>(this->testEntity);

	float roughness = mat.roughness;
	float metallic = mat.metallic;

	ImGui::Begin("Material");
	ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f);
	ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f);
	ImGui::End();

	mat.roughness = roughness;
	mat.metallic = metallic;
}
