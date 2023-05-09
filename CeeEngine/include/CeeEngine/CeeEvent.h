#ifndef CEE_EVENT_H
#define CEE_EVENT_H

#include <string>
#include <sstream>

#include <CeeEngine/CeeEnginePlatform.h>
#include <CeeEngine/CeeKeyCodes.h>

namespace cee {
	enum class EventType {
		none = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMove,
		AppTick, AppUpdate, AppRender,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMove, MouseScroll
	};

	enum EventCategory {
		EventCategoryApplication = 1 << 0,
		EventCategoryInput       = 1 << 1,
		EventCategoryKeyboard    = 1 << 2,
		EventCategoryMouseButton = 1 << 3,
		EventCategoryMouse       = 1 << 4,
	};

	class CEEAPI Event {
	public:
		Event() = default;
		virtual ~Event() = default;

		bool handled = false;

		virtual EventType GetEventType() const = 0;
		virtual const char* GetName() const = 0;
		virtual int GetCategoryFlags() const = 0;
		virtual std::string ToString() const { return this->GetName(); }

		bool IsInCategory(EventCategory category) const {
			return GetCategoryFlags() & category;
		}
	};

	class CEEAPI EventDispatcher {
	public:
		EventDispatcher(Event& e)
		 : m_Event(e)
		{
		}

		template<typename T, typename F>
		bool CEECALL dispatch(const F& funtion) {
			if (m_Event.GetEventType() == T::GetStaticType()) {
				m_Event.handled |= funtion(static_cast<T&>(m_Event));
				return true;
			}
			return false;
		}

	private:
		Event& m_Event;
	};

	CEEAPI inline std::ostream& operator<<(std::ostream& os, Event& e) {
		return os << e.ToString();
	}

	class CEEAPI WindowCloseEvent : public Event {
	public:
		WindowCloseEvent() = default;
		~WindowCloseEvent() = default;

		EventType CEECALL GetEventType() const override {
			return EventType::WindowClose;
		}

		const char* CEECALL GetName() const override {
			return "WindowClose";
		}

		int CEECALL GetCategoryFlags() const override {
			return EventCategoryApplication;
		}

		std::string CEECALL ToString() const override {
			std::stringstream ss;
			ss << "Window close event";
			return ss.str();
		}

		static EventType GetStaticEventType() {
			return EventType::WindowClose;
		}
	};

	class CEEAPI WindowResizeEvent : public Event {
	public:
		WindowResizeEvent(int width, int height)
		 : m_Width(width), m_Height(height)
		{
		}
		~WindowResizeEvent() = default;

		int CEECALL GetWidth() const { return m_Width; }
		int CEECALL GetHeight() const { return m_Height; }

		EventType CEECALL GetEventType() const override {
			return EventType::WindowResize;
		}

		const char* CEECALL GetName() const override {
			return "WindowResize";
		}

		int CEECALL GetCategoryFlags() const override {
			return EventCategoryApplication;
		}

		std::string CEECALL ToString() const override {
			std::stringstream ss;
			ss << "Window resize event: (" << m_Width << "x" << m_Height << ")";
			return ss.str();
		}

		static EventType GetStaticEventType() {
			return EventType::WindowResize;
		}

	private:
		int m_Width, m_Height;
	};

	class CEEAPI WindowFocusEvent : public Event {
	public:
		WindowFocusEvent() = default;
		~WindowFocusEvent() = default;

		EventType CEECALL GetEventType() const override {
			return EventType::WindowFocus;
		}

		const char* CEECALL GetName() const override {
			return "WindowFocus";
		}

		int CEECALL GetCategoryFlags() const override {
			return EventCategoryApplication;
		}

		std::string CEECALL ToString() const override {
			std::stringstream ss;
			ss << "Window focus event";
			return ss.str();
		}

		static EventType GetStaticEventType() {
			return EventType::WindowFocus;
		}
	};

	class CEEAPI WindowLostFocusEvent : public Event {
	public:
		WindowLostFocusEvent() = default;
		~WindowLostFocusEvent() = default;

		EventType CEECALL GetEventType() const override {
			return EventType::WindowLostFocus;
		}

		const char* CEECALL GetName() const override {
			return "WindowLostFocus";
		}

		int CEECALL GetCategoryFlags() const override {
			return EventCategoryApplication;
		}

		std::string CEECALL ToString() const override {
			std::stringstream ss;
			ss << "Window lost focus event";
			return ss.str();
		}

