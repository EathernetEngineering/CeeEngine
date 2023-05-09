#ifndef CEE_ENGINE_DEBUG_MESSENGER_H
#define CEE_ENGINE_DEBUG_MESSENGER_H

#include <functional>

#include "vulkan/vulkan.h"

namespace cee {

	/* Forward declerations. */
	class Application;
	class Window;
	class Renderer;
	class MessageBus;
	class DebugLayer;
	/* end forward declerations. */

	enum CeeErrorSeverity {
		CEE_ERROR_SEVERITY_DEBUG    = 1 << 0,
		CEE_ERROR_SEVERITY_INFO     = 1 << 1,
		CEE_ERROR_SEVERITY_WARNING  = 1 << 2,
		CEE_ERROR_SEVERITY_ERROR    = 1 << 3
	};

	typedef void(*PFN_CeeDebugMessengerCallback)(CeeErrorSeverity, const char*, void*);

	class DebugMessenger {
	public:
		static void RegisterDebugMessenger(CeeErrorSeverity messageTypes,
										   void* userData,
										   std::function<void(CeeErrorSeverity, const char*, void*)> callback);

	private:
		static void DefaultHandler(CeeErrorSeverity severity, const char* message, void*);

		static std::function<void(CeeErrorSeverity, const char*, void*)> s_Messenger;
		static void* s_UserData;
		static CeeErrorSeverity s_ReportErrorLevels;

	private:
		friend Application;
		friend Window;
		friend Renderer;
		friend MessageBus;
		friend DebugLayer;

		friend VkBool32 vulkanDebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
													 VkDebugUtilsMessageTypeFlagsEXT messageType,
													 const VkDebugUtilsMessengerCallbackDataEXT* messageData,
													 void* userData);

	private:
		static void PostDebugMessage(CeeErrorSeverity serverity, const char* message);
	};
}

#endif
