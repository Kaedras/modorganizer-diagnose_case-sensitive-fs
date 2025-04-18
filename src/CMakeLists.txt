cmake_minimum_required(VERSION 3.30)

find_package(OpenGL REQUIRED)
find_package(mo2-cmake CONFIG REQUIRED)
find_package(mo2-uibase CONFIG REQUIRED)

set(PLUGIN_NAME diagnose_case-sensitive-fs)

add_library(${PLUGIN_NAME} SHARED)
target_include_directories(${PLUGIN_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
mo2_configure_plugin(${PLUGIN_NAME} WARNINGS 4)
target_link_libraries(${PLUGIN_NAME} PRIVATE mo2::uibase)
mo2_install_plugin(${PLUGIN_NAME})
