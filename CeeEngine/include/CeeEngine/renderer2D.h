#ifndef _CEE_ENGINE_RENDERER_2D_h
#define _CEE_ENGINE_RENDERER_2D_h

#include <CeeEngine/renderer.h>
#include <CeeEngine/camera.h>

#include <memory>

namespace cee {
	class Renderer2D {
	public:
		Renderer2D() = default;
		virtual ~Renderer2D() = default;

		static void Init(MessageBus* msgBus, std::shared_ptr<Window> window);
		static void Shutdown();

		static void BeginFrame();
		static void Flush();
		static void EndFrame();

		static void DrawQuad(const glm::vec3& translation,
							 float rotationAngle,
							 const glm::vec3& scale,
							 const glm::vec4& color);

		static int UpdateCamera(Camera& camera);

	private:
		static bool MessageHandler(Event& e);

	private:
		static RendererCapabilities s_RendererCapabilities;

		static VertexBuffer s_VertexBuffer;
		static IndexBuffer s_IndexBuffer;

		static StagingBuffer s_StagingBuffer;

		static size_t s_VertexOffset;
		static size_t s_Index;

	private:
		static bool s_Initialized;
		static MessageBus* s_MessageBus;
		static std::shared_ptr<Renderer> s_Renderer;
	};
}
#endif
