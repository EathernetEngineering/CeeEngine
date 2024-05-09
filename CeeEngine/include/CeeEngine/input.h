#ifndef _CEE_ENGINE_INPUT_H
#define _CEE_ENGINE_INPUT_H

#include <CeeEngine/messageBus.h>
#include <CeeEngine/keyCodes.h>
#include <CeeEngine/window.h>

#include <map>
#include <memory>

namespace cee {
namespace Input {
	void Init(MessageBus* messageBus, std::shared_ptr<Window> window);
	void Shutdown();
	int GetKeyState(KeyCode keycode);
}
}

#endif
