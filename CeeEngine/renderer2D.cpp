#include <CeeEngine/renderer2D.h>

#include <CeeEngine/debugMessenger.h>

namespace cee {
RendererCapabilities Renderer2D::s_RendererCapabilities = {};

VertexBuffer Renderer2D::s_VertexBuffer;
IndexBuffer Renderer2D::s_IndexBuffer;

StagingBuffer Renderer2D::s_StagingBuffer;

size_t Renderer2D::s_VertexOffset = 0;
size_t Renderer2D::s_Index= 0;

bool Renderer2D::s_Initialized = false;
MessageBus* Renderer2D::s_MessageBus = NULL;;
std::shared_ptr<Renderer> Renderer2D::s_Renderer = NULL;

void Renderer2D::Init(const RendererSpec& spec) {
	if (s_Initialized == true) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Renderer2D::Init called more than once.");
		return;
	}

	s_MessageBus = spec.msgBus;
	s_MessageBus->RegisterMessageHandler(Renderer2D::MessageHandler);

	RendererCapabilities rendererCapabilities = {};
	rendererCapabilities.applicationName = "CeeEngine Application";
	rendererCapabilities.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	rendererCapabilities.maxFramesInFlight = 3;
	rendererCapabilities.maxIndices = 10000;
	rendererCapabilities.rendererMode = RENDERER_MODE_2D;
	s_RendererCapabilities = rendererCapabilities;

	s_Renderer = std::make_shared<Renderer>(spec, rendererCapabilities);
	if (s_Renderer->Init() != 0) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Failed to initialize renderer framework for Renderer2D.");
		return;
	}

	s_VertexOffset = 0;

	size_t maxVertices = (6 * s_RendererCapabilities.maxIndices) / 4;
	s_StagingBuffer = s_Renderer->CreateStagingBuffer(sizeof(Vertex2D) * maxVertices);
	s_IndexBuffer = s_Renderer->CreateIndexBuffer(sizeof(uint32_t) * s_RendererCapabilities.maxIndices);
	s_VertexBuffer = s_Renderer->CreateVertexBuffer(sizeof(Vertex2D) * maxVertices);

	uint32_t* indices = new uint32_t[s_RendererCapabilities.maxIndices];
	size_t offset = 0, index = 0;
	for (; offset < s_RendererCapabilities.maxIndices; offset += 6) {
		indices[offset + 0] = index + 0;
		indices[offset + 1] = index + 1;
		indices[offset + 2] = index + 2;
		indices[offset + 3] = index + 2;
		indices[offset + 4] = index + 3;
		indices[offset + 5] = index + 0;

		index += 4;
	}

	s_StagingBuffer.SetData(sizeof(uint32_t) * s_RendererCapabilities.maxIndices, 0, indices);
	s_StagingBuffer.TransferData(s_IndexBuffer, 0, 0, sizeof(uint32_t) * s_RendererCapabilities.maxIndices);

	delete[] indices;

	s_Initialized = true;
}

void Renderer2D::Shutdown() {
	s_VertexBuffer = VertexBuffer();
	s_StagingBuffer = StagingBuffer();
	s_IndexBuffer = IndexBuffer();
	s_Renderer.reset();
}

void Renderer2D::BeginFrame() {
	s_Renderer->Clear({ 0.0f, 0.0f, 0.0f, 1.0f });
	s_Renderer->StartFrame();
}

void Renderer2D::Flush() {
	s_StagingBuffer.TransferData(s_VertexBuffer, 0, 0, s_VertexOffset * sizeof(Vertex2D));
	s_Renderer->Draw(s_IndexBuffer, s_VertexBuffer, s_Index);
	s_VertexOffset = 0;
	s_Index = 0;
}

void Renderer2D::EndFrame() {
	Flush();
	s_Renderer->EndFrame();
}

void Renderer2D::DrawQuad(const glm::vec3& translation,
						  float rotationAngle,
						  const glm::vec3& scale,
						  const glm::vec4& color) {
	static const glm::vec4 positions[] = {
		{ -0.5f,  0.5f, 0.0f, 1.0f },
		{  0.5f,  0.5f, 0.0f, 1.0f },
		{  0.5f, -0.5f, 0.0f, 1.0f },
		{ -0.5f, -0.5f, 0.0f, 1.0f }
	};
	static const glm::vec2 uv[] = {
		{ 0.0f, 1.0f },
		{ 1.0f, 1.0f },
		{ 1.0f, 0.0f },
		{ 0.0f, 0.0f }
	};

	glm::mat4 transform = ConstructTransformMatrix2D(translation, rotationAngle, scale);

	Vertex2D vertices[4];
	vertices[0].position = transform * positions[0];
	vertices[0].color = color;
	vertices[0].texCoords = uv[0];
	vertices[1].position = transform * positions[1];
	vertices[1].color = color;
	vertices[1].texCoords = uv[1];
	vertices[2].position = transform * positions[2];
	vertices[2].color = color;
	vertices[2].texCoords = uv[2];
	vertices[3].position = transform * positions[3];
	vertices[3].color = color;
	vertices[3].texCoords = uv[3];

	s_StagingBuffer.SetData(sizeof(vertices), s_VertexOffset * sizeof(Vertex2D), vertices);
	s_VertexOffset += 4;
	s_Index += 6;
}

int Renderer2D::UpdateCamera(Camera& camera) {
	s_Renderer->UpdateCamera(camera);
	return 0;
}

bool Renderer2D::MessageHandler(Event& e) {
	(void)e;
	return true;
}
}
