cmake_minimum_required(VERSION 3.2)

project(CeeEngine LANGUAGES CXX VERSION 1.0.0)

option(CEE_ENGINE_STATIC_BUILD "Build engine as a shared library" OFF)

if(WIN32)
	message(STATUS ">>> WINDOWS BUILD")
elseif(UNIX AND NOT APPLE)
	message(STATUS ">>> LINUX BUILD")
	find_package(X11)
	if (NOT ${X11_FOUND})
		message(SEND_ERROR "Failed to find X11")
	endif()

	list(APPEND SOURCES linux_util.cpp)
	list(APPEND LIBRARIES m pthread)
	list(APPEND LIBRARIES ${X11_LIBRARIES} xkbcommon xkbcommon-x11)
	list(APPEND INCLUDES ${X11_INCLUDE_DIRS})
elseif(APPLE)
	message(STATUS ">>> APPLE BUILD")
else()
	message(FATAL_ERROR ">>> UNKNOWN PLATFORM")
endif()

find_package(Vulkan)
if(NOT ${Vulkan_FOUND})
	message(SEND_ERROR "Failed to find Vulkan")
endif()

list(APPEND SOURCES application.cpp layer.cpp timestep.cpp window.cpp renderer.cpp messageBus.cpp debugLayer.cpp debugMessenger.cpp libimpl.cpp input.cpp renderer2D.cpp renderer3D.cpp camera.cpp assetManager.cpp)
list(APPEND INCLUDES include/ ${Vulkan_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/vendor/glm/include)
list(APPEND LIBRARIES ${Vulkan_LIBRARY})

list(APPEND SOURCES ${CMAKE_SOURCE_DIR}/vendor/tracy/public/TracyClient.cpp)
list(APPEND INCLUDES ${CMAKE_SOURCE_DIR}/vendor/tracy/public/tracy/)

if (CEE_ENGINE_STATIC_BUILD)
	add_library(CeeEngine STATIC ${SOURCES})
else()
	add_library(CeeEngine SHARED ${SOURCES})
endif(CEE_ENGINE_STATIC_BUILD)

target_include_directories(CeeEngine PUBLIC ${INCLUDES})
target_link_libraries(CeeEngine PUBLIC ${LIBRARIES})

target_include_directories(CeeEngine INTERFACE include/)

target_compile_definitions(CeeEngine PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_RADIANS TRACY_ENABLE)

install(TARGETS CeeEngine RUNTIME DESTINATION bin)
