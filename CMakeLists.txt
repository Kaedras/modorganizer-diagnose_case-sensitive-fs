cmake_minimum_required(VERSION 3.30)

if(DEFINED DEPENDENCIES_DIR)
    include(${DEPENDENCIES_DIR}/modorganizer_super/cmake_common/mo2.cmake)
else()
    include(${CMAKE_CURRENT_LIST_DIR}/../cmake_common/mo2.cmake)
endif()

project(diagnose_case-sensitive-fs)
add_subdirectory(src)
