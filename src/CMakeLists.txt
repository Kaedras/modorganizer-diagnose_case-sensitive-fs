cmake_minimum_required(VERSION 3.30)

find_package(OpenGL REQUIRED)
find_package(mo2-cmake CONFIG REQUIRED)
find_package(mo2-uibase CONFIG REQUIRED)

add_library(diagnose_case-sensitive-fs SHARED)
mo2_configure_plugin(diagnose_case-sensitive-fs WARNINGS OFF)
target_link_libraries(diagnose_case-sensitive-fs PRIVATE mo2::uibase)
mo2_install_plugin(diagnose_case-sensitive-fs)
