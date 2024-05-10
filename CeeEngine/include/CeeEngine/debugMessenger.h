#ifndef CEE_ENGINE_DEBUG_MESSENGER_H
#define CEE_ENGINE_DEBUG_MESSENGER_H

#include <cstdlib>
#include <functional>

#include <utility>
#include <cstdio>

#include <vulkan/vulkan.h>

namespace cee {
enum CeeErrorSeverity {
	ERROR_SEVERITY_DEBUG    = 1 << 0,
	ERROR_SEVERITY_INFO     = 1 << 1,
	ERROR_SEVERITY_WARNING  = 1 << 2,
	ERROR_SEVERITY_ERROR    = 1 << 3
};

typedef void(*PFN_CeeDebugMessengerCallback)(CeeErrorSeverity, const char*, void*);

class DebugMessenger {
public:
	static void RegisterDebugMessenger(CeeErrorSeverity messageTypes,
									   void* userData,
									   std::function<void(CeeErrorSeverity, const char*, void*)> callback);
	static void SetReportLevels(CeeErrorSeverity levels) { s_ReportErrorLevels = levels; }

private:
	static void DefaultHandler(CeeErrorSeverity severity, const char* message, void*);

	static std::function<void(CeeErrorSeverity, const char*, void*)> s_Messenger;
	static void* s_UserData;
	static CeeErrorSeverity s_ReportErrorLevels;

private:
	friend VkBool32 vulkanDebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
												 VkDebugUtilsMessageTypeFlagsEXT messageType,
												 const VkDebugUtilsMessengerCallbackDataEXT* messageData,
												 void* userData);

public:
	template<typename... Args>
	static void PostDebugMessage(CeeErrorSeverity serverity, const char* fmt, Args&&... args)
	{
		if (serverity & s_ReportErrorLevels) {
			size_t messageLen = snprintf(nullptr, 0, fmt, std::forward<Args>(args)...);
			char* message = reinterpret_cast<char*>(malloc(messageLen + 1));
			snprintf(message, messageLen + 1, fmt, std::forward<Args>(args)...);
			s_Messenger(serverity, message, s_UserData);
		}
	}
};
}

#endif
