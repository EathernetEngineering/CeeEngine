#include <CeeEngine/window.h>

#include <CeeEngine/debugMessenger.h>
#include <CeeEngine/keyCodes.h>

#include <cstdio>

namespace cee {
MessageBus* Window::s_MessageBus = NULL;
std::unordered_map<NativeWindowHandle, Window*> Window::s_WindowPointers;
NativeWindowConnection* Window::s_Connection = NULL;
int Window::s_WindowCount = 0;

Window::Window(const WindowSpec& spec)
 : m_MessageBus(spec.msgBus), m_ShouldClose(true), m_Spec(spec), m_Wnd(0)
{
	if (s_Connection == NULL) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_INFO, "Opening connection to XCB server.");
		s_Connection = xcb_connect(NULL, NULL);

		if (s_Connection == NULL) {
			DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "Failed to establish connection to XCB server.");
			return;
		}
	}
	m_MessageBus->RegisterMessageHandler([this](Event& e){ return this->MessageHandler(e); });
	if (s_MessageBus == NULL) {
		s_MessageBus = m_MessageBus;
	}

	const xcb_setup_t *setup = xcb_get_setup(s_Connection);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
	xcb_screen_t *screen = iter.data;

	uint32_t valueList[32];
	valueList[0] = screen->black_pixel;
	valueList[1] = XCB_CW_EVENT_MASK | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

	m_Wnd = xcb_generate_id(s_Connection);
	xcb_create_window(s_Connection,
					  XCB_COPY_FROM_PARENT,
					  m_Wnd,
					  screen->root,
					  0, 0,
					  m_Spec.width, m_Spec.height,
					  0,
					  XCB_WINDOW_CLASS_INPUT_OUTPUT,
					  screen->root_visual,
					  XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
					  &valueList);
	s_WindowPointers[m_Wnd] = this;

	xcb_generic_error_t* error = NULL;
	m_WmProtocolsCookie = xcb_intern_atom(s_Connection, 1, 12, "WM_PROTOCOLS");
	m_WmProtocolsReply = xcb_intern_atom_reply(s_Connection, m_WmProtocolsCookie, &error);
	if (error) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "XCB Error code from cookie \"WM_PROTOCOLS\": %hhu", error->error_code);
		free(error);
	}
	error = NULL;
	m_WmDeleteCookie = xcb_intern_atom(s_Connection, 0, 16, "WM_DELETE_WINDOW");
	m_WmDeleteReply = xcb_intern_atom_reply(s_Connection, m_WmDeleteCookie, &error);
	if (error) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR, "XCB Error code from cookie \"WM_DELETE_WINDOW\": %hhu", error->error_code);
		free(error);
	}

	xcb_change_property(s_Connection,
						XCB_PROP_MODE_REPLACE,
						m_Wnd,
						m_WmProtocolsReply->atom,
						XCB_ATOM_ATOM,
						32,
						1,
						&m_WmDeleteReply->atom);

	xcb_change_property(s_Connection,
						XCB_PROP_MODE_REPLACE,
						m_Wnd,
						XCB_ATOM_WM_NAME,
						XCB_ATOM_STRING,
						8,
						m_Spec.title.length(),
						m_Spec.title.c_str());

	xcb_map_window(s_Connection, m_Wnd);
	xcb_flush(s_Connection);

	xcb_get_geometry_cookie_t geometryCookie = xcb_get_geometry(s_Connection, m_Wnd);
	xcb_get_geometry_reply_t* geometryReply = xcb_get_geometry_reply(s_Connection, geometryCookie, NULL);
	if (geometryReply) {
		m_Spec.width = geometryReply->width;
		m_Spec.height = geometryReply->height;
		free(geometryReply);
	}

	m_ShouldClose = false;

	s_WindowCount++;
}

Window::~Window()
{
	xcb_destroy_window(s_Connection, m_Wnd);
	s_WindowCount--;
	if (s_WindowCount == 0) {
		xcb_disconnect(s_Connection);
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_INFO, "Closing connection to XCB server.");
		s_Connection = NULL;
	}
}

void Window::SetTitle(const std::string& title)
{
	m_Spec.title = title;
	xcb_change_property(s_Connection,
						XCB_PROP_MODE_REPLACE,
						m_Wnd,
						XCB_ATOM_WM_NAME,
						XCB_ATOM_STRING,
						8,
						m_Spec.title.length(),
						m_Spec.title.c_str());
}

void Window::SetWidth(uint32_t width)
{
	xcb_configure_window(s_Connection, m_Wnd, XCB_CONFIG_WINDOW_WIDTH, &width);
}

void Window::SetHeight(uint32_t height)
{
	xcb_configure_window(s_Connection, m_Wnd, XCB_CONFIG_WINDOW_HEIGHT, &height);
}

void Window::SetSize(uint32_t width, uint32_t height)
{
	xcb_configure_window(s_Connection, m_Wnd, XCB_CONFIG_WINDOW_WIDTH, &width);
	xcb_configure_window(s_Connection, m_Wnd, XCB_CONFIG_WINDOW_HEIGHT, &height);
}

NativeWindowConnection* Window::GetNativeConnection() const
{
	return s_Connection;
}

NativeWindowHandle Window::GetNativeWindowHandle() const
{
	return m_Wnd;
}

void Window::PollEvents() {
	Window* owner;
	xcb_generic_event_t* e;
	while ((e = xcb_poll_for_event(s_Connection))) {
		switch (e->response_type & ~0x80) {
		case XCB_CONFIGURE_NOTIFY:
			owner = s_WindowPointers[((xcb_configure_notify_event_t*)e)->window];
			if (owner->m_Spec.width != ((xcb_configure_notify_event_t*)e)->width ||
				owner->m_Spec.height != ((xcb_configure_notify_event_t*)e)->height)
			{
				owner->m_Spec.width = ((xcb_configure_notify_event_t*)e)->width;
				owner->m_Spec.height = ((xcb_configure_notify_event_t*)e)->height;
				WindowResizeEvent* event = new WindowResizeEvent(
					owner->m_Spec.width,
					owner->m_Spec.height);

				owner->m_MessageBus->PostMessage(event);
			}
			break;

		case XCB_KEY_PRESS:
		{
			KeyPressedEvent* keyEvent = new KeyPressedEvent(((xcb_key_press_event_t*)e)->detail);
			s_MessageBus->PostMessage(keyEvent);
		}
			break;

		case XCB_KEY_RELEASE:
		{
			KeyReleasedEvent* keyEvent = new KeyReleasedEvent(((xcb_key_press_event_t*)e)->detail);
			s_MessageBus->PostMessage(keyEvent);
		}
			break;

		case XCB_CLIENT_MESSAGE:
			owner = s_WindowPointers[((xcb_client_message_event_t*)e)->window];
			if (((xcb_client_message_event_t*)(e))->data.data32[0] == (*owner->m_WmDeleteReply).atom) {
				owner->m_ShouldClose = true;
				WindowCloseEvent* event = new WindowCloseEvent();
				owner->m_MessageBus->PostMessage(event);
			}
			break;

		default:
			break;
		}
		free(e);
	}
}

void Window::MessageHandler(Event& e) {
	(void)e;
}
}

