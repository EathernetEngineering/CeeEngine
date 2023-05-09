#include "CeeEngine/CeeEngineDebugMessenger.h"

#include <cstdio>

namespace cee {
	std::function<void(CeeErrorSeverity, const char*, void*)> DebugMessenger::s_Messenger = DebugMessenger::DefaultHandler;
	void* DebugMessenger::s_UserData = NULL;
	CeeErrorSeverity DebugMessenger::s_ReportErrorLevels = (CeeErrorSeverity)0b1111;

	void DebugMessenger::RegisterDebugMessenger(CeeErrorSeverity messageTypes,
												void* userData,
												std::function<void(CeeErrorSeverity, const char*, void*)> callback)
	{
		s_ReportErrorLevels = messageTypes;
		s_UserData = userData;
		s_Messenger = callback;
	}

	void DebugMessenger::DefaultHandler(CeeErrorSeverity severity, const char* message, void*)
	{
		switch (severity) {
			case CEE_ERROR_SEVERITY_DEBUG:
				fprintf(stderr, "[DEBUG] %s\n", message);
				break;

			case CEE_ERROR_SEVERITY_INFO:
				fprintf(stderr, "[INFO]  %s\n", message);
				break;

			case CEE_ERROR_SEVERITY_WARNING:
				fprintf(stderr, "[WARN]  %s\n", message);
				break;

			case CEE_ERROR_SEVERITY_ERROR:
				fprintf(stderr, "[ERROR] %s\n", message);
				break;

			default:
				fprintf(stderr, "[WARN]  An unknown error has occured.\n");
				fprintf(stderr, "[ERROR] %s\n", message);
		}
	}

	void DebugMessenger::PostDebugMessage(cee::CeeErrorSeverity serverity, const char* message)
	{
		if (serverity & s_ReportErrorLevels) {
			s_Messenger(serverity, message, s_UserData);
		}
	}


}
