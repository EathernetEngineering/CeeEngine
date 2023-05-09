cmake_minimum_required(VERSION 3.2)

project(CeeEditor)

add_executable(CeeEditor editor.cpp)

target_link_libraries(CeeEditor PUBLIC CeeEngine)

install(TARGETS CeeEditor RUNTIME DESTINATION bin)
