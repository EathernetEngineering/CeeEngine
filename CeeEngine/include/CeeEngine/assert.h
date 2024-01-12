/* Decleration of CeeEngine assert functions.
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

#if !defined(NDEBUG)
# define CEE_ENABLE_ASSERT
#endif

#define CEE_ENABLE_VERIFY

// Check for support of __VA_OPT__, this is supported by MSVC and GCC, but not Clang
#define VA_OPT_SUPPORT_THRID_ARG(a, b, c, ...) c
#define INTERNAL_VA_OPT_SUPPORT(...) VA_OPT_SUPPORT_THRID_ARG(__VA_OPT__(,),true,false)
#define VA_OPT_SUPPORT INTERNAL_VA_OPT_SUPPORT(?)

#if defined(CEE_ENABLE_ASSERT)
#	if VA_OPT_SUPPORT == true
#		define INTERNAL_CEE_ASSERT(...) do { \
			::cee::DebugMessenger::PostAssertMessage(::cee::ERROR_SEVERITY_ERROR, "Assertion Failed" __VA_OPT__(,) __VA_ARGS__); \
		} while (0)
#	else
#		define INTERNAL_CEE_ASSERT(...) do { \
			::cee::DebugMessenger::PostAssertMessage(::cee::ERROR_SEVERITY_ERROR, "Assertion Failed", #__VA_ARGS__); \
		} while (0)
#	endif
# define CEE_ASSERT(x, ...) do { if (!(x)) { INTERNAL_CEE_ASSERT(__VA_ARGS__); CEE_DEBUG_BREAK(); } } while(0)
#else
#	define CEE_ASSERT(x, ...)
#endif

#if defined(CEE_ENABLE_VERIFY)
#	if VA_OPT_SUPPORT == true
#		define INTERNAL_CEE_VERIFY(...) do { \
			::cee::DebugMessenger::PostAssertMessage(::cee::ERROR_SEVERITY_ERROR, "Assertion Failed: " __VA_OPT__(,) __VA_ARGS__); \
		} while (0)
#	else
#		define INTERNAL_CEE_VERIFY(...) do { \
			::cee::DebugMessenger::PostAssertMessage(::cee::ERROR_SEVERITY_ERROR, "Assertion Failed: ", #__VA_ARGS__); \
		} while (0)
#	endif
#	define CEE_VERIFY(x, ...) do { if (!(x)) { INTERNAL_CEE_VERIFY(__VA_ARGS__); CEE_DEBUG_BREAK(); } } while(0)
#else
#	define CEE_VERIFY(x, ...)
#endif

#endif
