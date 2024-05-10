#ifndef _CEE_ENGINE_ASSERT_H
#define _CEE_ENGINE_ASSERT_H

#include <CeeEngine/platform.h>
#include <CeeEngine/debugMessenger.h>

#if defined(CEE_PLATFORM_WINDOWS)
#	define CEE_DEBUG_BREAK() __debugbreak()
#elif defined(CEE_PLATFORM_LINUX)
#	include <signal.h>
#	define CEE_DEBUG_BREAK() raise(SIGINT)
#endif

#ifndef NDEBUG
# define CEE_ENABLE_ASSERT
#endif

#define CEE_ENABLE_VERIFY

#if defined(CEE_ENABLE_ASSERT)
#	define CEE_ASSERT(x, msg) do { \
		if (!(x)) { \
			::cee::DebugMessenger::PostDebugMessage(::cee::ERROR_SEVERITY_ERROR, "Assertion \"%s\" failed %s:%u", #x, __FILE__, __LINE__); \
			::cee::DebugMessenger::PostDebugMessage(::cee::ERROR_SEVERITY_ERROR, "Message: %s", #msg); \
			CEE_DEBUG_BREAK(); \
		} \
	} while (0)
#else
#	define CEE_ASSERT(x, msg)
#endif

#if defined(CEE_ENABLE_VERIFY)
#	define CEE_VERIFY(x, msg) do { \
	if (!(x)) { \
			::cee::DebugMessenger::PostDebugMessage(::cee::ERROR_SEVERITY_ERROR, "Assertion \"%s\" failed %s:%u", #x, __FILE__, __LINE__); \
			::cee::DebugMessenger::PostDebugMessage(::cee::ERROR_SEVERITY_ERROR, "Message: %s", #msg); \
			CEE_DEBUG_BREAK(); \
		} \
	} while (0)
#else
#	define CEE_VERIFY(x, msg)
#endif

#endif
