/* Decleration of window.
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

#ifndef CEE_ENGINE_WINDOW_H
#define CEE_ENGINE_WINDOW_H

#include <CeeEngine/messageBus.h>
#include <CeeEngine/platform.h>


#if defined(CEE_PLATFORM_WINDOWS)
# include <Windows.h>
	typedef HINSTANCE NativeWindowConnection;
	typedef HWND NativeWindowHandle;
#elif defined(CEE_PLATFORM_LINUX)
# include <xcb/xcb.h>
	typedef xcb_connection_t NativeWindowConnection;
	typedef xcb_window_t NativeWindowHandle;
#endif

#include <string>
#include <unordered_map>
#include <memory>

namespace cee {
	class Swapchain;
	class VulkanContext;

	struct WindowSpecification {
		uint32_t width;
		uint32_t height;

		std::string title;
	};

	class Window {
	public:
		Window(MessageBus* messageBus, const WindowSpecification& spec);
		~Window();

		NativeWindowConnection* GetNativeConnection() const;
		NativeWindowHandle GetNativeWindowHandle() const;

		uint32_t GetWidth() const { return m_Specification.width; }
		uint32_t GetHeight() const { return m_Specification.height; }
		WindowSpecification GetWindowSepcification() const { return m_Specification; }

		std::shared_ptr<VulkanContext> GetContext() { return m_RendererContext; }
		Swapchain* GetSwapchain() { return m_Swapchain; }

		void SetTitle(const std::string& title);
		void SetWidth(uint32_t width);
		void SetHeight(uint32_t height);
		void SetSize(uint32_t width, uint32_t height);

		bool ShouldClose() const { return m_ShouldClose; }

		static void PollEvents();

	private:
		void MessageHandler(Event&);

	private:
		MessageBus* m_MessageBus;
		static MessageBus* s_MessageBus; /* to post events that aren't associated
											with a specific window. e.g. KeyPressEvent */
		bool m_ShouldClose;

		WindowSpecification m_Specification;
		NativeWindowHandle m_Wnd;

#if defined(CEE_PLATFORM_LINUX)
		xcb_intern_atom_cookie_t m_WmProtocolsCookie;
		xcb_intern_atom_reply_t* m_WmProtocolsReply;
		xcb_intern_atom_cookie_t m_WmDeleteCookie;
		xcb_intern_atom_reply_t* m_WmDeleteReply;
#endif
		static std::unordered_map<NativeWindowHandle, Window*> s_WindowPointers;
		static NativeWindowConnection* s_Connection;
		static int s_WindowCount;

		std::shared_ptr<VulkanContext> m_RendererContext;
		Swapchain* m_Swapchain;
	};
}

#endif
