#ifndef CEE_ENGINE_WINDOW_H
#define CEE_ENGINE_WINDOW_H

#include <CeeEngine/messageBus.h>
#include <CeeEngine/platform.h>

#if defined(CEE_PLATFORM_LINUX)
#include <xcb/xcb.h>
#endif

#include <string>
#include <unordered_map>

namespace cee {
#if defined(CEE_PLATFORM_WINDOWS)
typedef HINSTANCE NativeWindowConnection;
typedef HWND NativeWindowHandle;
#elif defined(CEE_PLATFORM_LINUX)
typedef xcb_connection_t NativeWindowConnection;
typedef xcb_window_t NativeWindowHandle;
#endif

struct WindowSpec {
	uint32_t width;
	uint32_t height;

	std::string title;

	MessageBus* msgBus;
};

class Window {
public:
	Window(const WindowSpec& spec);
	~Window();

	NativeWindowConnection* GetNativeConnection() const;
	NativeWindowHandle GetNativeWindowHandle() const;

	uint32_t GetWidth() const { return m_Spec.width; }
	uint32_t GetHeight() const { return m_Spec.height; }
	WindowSpec GetWindowSepcification() const { return m_Spec; }

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

	WindowSpec m_Spec;
	NativeWindowHandle m_Wnd;

#if defined(CEE_PLATFORM_LINUX)
	xcb_intern_atom_cookie_t m_WmProtocolsCookie;
	xcb_intern_atom_reply_t* m_WmProtocolsReply;
	xcb_intern_atom_cookie_t m_WmDeleteCookie;
	xcb_intern_atom_reply_t* m_WmDeleteReply;
#endif // defined(CEE_PLATFORM_LINUX)

	static std::unordered_map<NativeWindowHandle, Window*> s_WindowPointers;
	static NativeWindowConnection* s_Connection;
	static int s_WindowCount;
};
}

#endif
