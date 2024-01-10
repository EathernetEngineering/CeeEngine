/* Decleration of old renderer 3D specialisation.
 *   Copyright (C) 2023-2024  Chloe Eather.
 *
 *   This file is part of CeeEngine.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>. */

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

		static void Init(MessageBus* msgBus, std::shared_ptr<Window> window);
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

		static size_t s_VertexOffset;
		static size_t s_IndexOffset;

	private:
		static bool s_Initialized;
		static MessageBus* s_MessageBus;
		static std::shared_ptr<oldRenderer> s_Renderer;
	};
}
#endif

