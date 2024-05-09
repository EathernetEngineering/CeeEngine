#include <CeeEngine/input.h>
#include <CeeEngine/debugMessenger.h>

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-x11.h>

namespace cee {
namespace Input {
bool MessageHandler(Event& e);

static bool g_Initialized = false;

static MessageBus* g_MessageBus = NULL;
static std::shared_ptr<Window> g_Window = NULL;
static std::map<uint32_t, KeyCode> g_KeyMap;
static std::map<KeyCode, int> g_KeyStates;

static xkb_context* g_XkbContext = NULL;
static int32_t g_XkbDeviceID = 0;
static xkb_keymap* g_XkbKeymap = NULL;
static xkb_state* g_XkbState = NULL;

static uint8_t g_XbkFirstEvent;

xkb_keysym_t GetKeysymFromKeycode(xcb_keycode_t kc) {
	return xkb_state_key_get_one_sym(g_XkbState, kc);
}

static void FillKeyMap() {
	g_KeyMap[0xFF1B] = key::Escape;    // esc
	g_KeyMap[0xFFBE] = key::F1;    // F1
	g_KeyMap[0xFFBF] = key::F2;    // F2
	g_KeyMap[0xFFC0] = key::F3;    // F3
	g_KeyMap[0xFFC1] = key::F4;    // F4
	g_KeyMap[0xFFC2] = key::F5;    // F5
	g_KeyMap[0xFFC3] = key::F6;    // F6
	g_KeyMap[0xFFC4] = key::F7;    // F7
	g_KeyMap[0xFFC5] = key::F8;    // F8
	g_KeyMap[0xFFC6] = key::F9;    // F9
	g_KeyMap[0xFFC7] = key::F10;    // F10
	g_KeyMap[0xFFC8] = key::F11;    // F12
	g_KeyMap[0xFFC9] = key::F12;    // F12
	g_KeyMap[0xFF61] = key::PrintScreen;    // prt scr
	g_KeyMap[0xFF14] = key::ScrollLock;    // scroll lock
	g_KeyMap[0xFF13] = key::Pause;    // pause
	g_KeyMap[0x60] = key::GraveAccent;    // grave
	g_KeyMap[0x31] = key::D1;    // 1
	g_KeyMap[0x32] = key::D2;    // 2
	g_KeyMap[0x33] = key::D3;    // 3
	g_KeyMap[0x34] = key::D4;    // 4
	g_KeyMap[0x35] = key::D5;    // 5
	g_KeyMap[0x36] = key::D6;    // 6
	g_KeyMap[0x37] = key::D7;    // 7
	g_KeyMap[0x38] = key::D8;    // 8
	g_KeyMap[0x39] = key::D9;    // 9
	g_KeyMap[0x30] = key::D0;    // 0
	g_KeyMap[0x2D] = key::Minus;    // -
	g_KeyMap[0x3F] = key::Equal;    // =
	g_KeyMap[0xFF08] = key::Backspace;    // backspace
	g_KeyMap[0xFF63] = key::Insert;    // insert
	g_KeyMap[0xFF50] = key::Home;    // home
	g_KeyMap[0xFF55] = key::PageUp;    // page up
	g_KeyMap[0xFF7F] = key::NumLock;    // numlock
	g_KeyMap[0xFFAF] = key::KPDevide;    // kp div
	g_KeyMap[0xFFAA] = key::KPMultiply;    // kp mul
	g_KeyMap[0xFFAD] = key::KPSubtract;    // kp sub
	g_KeyMap[0xFF09] = key::Tab;    // tab
	g_KeyMap[0x71] = key::Q;    // q
	g_KeyMap[0x77] = key::W;    // w
	g_KeyMap[0x65] = key::E;    // e
	g_KeyMap[0x72] = key::R;    // r
	g_KeyMap[0x74] = key::T;    // t
	g_KeyMap[0x79] = key::Y;    // y
	g_KeyMap[0x75] = key::U;    // u
	g_KeyMap[0x69] = key::I;    // i
	g_KeyMap[0x6F] = key::O;    // o
	g_KeyMap[0x70] = key::P;    // p
	g_KeyMap[0x5B] = key::LeftBracekt;    // left bracket
	g_KeyMap[0x5D] = key::RightBracket;    // right bracket
	g_KeyMap[0x5C] = key::BackSlash;    // backslash
	g_KeyMap[0xFFFF] = key::Delete;    // delete
	g_KeyMap[0xFF57] = key::End;    // end
	g_KeyMap[0xFF56] = key::PageDown;    // pg dn
	g_KeyMap[0xFFB7] = key::KP7;    // kp 7
	g_KeyMap[0xFFB8] = key::KP8;    // kp 8
	g_KeyMap[0xFFB9] = key::KP9;    // kp 9
	g_KeyMap[0xFFAB] = key::KPAdd;    // kp add
	g_KeyMap[0xFFE5] = key::CapsLock;    // caps lock
	g_KeyMap[0x61] = key::A;    // a
	g_KeyMap[0x73] = key::S;    // s
	g_KeyMap[0x64] = key::D;    // d
	g_KeyMap[0x66] = key::F;    // f
	g_KeyMap[0x67] = key::G;    // g
	g_KeyMap[0x68] = key::H;    // h
	g_KeyMap[0x6A] = key::J;    // j
	g_KeyMap[0x6B] = key::K;    // k
	g_KeyMap[0x6C] = key::L;    // l
	g_KeyMap[0x3B] = key::Semicolon;    // semicolon
	g_KeyMap[0x27] = key::Apostrophe;    // apostrophe
	g_KeyMap[0xFF0D] = key::Enter;    // return
	g_KeyMap[0xFFB4] = key::KP4;    // kp 4
	g_KeyMap[0xFFB5] = key::KP5;    // kp 5
	g_KeyMap[0xFFB6] = key::KP6;    // kp 6
	g_KeyMap[0xFFE1] = key::LeftShift;    // l shift
	g_KeyMap[0x7A] = key::Z;    // z
	g_KeyMap[0x78] = key::X;    // x
	g_KeyMap[0x63] = key::C;    // c
	g_KeyMap[0x76] = key::V;    // v
	g_KeyMap[0x62] = key::B;    // b
	g_KeyMap[0x6E] = key::N;    // n
	g_KeyMap[0x6D] = key::M;    // m
	g_KeyMap[0x2C] = key::Comma;    // comma
	g_KeyMap[0x2E] = key::Period;    // period
	g_KeyMap[0x2F] = key::Slash;    // forward slash
	g_KeyMap[0xFFE2] = key::RigthShift;    // r shift
	g_KeyMap[0xFF52] = key::Up;    // up
	g_KeyMap[0xFFB1] = key::KP1;    // kp 1
	g_KeyMap[0xFFB2] = key::KP2;    // kp 2
	g_KeyMap[0xFFB3] = key::KP3;    // kp 3
	g_KeyMap[0xFF8D] = key::KPEnter;    // kp enter
	g_KeyMap[0xFFE3] = key::LeftControl;    // l ctrl
	g_KeyMap[0xFFEB] = key::LeftSuper;    // l super
	g_KeyMap[0xFFE9] = key::LeftAlt;    // l alt
	g_KeyMap[0x20] = key::Space;    // space
	g_KeyMap[0xFFEA] = key::RightAlt;    // r alt
	g_KeyMap[0xFFEC] = key::RightSuper;    // r super
	g_KeyMap[0xFF67] = key::Menu;    // menu
	g_KeyMap[0xFFE4] = key::RigthControl;    // r ctrl
	g_KeyMap[0xFF51] = key::Left;    // left
	g_KeyMap[0xFF54] = key::Down;    // down
	g_KeyMap[0xFF53] = key::Right;    // right
	g_KeyMap[0xFFB0] = key::KP0;    // kp 0
	g_KeyMap[0xFFAE] = key::KPDecimal;    // kp decimal
	// g_KeyMap[0x111] = mouse::ButtonLeft;    // Mouse button 0 (l click)
	// g_KeyMap[0x111] = mouse::ButtonRight;    // Mouse button 1 (r click)
	// g_KeyMap[0x111] = mouse::ButtonMiddle;    // Mouse button 2 (mmb)
	// g_KeyMap[0x111] = mouse::Button3;    // Mouse button 3
	// g_KeyMap[0x111] = mouse::Button4;    // Mouse button 4
	// g_KeyMap[0x111] = mouse::Button5;    // Mouse button 5
	// g_KeyMap[0x111] = mouse::Button6;    // Mouse button 6
	// g_KeyMap[0x111] = mouse::Button7;    // Mouse button 7
}

static void FillKeyStates() {
	g_KeyMap[key::Escape] = false;    // esc
	g_KeyMap[key::F1] = false;    // F1
	g_KeyMap[key::F2] = false;    // F2
	g_KeyMap[key::F3] = false;    // F3
	g_KeyMap[key::F4] = false;    // F4
	g_KeyMap[key::F5] = false;    // F5
	g_KeyMap[key::F6] = false;    // F6
	g_KeyMap[key::F7] = false;    // F7
	g_KeyMap[key::F8] = false;    // F8
	g_KeyMap[key::F9] = false;    // F9
	g_KeyMap[key::F10] = false;    // F10
	g_KeyMap[key::F11] = false;    // F12
	g_KeyMap[key::F12] = false;    // F12
	g_KeyMap[key::PrintScreen] = false;    // prt scr
	g_KeyMap[key::ScrollLock] = false;    // scroll lock
	g_KeyMap[key::Pause] = false;    // pause
	g_KeyMap[key::GraveAccent] = false;    // grave
	g_KeyMap[key::D1] = false;    // 1
	g_KeyMap[key::D2] = false;    // 2
	g_KeyMap[key::D3] = false;    // 3
	g_KeyMap[key::D4] = false;    // 4
	g_KeyMap[key::D5] = false;    // 5
	g_KeyMap[key::D6] = false;    // 6
	g_KeyMap[key::D7] = false;    // 7
	g_KeyMap[key::D8] = false;    // 8
	g_KeyMap[key::D9] = false;    // 9
	g_KeyMap[key::D0] = false;    // 0
	g_KeyMap[key::Minus] = false;    // -
	g_KeyMap[key::Equal] = false;    // =
	g_KeyMap[key::Backspace] = false;    // backspace
	g_KeyMap[key::Insert] = false;    // insert
	g_KeyMap[key::Home] = false;    // home
	g_KeyMap[key::PageUp] = false;    // page up
	g_KeyMap[key::NumLock] = false;    // numlock
	g_KeyMap[key::KPDevide] = false;    // kp div
	g_KeyMap[key::KPMultiply] = false;    // kp mul
	g_KeyMap[key::KPSubtract] = false;    // kp sub
	g_KeyMap[key::Tab] = false;    // tab
	g_KeyMap[key::Q] = false;    // q
	g_KeyMap[key::W] = false;    // w
	g_KeyMap[key::E] = false;    // e
	g_KeyMap[key::R] = false;    // r
	g_KeyMap[key::T] = false;    // t
	g_KeyMap[key::Y] = false;    // y
	g_KeyMap[key::U] = false;    // u
	g_KeyMap[key::I] = false;    // i
	g_KeyMap[key::O] = false;    // o
	g_KeyMap[key::P] = false;    // p
	g_KeyMap[key::LeftBracekt] = false;    // left bracket
	g_KeyMap[key::RightBracket] = false;    // right bracket
	g_KeyMap[key::BackSlash] = false;    // backslash
	g_KeyMap[key::Delete] = false;    // delete
	g_KeyMap[key::End] = false;    // end
	g_KeyMap[key::PageDown] = false;    // pg dn
	g_KeyMap[key::KP7] = false;    // kp 7
	g_KeyMap[key::KP8] = false;    // kp 8
	g_KeyMap[key::KP9] = false;    // kp 9
	g_KeyMap[key::KPAdd] = false;    // kp add
	g_KeyMap[key::CapsLock] = false;    // caps lock
	g_KeyMap[key::A] = false;    // a
	g_KeyMap[key::S] = false;    // s
	g_KeyMap[key::D] = false;    // d
	g_KeyMap[key::F] = false;    // f
	g_KeyMap[key::G] = false;    // g
	g_KeyMap[key::H] = false;    // h
	g_KeyMap[key::J] = false;    // j
	g_KeyMap[key::K] = false;    // k
	g_KeyMap[key::L] = false;    // l
	g_KeyMap[key::Semicolon] = false;    // semicolon
	g_KeyMap[key::Apostrophe] = false;    // apostrophe
	g_KeyMap[key::Enter] = false;    // return
	g_KeyMap[key::KP4] = false;    // kp 4
	g_KeyMap[key::KP5] = false;    // kp 5
	g_KeyMap[key::KP6] = false;    // kp 6
	g_KeyMap[key::LeftShift] = false;    // l shift
	g_KeyMap[key::Z] = false;    // z
	g_KeyMap[key::X] = false;    // x
	g_KeyMap[key::C] = false;    // c
	g_KeyMap[key::V] = false;    // v
	g_KeyMap[key::B] = false;    // b
	g_KeyMap[key::N] = false;    // n
	g_KeyMap[key::M] = false;    // m
	g_KeyMap[key::Comma] = false;    // comma
	g_KeyMap[key::Period] = false;    // period
	g_KeyMap[key::Slash] = false;    // forward slash
	g_KeyMap[key::RigthShift] = false;    // r shift
	g_KeyMap[key::Up] = false;    // up
	g_KeyMap[key::KP1] = false;    // kp 1
	g_KeyMap[key::KP2] = false;    // kp 2
	g_KeyMap[key::KP3] = false;    // kp 3
	g_KeyMap[key::KPEnter] = false;    // kp enter
	g_KeyMap[key::LeftControl] = false;    // l ctrl
	g_KeyMap[key::LeftSuper] = false;    // l super
	g_KeyMap[key::LeftAlt] = false;    // l alt
	g_KeyMap[key::Space] = false;    // space
	g_KeyMap[key::RightAlt] = false;    // r alt
	g_KeyMap[key::RightSuper] = false;    // r super
	g_KeyMap[key::Menu] = false;    // menu
	g_KeyMap[key::RigthControl] = false;    // r ctrl
	g_KeyMap[key::Left] = false;    // left
	g_KeyMap[key::Down] = false;    // down
	g_KeyMap[key::Right] = false;    // right
	g_KeyMap[key::KP0] = false;    // kp 0
	g_KeyMap[key::KPDecimal] = false;    // kp decimal
	// g_KeyStates[mouse::Button0] = false;    // Mouse button 0 (l click)
	// g_KeyStates[mouse::Button1] = false;    // Mouse button 1 (r click)
	// g_KeyStates[mouse::Button2] = false;    // Mouse button 2 (mmb)
	// g_KeyStates[mouse::Button3] = false;    // Mouse button 3
	// g_KeyStates[mouse::Button4] = false;    // Mouse button 4
	// g_KeyStates[mouse::Button5] = false;    // Mouse button 5
	// g_KeyStates[mouse::Button6] = false;    // Mouse button 6
	// g_KeyStates[mouse::Button7] = false;    // Mouse button 7
}

void Init(MessageBus* messageBus, std::shared_ptr<Window> window) {
	g_MessageBus = messageBus;
	g_MessageBus->RegisterMessageHandler(MessageHandler);
	g_Window = window;

	g_XkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (g_XkbContext == NULL) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Failed to create xkb context.");
		return;
	}

