#include <CeeEngine/debugMessenger.h>

#include <cstdio>

namespace cee {
std::function<void(CeeErrorSeverity, const char*, void*)> DebugMessenger::s_Messenger = DebugMessenger::DefaultHandler;
void* DebugMessenger::s_UserData = NULL;
CeeErrorSeverity DebugMessenger::s_ReportErrorLevels = (CeeErrorSeverity)(ERROR_SEVERITY_DEBUG | ERROR_SEVERITY_INFO | ERROR_SEVERITY_WARNING | ERROR_SEVERITY_ERROR);

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
		case ERROR_SEVERITY_DEBUG:
			fprintf(stderr, "\e[0;90m[DEBUG] %s\e[0m\n", message);
			break;

		case ERROR_SEVERITY_INFO:
			fprintf(stderr, "\e[0;32m[INFO]  %s\e[0m\n", message);
			break;

		case ERROR_SEVERITY_WARNING:
			fprintf(stderr, "\e[0;33m[WARN]  %s\e[0m\n", message);
			break;

		case ERROR_SEVERITY_ERROR:
			fprintf(stderr, "\e[0;31m[ERROR] %s\e[0m\n", message);
			break;

		default:
			fprintf(stderr, "\e[0;33m[WARN]  An unknown error has occured.\e[0m\n");
			fprintf(stderr, "\e[0;31m[ERROR] %s\e[0m\n", message);
	}
}

void DebugMessenger::PostDebugMessage(cee::CeeErrorSeverity serverity, const char* message)
{
	if (serverity & s_ReportErrorLevels) {
		s_Messenger(serverity, message, s_UserData);
	}
}
}

