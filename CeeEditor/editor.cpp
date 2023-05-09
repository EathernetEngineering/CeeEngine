#include <cstdio>

#include "CeeEngine/CeeEngine.h"

#ifndef NDEBUG
#include "mcheck.h"
#endif

class GameLayer : public cee::Layer {
public:
	GameLayer();
	~GameLayer();

	void OnAttach() override;
	void OnDetach() override;

	void OnUpdate(cee::Timestep t) override;
	void OnGui() override;

	void OnEnable() override;
	void OnDisable() override;

	void MessageHandler(cee::Event& e) override;
};

GameLayer::GameLayer::GameLayer()
{
}

GameLayer::~GameLayer()
{
}

void GameLayer::OnAttach()
{
}

void GameLayer::OnDetach()
{
}

void GameLayer::OnUpdate(cee::Timestep t)
{
	(void)t;
}

void GameLayer::OnGui()
{
}

void GameLayer::OnEnable()
{
}

void GameLayer::OnDisable()
{
}

void GameLayer::MessageHandler(cee::Event& e)
{
	(void)e;
}


int main(int argc, char **argv) {
#ifndef NDEBUG
	mcheck(NULL);
#endif
	(void)argc;
	(void)argv;
	cee::Application* app = nullptr;

	app = new cee::Application;

	GameLayer layer;
	app->PushLayer(&layer);
	app->Run();

	delete app;

	return EXIT_SUCCESS;
}
