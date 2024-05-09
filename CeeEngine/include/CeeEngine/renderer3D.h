#ifndef _CEE_ENGINE_RENDERER_3D_h
#define _CEE_ENGINE_RENDERER_3D_h

#include <CeeEngine/renderer.h>
#include <CeeEngine/camera.h>

#include <memory>
namespace cee {
class Renderer3D {
public:
	Renderer3D() = default;
	virtual ~Renderer3D() = default;

	static int32_t Init(MessageBus* msgBus, std::shared_ptr<Window> window);
	static void Shutdown();

	static void BeginFrame();
	static void Flush();
	static void EndFrame();

	static void DrawCube(const glm::vec3& translation,
						 float rotationAngle,
						 const glm::vec3& rotationAxis,
						 const glm::vec3& scale,
						 const glm::vec4& color);

	static int UpdateCamera(Camera& camera);

private:
	static bool MessageHandler(Event& e);

private:
	static RendererCapabilities s_RendererCapabilities;

	static VertexBuffer s_VertexBuffer;
	static IndexBuffer s_IndexBuffer;

	static StagingBuffer s_VertexStagingBuffer;
	static StagingBuffer s_IndexStagingBuffer;

	static size_t s_VertexOffset;
	static size_t s_IndexOffset;

private:
	static bool s_Initialized;
	static MessageBus* s_MessageBus;
	static std::shared_ptr<Renderer> s_Renderer;
};
}
#endif

