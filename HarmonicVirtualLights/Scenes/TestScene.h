#pragma once

#include "../Engine/Application/Scene.h"

class TestScene : public Scene
{
private:
	uint32_t testEntity;

public:
	TestScene();
	~TestScene();

	void init() override;
	void update() override;
};