	int res = xkb_x11_setup_xkb_extension(g_Window->GetNativeConnection(),
										  XKB_X11_MIN_MAJOR_XKB_VERSION,
										  XKB_X11_MIN_MINOR_XKB_VERSION,
										  XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS,
										  NULL, NULL, &g_XbkFirstEvent, NULL);
	if (res == 0) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Failed to setup xkb X11 extension.");
		return;
	}

	g_XkbDeviceID = xkb_x11_get_core_keyboard_device_id(g_Window->GetNativeConnection());
	if (g_XkbDeviceID == 0) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Failed to setup get xkb X11 device ID.");
		return;
	}

	g_XkbKeymap = xkb_x11_keymap_new_from_device(g_XkbContext,
												 g_Window->GetNativeConnection(),
												 g_XkbDeviceID,
												 XKB_KEYMAP_COMPILE_NO_FLAGS);
	if (g_XkbKeymap == NULL) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Failed to get xkb keymap.");
		return;
	}

	g_XkbState = xkb_x11_state_new_from_device(g_XkbKeymap,
											   g_Window->GetNativeConnection(),
											   g_XkbDeviceID);
	if (g_XkbState == NULL) {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_ERROR,
										 "Failed to get xkb state.");
		return;
	}

	FillKeyStates();
	FillKeyMap();

	g_Initialized = true;
}

void Shutdown() {
	g_MessageBus = NULL;
	g_Window.reset();
	g_Initialized = false;
}

int GetKeyState(KeyCode keycode) {
	if (g_Initialized)
		return g_KeyStates[keycode];
	else {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_WARNING,
										 "Requesting key state without calling `Input::Init()`");
		return false;
	}
}

bool MessageHandler(Event& e) {
	if (g_Initialized) {
		if (e.IsInCategory(EventCategoryKeyboard)) {
			if (e.GetEventType() == EventType::KeyPressed) {
				KeyPressedEvent& event = dynamic_cast<KeyPressedEvent&>(e);
				xkb_keysym_t keysym = xkb_state_key_get_one_sym(g_XkbState, event.GetKeyCode());
				g_KeyStates[g_KeyMap[keysym]] = true;
			} else if (e.GetEventType() == EventType::KeyReleased) {
				KeyReleasedEvent& event = dynamic_cast<KeyReleasedEvent&>(e);
				xkb_keysym_t keysym = xkb_state_key_get_one_sym(g_XkbState, event.GetKeyCode());
				g_KeyStates[g_KeyMap[keysym]] = false;
			}
			return true;
		}
	}
	return false;
}
}
}

