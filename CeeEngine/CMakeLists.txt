#  Build file for engine library.
#  Copyright (C) 2023-2024  Chloe Eather.
#
#  This file is part of CeeEngine.
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.5)

project(CeeEngine LANGUAGES CXX VERSION 1.0.0)

option(CEE_ENGINE_STATIC_BUILD "Build engine as a shared library" OFF)
option(CEE_ENGINE_USE_AFTERMATH "compile with compatibility with Nvidia Nsight" ON)

if(WIN32)
	message(STATUS ">>> WINDOWS BUILD")
	list(APPEND SOURCES src/windows_util.cpp)
elseif(UNIX AND NOT APPLE)
	message(STATUS ">>> LINUX BUILD")
	include(GNUInstallDirs)

	find_package(X11)
	if (NOT ${X11_FOUND})
		message(SEND_ERROR "Failed to find X11")
	endif()

	list(APPEND LIBRARIES m pthread)
	list(APPEND LIBRARIES ${X11_LIBRARIES} xkbcommon xkbcommon-x11)
	list(APPEND INCLUDES ${X11_INCLUDE_DIRS})
	list(APPEND DEFINES VK_USE_PLATFORM_XCB_KHR)
	list(APPEND SOURCES src/linux_util.cpp)
elseif(APPLE)
	message(STATUS ">>> APPLE BUILD")
else()
	message(FATAL_ERROR ">>> UNKNOWN PLATFORM")
endif()

find_package(Vulkan)
if(NOT ${Vulkan_FOUND})
	message(SEND_ERROR "Failed to find Vulkan")
endif()

list(APPEND SOURCES src/application.cpp src/layer.cpp src/timestep.cpp src/window.cpp src/messageBus.cpp src/debugLayer.cpp src/debugMessenger.cpp src/libimpl.cpp src/input.cpp src/camera.cpp src/util.cpp)
list(APPEND SOURCES src/renderer.cpp src/renderer/buffers.cpp src/renderer2D.cpp src/renderer3D.cpp src/renderer/renderer.cpp src/renderer/context.cpp src/renderer/swapchain.cpp src/renderer/device.cpp)

list(APPEND INSTALLHEADERS include/CeeEngine/application.h include/CeeEngine/assert.h include/CeeEngine/camera.h include/CeeEngine/debugMessenger.h include/CeeEngine/event.h include/CeeEngine/input.h include/CeeEngine/keyCodes.h include/CeeEngine/layer.h include/CeeEngine/platform.h include/CeeEngine/timestep.h include/CeeEngine/util.h include/CeeEngine/window.h)

list(APPEND INCLUDES include/ ${Vulkan_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/vendor/glm/include)
list(APPEND LIBRARIES ${Vulkan_LIBRARY})


list(APPEND SOURCES ${CMAKE_SOURCE_DIR}/vendor/tracy/public/TracyClient.cpp)
list(APPEND INCLUDES ${CMAKE_SOURCE_DIR}/vendor/tracy/public/tracy/)

if (CEE_ENGINE_USE_AFTERMATH)
	list(APPEND LIBRARIES ${CMAKE_SOURCE_DIR}/vendor/NVIDIA_Nsight_Aftermath/lib/x64/libGFSDK_Aftermath_Lib.x64.so)
	list(APPEND INCLUDES ${CMAKE_SOURCE_DIR}/vendor/NVIDIA_Nsight_Aftermath/include)
	list(APPEND DEFINES CEE_USE_AFTERMATH)
	list(APPEND SOURCES src/renderer/aftermathCrashTracker.cpp src/renderer/aftermathShaderDatabase.cpp)
	message(STATUS "Building with NVIDIA Aftermath")

endif (CEE_ENGINE_USE_AFTERMATH)

if (CEE_ENGINE_STATIC_BUILD)
	add_library(CeeEngine STATIC ${SOURCES})
	message(STATUS "Building cee engine static library")
else()
	add_library(CeeEngine SHARED ${SOURCES})
	message(STATUS "Building cee engine shared library")
endif(CEE_ENGINE_STATIC_BUILD)

set_property(TARGET CeeEngine PROPERTY CXX_STANDARD 20)
set_property(TARGET CeeEngine PROPERTY CXX_STANDARD_REQUIRED 20)
set_property(TARGET CeeEngine PROPERTY PUBLIC_HEADER ${INSTALLHEADERS})

target_include_directories(CeeEngine PUBLIC ${INCLUDES})
target_link_libraries(CeeEngine PUBLIC ${LIBRARIES})

target_include_directories(CeeEngine INTERFACE include/)

target_compile_definitions(CeeEngine PUBLIC ${DEFINES} GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_RADIANS TRACY_ENABLE)

install(TARGETS CeeEngine LIBRARY PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CeeEngine)
