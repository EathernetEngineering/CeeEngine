#include <CeeEngine/renderer3D.h>

#include <CeeEngine/debugMessenger.h>

#include <random>

static constexpr glm::vec4 CubeVertexPositions[] = {
	{ -1.0f,  1.0f, -1.0f, 1.0f },    /////////////////
	{  1.0f,  1.0f, -1.0f, 1.0f },    /// Top face ////
	{  1.0f,  1.0f,  1.0f, 1.0f },    /////////////////
	{ -1.0f,  1.0f,  1.0f, 1.0f },    /////////////////

	{ -1.0f,  1.0f, -1.0f, 1.0f },    /////////////////
	{ -1.0f,  1.0f,  1.0f, 1.0f },    /// Left face ///
	{ -1.0f, -1.0f,  1.0f, 1.0f },    /////////////////
	{ -1.0f, -1.0f, -1.0f, 1.0f },    /////////////////

	{ -1.0f,  1.0f,  1.0f, 1.0f },    /////////////////
	{  1.0f,  1.0f,  1.0f, 1.0f },    // Front face ///
	{  1.0f, -1.0f,  1.0f, 1.0f },    /////////////////
	{ -1.0f, -1.0f,  1.0f, 1.0f },    /////////////////

	{  1.0f,  1.0f,  1.0f, 1.0f },    /////////////////
	{  1.0f,  1.0f, -1.0f, 1.0f },    // Right face ///
	{  1.0f, -1.0f, -1.0f, 1.0f },    /////////////////
	{  1.0f, -1.0f,  1.0f, 1.0f },    /////////////////

	{  1.0f,  1.0f, -1.0f, 1.0f },    /////////////////
	{ -1.0f,  1.0f, -1.0f, 1.0f },    /// Back face ///
	{ -1.0f, -1.0f, -1.0f, 1.0f },    /////////////////
	{  1.0f, -1.0f, -1.0f, 1.0f },    /////////////////

	{  1.0f, -1.0f, -1.0f, 1.0f },    /////////////////
	{ -1.0f, -1.0f, -1.0f, 1.0f },    // Bottom face //
	{ -1.0f, -1.0f,  1.0f, 1.0f },    /////////////////
	{  1.0f, -1.0f,  1.0f, 1.0f },    /////////////////
};

static constexpr glm::vec3 CubeNormalVectors[] = {
	{  0.0f,  1.0f,  0.0f },         /////////////////
	{  0.0f,  1.0f,  0.0f },         //  Top face   //
	{  0.0f,  1.0f,  0.0f },         /////////////////
	{  0.0f,  1.0f,  0.0f },         /////////////////

	{ -1.0f,  0.0f,  0.0f },         /////////////////
	{ -1.0f,  0.0f,  0.0f },         //  Left face  //
	{ -1.0f,  0.0f,  0.0f },         /////////////////
	{ -1.0f,  0.0f,  0.0f },         /////////////////

	{  0.0f,  0.0f,  1.0f },         /////////////////
	{  0.0f,  0.0f,  1.0f },         // Front face  //
	{  0.0f,  0.0f,  1.0f },         /////////////////
	{  0.0f,  0.0f,  1.0f },         /////////////////

	{  1.0f,  0.0f,  0.0f },         /////////////////
	{  1.0f,  0.0f,  0.0f },         // Right face  //
	{  1.0f,  0.0f,  0.0f },         /////////////////
	{  1.0f,  0.0f,  0.0f },         /////////////////

	{  0.0f,  0.0f, -1.0f },         /////////////////
	{  0.0f,  0.0f, -1.0f },         //  Back face  //
	{  0.0f,  0.0f, -1.0f },         /////////////////
	{  0.0f,  0.0f, -1.0f },         /////////////////

	{  0.0f, -1.0f,  0.0f },         /////////////////
	{  0.0f, -1.0f,  0.0f },         // Bottom face //
	{  0.0f, -1.0f,  0.0f },         /////////////////
	{  0.0f, -1.0f,  0.0f }          /////////////////
};

static constexpr uint32_t CubeIndices[] = {
	 0,  1,  2,  2,  3,  0,          //  Top Face   //
	 4,  5,  6,  6,  7,  4,          //  Left Face  //
	 8,  9, 10, 10, 11,  8,          // Front Face  //
	12, 13, 14, 14, 15, 12,          // Right Face  //
	16, 17, 18, 18, 19, 16,          //  Back Face  //
	20, 21, 22, 22, 23, 20           // Bottom Face //
};

static constexpr glm::vec2 CubeTexCoords[] {
	{ 0.0f, 0.0f },
	{ 1.0f, 0.0f },
	{ 1.0f, 1.0f },
	{ 0.0f, 1.0f },

	{ 0.0f, 0.0f },
	{ 1.0f, 0.0f },
	{ 1.0f, 1.0f },
	{ 0.0f, 1.0f },

	{ 0.0f, 0.0f },
	{ 1.0f, 0.0f },
	{ 1.0f, 1.0f },
	{ 0.0f, 1.0f },

	{ 0.0f, 0.0f },
	{ 1.0f, 0.0f },
	{ 1.0f, 1.0f },
	{ 0.0f, 1.0f },

	{ 0.0f, 0.0f },
	{ 1.0f, 0.0f },
	{ 1.0f, 1.0f },
	{ 0.0f, 1.0f },

	{ 0.0f, 0.0f },
	{ 1.0f, 0.0f },
	{ 1.0f, 1.0f },
	{ 0.0f, 1.0f }
};

uint32_t CubeTexIndices[] {
	0, 0, 0, 0,
	1, 1, 1, 1,
	2, 2, 2, 2,
	3, 3, 3, 3,
	4, 4, 4, 4,
	5, 5, 5, 5
};


