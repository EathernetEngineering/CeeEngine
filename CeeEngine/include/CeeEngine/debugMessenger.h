/* Decleration of message system.
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

#ifndef CEE_ENGINE_DEBUG_MESSENGER_H
#define CEE_ENGINE_DEBUG_MESSENGER_H

#include <functional>

#include <vulkan/vulkan.h>

namespace cee {
	enum ErrorSeverity {
		ERROR_SEVERITY_DEBUG    = 1 << 0,
		ERROR_SEVERITY_INFO     = 1 << 1,
		ERROR_SEVERITY_WARNING  = 1 << 2,
		ERROR_SEVERITY_ERROR    = 1 << 3
	};

	typedef void(*PFN_CeeDebugMessengerCallback)(ErrorSeverity, const char*, void*);

	class DebugMessenger {
	public:
		static void RegisterDebugMessenger(ErrorSeverity messageTypes,
										   void* userData,
										   std::function<void(ErrorSeverity, const char*, void*)> callback);

	private:
		static void DefaultHandler(ErrorSeverity severity, const char* message, void*);

		static std::function<void(ErrorSeverity, const char*, void*)> s_Messenger;
		static void* s_UserData;
		static ErrorSeverity s_ReportErrorLevels;

	private:

		friend VkBool32 vulkanDebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
													 VkDebugUtilsMessageTypeFlagsEXT messageType,
													 const VkDebugUtilsMessengerCallbackDataEXT* messageData,
													 void* userData);

	public:
		static void PostDebugMessage(ErrorSeverity severity, const char* fmt...);

		template<typename... Args>
		static void PostAssertMessage(ErrorSeverity severity, const char* prefix, Args&&... args);
	};

	template<typename... Args>
	void DebugMessenger::PostAssertMessage(ErrorSeverity severity, const char* prefix, Args&&... args) {
		DebugMessenger::PostDebugMessage(severity, "%s: %s", prefix, std::forward<Args>(args)...);
	}

	template<>
	inline void DebugMessenger::PostAssertMessage(ErrorSeverity severity, const char* prefix) {
		DebugMessenger::PostDebugMessage(severity, prefix);
	}
}

#endif