		static EventType GetStaticEventType() {
			return EventType::WindowLostFocus;
		}
	};

	class CEEAPI WindowMoveEvent : public Event {
	public:
		WindowMoveEvent(int x, int y)
		 : m_X(x), m_Y(y)
		{
		}
		~WindowMoveEvent() = default;

		int CEECALL GetX() const { return m_X; }
		int CEECALL GetY() const { return m_Y; }

		EventType CEECALL GetEventType() const override {
			return EventType::WindowMove;
		}

		const char* CEECALL GetName() const override {
			return "WindowMove";
		}

		int CEECALL GetCategoryFlags() const override {
			return EventCategoryApplication;
		}

		std::string CEECALL ToString() const override {
			std::stringstream ss;
			ss << "Window move event: (" << m_X << "," << m_Y << ")";
			return ss.str();
		}

		static EventType GetStaticEventType() {
			return EventType::WindowMove;
		}

	private:
		int m_X, m_Y;
	};

	class CEEAPI AppTickEvent : public Event {
	public:
		AppTickEvent() = default;
		~AppTickEvent() = default;

		EventType CEECALL GetEventType() const override {
			return EventType::AppTick;
		}

		const char* CEECALL GetName() const override {
			return "AppTick";
		}

		int CEECALL GetCategoryFlags() const override {
			return EventCategoryApplication;
		}

		std::string CEECALL ToString() const override {
			std::stringstream ss;
			ss << "App tick event";
			return ss.str();
		}

		static EventType GetStaticEventType() {
			return EventType::AppTick;
		}
	};

	class CEEAPI AppUpdateEvent : public Event {
	public:
		AppUpdateEvent() = default;
		~AppUpdateEvent() = default;

		EventType CEECALL GetEventType() const override {
			return EventType::AppUpdate;
		}

		const char* CEECALL GetName() const override {
			return "AppUpdate";
		}

		int CEECALL GetCategoryFlags() const override {
			return EventCategoryApplication;
		}

		std::string CEECALL ToString() const override {
			std::stringstream ss;
			ss << "App update event";
			return ss.str();
		}

		static EventType GetStaticEventType() {
			return EventType::AppUpdate;
		}
	};

	class CEEAPI AppRenderEvent : public Event {
	public:
		AppRenderEvent() = default;
		~AppRenderEvent() = default;

		EventType CEECALL GetEventType() const override {
			return EventType::AppRender;
		}

		const char* CEECALL GetName() const override {
			return "AppRender";
		}

		int CEECALL GetCategoryFlags() const override {
			return EventCategoryApplication;
		}

		std::string CEECALL ToString() const override {
			std::stringstream ss;
			ss << "App Render event";
			return ss.str();
		}

		static EventType GetStaticEventType() {
			return EventType::AppRender;
		}
	};

	class CEEAPI KeyPressedEvent : public Event {
	public:
		KeyPressedEvent(KeyCode keycode, bool isRepeat = false)
		: m_Keycode(keycode), m_IsRepeat(isRepeat)
		{
		}
		~KeyPressedEvent() = default;

		EventType CEECALL GetEventType() const override {
			return EventType::KeyPressed;
		}

		const char* CEECALL GetName() const override {
			return "KeyPressed";
		}

		int CEECALL GetCategoryFlags() const override {
			return EventCategoryInput | EventCategoryKeyboard;
		}

		std::string CEECALL ToString() const override {
			std::stringstream ss;
			ss << "Key pressed event (keycode = " << m_Keycode << ", is repeat = " << m_IsRepeat << ")";
			return ss.str();
		}

		static EventType GetStaticEventType() {
			return EventType::KeyPressed;
		}
	private:
		KeyCode m_Keycode;
		bool m_IsRepeat;
	};

	class CEEAPI KeyReleasedEvent : public Event {
	public:
		KeyReleasedEvent(KeyCode keycode)
		: m_Keycode(keycode)
		{
		}
		~KeyReleasedEvent() = default;

		EventType CEECALL GetEventType() const override {
			return EventType::KeyReleased;
		}

		const char* CEECALL GetName() const override {
			return "KeyReleased";
		}

		int CEECALL GetCategoryFlags() const override {
			return EventCategoryInput | EventCategoryKeyboard;
		}

		std::string CEECALL ToString() const override {
			std::stringstream ss;
			ss << "Key released event (" << m_Keycode << ")";
			return ss.str();
		}