namespace cee {
	RendererCapabilities Renderer3D::s_RendererCapabilities = {};

	VertexBuffer Renderer3D::s_VertexBuffer;
	IndexBuffer Renderer3D::s_IndexBuffer;

	StagingBuffer Renderer3D::s_VertexStagingBuffer;
	StagingBuffer Renderer3D::s_IndexStagingBuffer;

	size_t Renderer3D::s_VertexOffset = 0;
	size_t Renderer3D::s_IndexOffset= 0;

	bool Renderer3D::s_Initialized = false;
	MessageBus* Renderer3D::s_MessageBus = NULL;;
	std::shared_ptr<Renderer> Renderer3D::s_Renderer = NULL;

	void Renderer3D::Init(MessageBus* msgBus, std::shared_ptr<Window> window) {
		if (s_Initialized == true) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
											 "Renderer3D::Init called more than once.");
			return;
		}

		s_MessageBus = msgBus;
		msgBus->RegisterMessageHandler(Renderer3D::MessageHandler);

		RendererCapabilities rendererCapabilities = {};
		rendererCapabilities.applicationName = "CeeEngine Application";
		rendererCapabilities.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		rendererCapabilities.maxFramesInFlight = 3;
		rendererCapabilities.maxIndices = 10000;
		rendererCapabilities.useRenderDocLayer = true;
		rendererCapabilities.rendererMode = RENDERER_MODE_3D;
		s_RendererCapabilities = rendererCapabilities;

		s_Renderer = std::make_shared<Renderer>(rendererCapabilities, window);
		if (s_Renderer->Init() != 0) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
											 "Failed to initialize renderer framework for Renderer3D.");
			return;
		}

		s_VertexOffset = 0;

		size_t maxVertices = (6 * s_RendererCapabilities.maxIndices) / 4;
		s_VertexStagingBuffer = s_Renderer->CreateStagingBuffer(sizeof(Vertex3D) * maxVertices);
		s_IndexStagingBuffer = s_Renderer->CreateStagingBuffer(sizeof(uint32_t) * maxVertices);
		s_IndexBuffer = s_Renderer->CreateIndexBuffer(sizeof(uint32_t) * s_RendererCapabilities.maxIndices);
		s_VertexBuffer = s_Renderer->CreateVertexBuffer(sizeof(Vertex3D) * maxVertices);

		s_Initialized = true;
	}

	void Renderer3D::Shutdown() {
		s_VertexBuffer = VertexBuffer();
		s_VertexStagingBuffer = StagingBuffer();
		s_IndexStagingBuffer = StagingBuffer();
		s_IndexBuffer = IndexBuffer();
		s_Renderer.reset();
	}

	void Renderer3D::BeginFrame() {
		s_Renderer->Clear({ 0.0f, 0.0f, 0.0f, 1.0f });
		s_Renderer->StartFrame();
	}

	void Renderer3D::Flush() {
		s_VertexStagingBuffer.TransferData(s_VertexBuffer, 0, 0, s_VertexOffset * sizeof(Vertex3D));
		s_IndexStagingBuffer.TransferData(s_IndexBuffer, 0, 0, s_IndexOffset * sizeof(uint32_t));
		s_Renderer->Draw(s_IndexBuffer, s_VertexBuffer, s_IndexOffset);
		s_VertexOffset = 0;
		s_IndexOffset = 0;
	}

	void Renderer3D::EndFrame() {
		Flush();
		s_Renderer->EndFrame();
	}

	void Renderer3D::DrawCube(const glm::vec3& translation,
							  float rotationAngle,
							  const glm::vec3& rotationAxis,
							  const glm::vec3& scale,
							  const glm::vec4& color) {
		glm::mat4 transform = ConstructTransformMatrix3D(translation, rotationAngle, rotationAxis, scale);

		std::random_device rd;
		std::mt19937 gen(rd());

		std::array<Vertex3D, 24> vertices;
		for (size_t i = 0 ; i < vertices.size(); i++) {
			vertices[i].position = transform * CubeVertexPositions[i];
			vertices[i].normal = CubeNormalVectors[i];
			if (rotationAngle != 0.0f) {
				glm::mat4 nrm = glm::identity<glm::mat3>();
				nrm = glm::rotate(nrm, rotationAngle, rotationAxis);
				vertices[i].normal = glm::vec4(vertices[i].normal, 0.0f) * nrm;
			}
			vertices[i].color = color;
			vertices[i].texCoords = CubeTexCoords[i];
			vertices[i].texIndex = CubeTexIndices[i];
		}
		// for (uint32_t i = 0; i < vertices.size(); i+=4) {
		// 	vertices[i].color.r = std::generate_canonical<float, 10>(gen);
		// 	vertices[i].color.g = std::generate_canonical<float, 10>(gen);
		// 	vertices[i].color.b = std::generate_canonical<float, 10>(gen);
		// 	vertices[i].color.a = 1.0f;
  //
		// 	vertices[i + 1].color = vertices[i].color;
		// 	vertices[i + 2].color = vertices[i].color;
		// 	vertices[i + 3].color = vertices[i].color;
		// }
		s_VertexStagingBuffer.SetData(vertices.size() * sizeof(Vertex3D), s_VertexOffset * sizeof(Vertex3D), vertices.data());
		s_IndexStagingBuffer.SetData(36 * sizeof(uint32_t), s_IndexOffset * sizeof(uint32_t), CubeIndices);
		s_VertexOffset += 24;
		s_IndexOffset += 36;
	}

	int Renderer3D::UpdateCamera(Camera& camera) {
		s_Renderer->UpdateCamera(camera);
		return 0;
	}

	bool Renderer3D::MessageHandler(Event& e) {
		(void)e;
		return true;
	}
}

