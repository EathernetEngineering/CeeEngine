/* Implementation of CeeEngine Applications.
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

#include <CeeEngine/application.h>

#include <cstdio>
#include <cstring>

#include <chrono>
#include <iostream>

#include <CeeEngine/input.h>
#include <CeeEngine/renderer2D.h>
#include <CeeEngine/renderer3D.h>
#include <CeeEngine/renderer/swapchain.h>


namespace cee {
	Application* Application::s_Instance = nullptr;

	Application::Application()
	 : m_LayerStack(&m_MessageBus), m_RenderThread(RenderThread::ThreadingPolicy::SINGLETHREADED) {
		if (s_Instance) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Application already exits...\tExiting...\t");
			std::exit(EXIT_FAILURE);
		}
		s_Instance = this;

		auto start = std::chrono::high_resolution_clock::now();

#ifndef NDEBUG
		m_DebugLayer = new DebugLayer;
		m_LayerStack.PushLayer(m_DebugLayer);
#endif

		m_RenderThread.Run();

		WindowSpecification windowSpec = {};
		windowSpec.width = 1280;
		windowSpec.height = 720;
		windowSpec.title = "CeeEngine Application";
		m_Window = std::make_shared<Window>(&m_MessageBus, windowSpec);
		Input::Init(&m_MessageBus, m_Window);
		//Renderer3D::Init(&m_MessageBus, m_Window);
		m_MessageBus.RegisterMessageHandler([this](Event& e){ (void)(this->OnEvent(e)); });

		Renderer::Init();
		m_RenderThread.Pump();

		auto end = std::chrono::high_resolution_clock::now();
		float duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0f;
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_DEBUG, "Time to initialise engine: %.3fms", duration);
	}

	Application::~Application() {
		m_RenderThread.Terminate();

		for (auto& layer : m_LayerStack) {
			layer->OnDetach();
			delete layer;
		}
		Renderer3D::Shutdown();
		s_Instance = nullptr;
	}

	bool Application::OnEvent(Event& e)
	{
		(void)e;
		return true;
	}

	void Application::PushLayer(Layer *layer)
	{
		m_LayerStack.PushLayer(layer);
	}

	void Application::PushOverlay(Layer *overlay)
	{
		m_LayerStack.PushOverlay(overlay);
	}

	void Application::Run()
	{
		//m_RenderThread = std::thread(&Renderer::Run, m_Renderer);
		Timestep ts, start, end;
		GetTime(&start);
		memset(&ts, 0, sizeof(Timestep));
		memset(&end, 0, sizeof(Timestep));
		uint64_t frameIndex = 0;
		m_Running = true;
		while (m_Running) {
			m_RenderThread.BlockUntilRenderComplete();
			GetTime(&end);
			GetTimeStep(&start, &end, &ts);
			GetTime(&start);
			m_AverageFrameTime *= frameIndex++;
			m_AverageFrameTime += ts.nsec + ts.sec * 1000000000;
			m_AverageFrameTime /= frameIndex;

			m_RenderThread.NextFrame();
			m_RenderThread.Kick();

			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_DEBUG, "Time to render frame: %.3fms (%.3ffps)",
											 (ts.nsec + ts.sec * 1000000000)/1000000.0f,
											 1.0f/((ts.nsec + ts.sec * 1000000000)/1000000000.0f));


			for (auto& layer : m_LayerStack) {
				layer->OnUpdate(ts);
			};


			// Renderer3D::BeginFrame();
			Renderer::Submit([&](){
				m_Window->GetSwapchain()->BeginFrame();
			});
			Renderer::BeginFrame();
			for (auto& layer : m_LayerStack) {
				// layer->OnRender();;
			};

			Renderer::Submit([&](){
				auto cmdBuffer = m_Window->GetSwapchain()->GetCurrentDrawCommandBuffer();

				VkCommandBufferBeginInfo beginInfo = {
					.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
					.pNext = nullptr,
					.flags = 0,
					.pInheritanceInfo = nullptr
				};
				vkBeginCommandBuffer(cmdBuffer, &beginInfo);

				auto swapchainImage = m_Window->GetSwapchain()->GetImage();
				VkImageMemoryBarrier barrier = {
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.pNext = nullptr,
					.srcAccessMask = 0,
					.dstAccessMask = 0,
					.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.image = swapchainImage,
					.subresourceRange = {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.baseMipLevel = 0,
						.levelCount = 1,
						.baseArrayLayer = 0,
						.layerCount = 1
					}
				};

				vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

				vkEndCommandBuffer(cmdBuffer);
			});

			Renderer::EndFrame();
			// Renderer3D::EndFrame();
			Renderer::Submit([&](){
				m_Window->SwapBuffers();
			});


			Window::PollEvents();
			m_MessageBus.DispatchEvents();
			m_Running = !m_Window->ShouldClose();
		}
		//m_RenderThread.join();
	}
}