		static EventType GetStaticEventType() {
			return EventType::KeyReleased;
		}
	private:
		KeyCode m_Keycode;
	};

	class CEEAPI KeyTypedEvent : public Event {
	public:
		KeyTypedEvent(KeyCode keycode)
		: m_Keycode(keycode)
		{
		}
		~KeyTypedEvent() = default;

		EventType CEECALL GetEventType() const override {
			return EventType::KeyTyped;
		}

		const char* CEECALL GetName() const override {
			return "KeyTyped";
		}

		int CEECALL GetCategoryFlags() const override {
			return EventCategoryInput | EventCategoryKeyboard;
		}

		std::string CEECALL ToString() const override {
			std::stringstream ss;
			ss << "Key typed event (" << m_Keycode << ")";
			return ss.str();
		}

		static EventType GetStaticEventType() {
			return EventType::KeyTyped;
		}
	private:
		KeyCode m_Keycode;
	};

	class CEEAPI MouseButtonPressedEvent : public Event {
	public:
		MouseButtonPressedEvent(MouseCode mousecode)
		: m_MouseCode(mousecode)
		{
		}
		~MouseButtonPressedEvent() = default;

		EventType CEECALL GetEventType() const override {
			return EventType::MouseButtonPressed;
		}

		const char* CEECALL GetName() const override {
			return "MouseButtonPressed";
		}

		int CEECALL GetCategoryFlags() const override {
			return EventCategoryInput | EventCategoryMouseButton | EventCategoryMouse;
		}

		std::string CEECALL ToString() const override {
			std::stringstream ss;
			ss << "Mouse button pressed event (" << m_MouseCode << ")";
			return ss.str();
		}

		static EventType GetStaticEventType() {
			return EventType::MouseButtonPressed;
		}
	private:
		KeyCode m_MouseCode;
	};

	class CEEAPI MouseButtonReleasedEvent : public Event {
	public:
		MouseButtonReleasedEvent(MouseCode mousecode)
		: m_MouseCode(mousecode)
		{
		}
		~MouseButtonReleasedEvent() = default;

		EventType CEECALL GetEventType() const override {
			return EventType::MouseButtonReleased;
		}

		const char* CEECALL GetName() const override {
			return "MouseButtonReleased";
		}

		int CEECALL GetCategoryFlags() const override {
			return EventCategoryInput | EventCategoryMouseButton | EventCategoryMouse;
		}

		std::string CEECALL ToString() const override {
			std::stringstream ss;
			ss << "Mouse button released event (" << m_MouseCode << ")";
			return ss.str();
		}

		static EventType GetStaticEventType() {
			return EventType::MouseButtonReleased;
		}
	private:
		KeyCode m_MouseCode;
	};

	class CEEAPI MouseMoveEvent : public Event {
	public:
		MouseMoveEvent(float x, float y)
		 : m_X(x), m_Y(y)
		{
		}
		~MouseMoveEvent() = default;

		EventType CEECALL GetEventType() const override {
			return EventType::MouseMove;
		}

		const char* CEECALL GetName() const override {
			return "MouseMove";
		}

		int CEECALL GetCategoryFlags() const override {
			return EventCategoryInput | EventCategoryMouse;
		}

		std::string CEECALL ToString() const override {
			std::stringstream ss;
			ss << "Mouse move event (" << m_X << ", " << m_Y << ")";
			return ss.str();
		}

		static EventType GetStaticEventType() {
			return EventType::MouseMove;
		}
	private:
		float m_X, m_Y;
	};

	class CEEAPI MouseScrollEvent : public Event {
	public:
		MouseScrollEvent(float xOffset, float yOffset)
		 : m_XOffset(xOffset), m_YOffset(yOffset)
		{
		}
		~MouseScrollEvent() = default;

		EventType CEECALL GetEventType() const override {
			return EventType::MouseScroll;
		}

		const char* CEECALL GetName() const override {
			return "MouseScroll";
		}

		int CEECALL GetCategoryFlags() const override {
			return EventCategoryInput | EventCategoryMouse | EventCategoryMouseButton;
		}

		std::string CEECALL ToString() const override {
			std::stringstream ss;
			ss << "Mouse scroll event (" << m_XOffset << ", " << m_YOffset << ")";
			return ss.str();
		}

		static EventType GetStaticEventType() {
			return EventType::MouseScroll;
		}
	private:
		float m_XOffset, m_YOffset;
	};
}

#endif
