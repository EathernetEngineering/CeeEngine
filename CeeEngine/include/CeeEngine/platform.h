#ifndef CEE_ENGINE_PLATFORM_H
#define CEE_ENGINE_PLATFORM_H

#if defined(_WIN32)
#	if defined(_WIN64)
#		define CEE_PLATFORM_WIN64 1
#       define CEE_PLATFORM_WINDOWS 1
#		error "x64 Windows not supported"
#	else
#		define CEE_PLATFORM_WIN32 1
#       define CEE_PLATFORM_WINDOWS 1
#		error "x86 Windows not supported"
#	endif
#elif defined(__APPLE__) || defined(__MACH__)
#	include "TargetConditionals.h"
#	if TARGET_IPHONE_SIMULATOR == 1
#		define CEE_PLATFORM_IOS_SIMULATOR 1
#		error "IOS simulator not supported"
#	elif TARGET_OS_IPHONE == 1
#		define CEE_PLATFORM_IOS 1
#	elif TARGET_OS_MAC == 1
#		define CEE_PLATFORM_MAC 1
#		error "MacOS not supported"
#	else
#		error "Unknown Apple platform"
#	endif
#elif defined(__ANDROID__)
#	define CEE_PLATFORM_ANDROID 1
#	error "Android not supported"
#elif defined(__linux__)
#	define CEE_PLATFORM_LINUX 1
#else
#	error "Unknown platform"
#endif

#if defined(CEE_PLATFORM_WINDOWS)
#   include "Windows.h"
#endif

#if defined(CEE_PLATFORM_WIN32)
#	define CEECALL __stdcall
#elif defined(CEE_PLATFORM_LINUX)
#	if defined(__x86_64)
#		define CEECALL
#	else
#		define CEECALL __attribute__((stdcall))
#	endif
#endif

#if defined(CEE_PLATFORM_WINDOWS)
#   if defined(CEE_ENGINE_COMPILE)
#       define CEEAPI __declspec(dllexport)
#   else
#       define CEEAPI __declspec(dllimport)
#   endif
#elif defined(CEE_PLATFORM_LINUX)
#   if defined(CEE_ENGINE_COMPILE)
#       define CEEAPI
#   else
#       define CEEAPI
#   endif
#endif

#if defined(CEE_PLATFORM_WINDOWS)
#   define CEE_ATOMIC _Atomic
#elif defined(CEE_PLATFORM_LINUX)
#   define CEE_ATOMIC _Atomic
#endif

#endif
