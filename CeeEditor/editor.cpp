#include <cstdio>

#include <CeeEngine/application.h>
#include <CeeEngine/renderer3D.h>
#include <CeeEngine/camera.h>

#include <glm/gtx/transform.hpp>
#include <chrono>

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
	void OnRender() override;
	void OnGui() override;

	void OnEnable() override;
	void OnDisable() override;

	void MessageHandler(cee::Event& e) override;

private:
	cee::PerspectiveCamera m_Camera;
};

GameLayer::GameLayer::GameLayer()
: m_Camera(60.0f, 1.77778, 0.001f, 256.0f)
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
	m_Camera.Rotate(M_PI_4 * (float(t.nsec)/1000000000.0f), { 0.0f, 1.0f, 0.0f });
	cee::Renderer3D::UpdateCamera(m_Camera);
}

void GameLayer::OnRender() {
	static std::chrono::time_point start = std::chrono::system_clock::now();
	std::chrono::time_point now = std::chrono::system_clock::now();
	if ((float)std::chrono::duration_cast<std::chrono::milliseconds>(start - now).count() > 1000.0f * M_2_PIf) {
		start = std::chrono::system_clock::now();
	}
	float duration = (float)std::chrono::duration_cast<std::chrono::milliseconds>(start - now).count();
	duration /= 1000.0f;
	cee::Renderer3D::DrawCube({ 4.3f, 0.0f, -5.0f },
							  0.0f,
							  { 0.0f, 0.0f, 0.0f },
							  { 0.6f, 0.6f, 2.0f },
							  { 1.0f, 1.0f, 1.0f, 1.0f });

	cee::Renderer3D::DrawCube({ 0.0f, 0.0f, -5.0f },
							  duration,
							  { -1.0, -1.0, 1.0f },
							  { 0.5f, 0.5f, 0.5f },
							  { 1.0f, 1.0f, 1.0f, 1.0f });
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

	GameLayer* gameLayer = new GameLayer;
	app->PushLayer(gameLayer);
	app->Run();

	delete app;

	return EXIT_SUCCESS;
}
