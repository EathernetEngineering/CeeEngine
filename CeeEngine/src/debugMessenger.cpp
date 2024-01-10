/* Implementation of message system.
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

#include <CeeEngine/debugMessenger.h>

#include <cstdio>
#include <cstdarg>

#ifndef WORK_BUFFER_SIZE
# define WORK_BUFFER_SIZE 1000lu
#endif

namespace cee {
	std::function<void(ErrorSeverity, const char*, void*)> DebugMessenger::s_Messenger = DebugMessenger::DefaultHandler;
	void* DebugMessenger::s_UserData = NULL;
	ErrorSeverity DebugMessenger::s_ReportErrorLevels = (ErrorSeverity)0b1111;

	void DebugMessenger::RegisterDebugMessenger(ErrorSeverity messageTypes,
												void* userData,
												std::function<void(ErrorSeverity, const char*, void*)> callback)
	{
		s_ReportErrorLevels = messageTypes;
		s_UserData = userData;
		s_Messenger = callback;
	}

	void DebugMessenger::DefaultHandler(ErrorSeverity severity, const char* message, void*)
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

	void DebugMessenger::PostDebugMessage(::cee::ErrorSeverity severity, const char* fmt...)
	{
		va_list args;
		va_start(args, fmt);

		char* str = static_cast<char*>(malloc(WORK_BUFFER_SIZE));
		size_t strSize = vsnprintf(str, WORK_BUFFER_SIZE, fmt, args);
		if ((strSize + 1) > WORK_BUFFER_SIZE) {
			str = static_cast<char*>(realloc(str, strSize));
			if (str == nullptr) {
				std::exit(-1);
			}
			vsprintf(str, fmt, args);
		}

		va_end(args);

		if (severity & s_ReportErrorLevels) {
			s_Messenger(severity, str, s_UserData);
		}
		free(str);
	}


}
