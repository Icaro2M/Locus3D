# SPDX-FileCopyrightText: 2026 Icaro2M
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)

project(Locus3D
    VERSION 0.1.0
    LANGUAGES C CXX
)

# Global C++ configuration

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Project paths

set(LOCUS3D_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(LOCUS3D_SOURCE_DIR ${LOCUS3D_ROOT_DIR}/source)
set(LOCUS3D_ASSETS_DIR ${LOCUS3D_ROOT_DIR}/assets)
set(LOCUS3D_EXTERN_DIR ${LOCUS3D_ROOT_DIR}/extern)

# Build options

option(LOCUS3D_BUILD_TESTS "Build Locus3D tests" ON)

# External dependencies

set(GLAD_DIR     ${LOCUS3D_EXTERN_DIR}/glad)
set(GLFW_DIR     ${LOCUS3D_EXTERN_DIR}/glfw)
set(GLM_DIR      ${LOCUS3D_EXTERN_DIR}/glm)
set(IMGUI_DIR    ${LOCUS3D_EXTERN_DIR}/imgui)
set(NLOHMANN_DIR ${LOCUS3D_EXTERN_DIR}/nlohmann)
set(STB_DIR      ${LOCUS3D_EXTERN_DIR}/stb)

# Platform libraries

if(WIN32)
    set(LOCUS3D_PLATFORM_LIBS
        opengl32.lib
        Comdlg32.lib
        Comctl32.lib
    )
endif()

# Subdirectories

add_subdirectory(source)

if(LOCUS3D_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Documentation

find_package(Doxygen QUIET)

if(DOXYGEN_FOUND)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${LOCUS3D_ROOT_DIR}/Doxyfile
        WORKING_DIRECTORY ${LOCUS3D_ROOT_DIR}
        COMMENT "Generating Locus3D API documentation"
        VERBATIM
    )
endif()
