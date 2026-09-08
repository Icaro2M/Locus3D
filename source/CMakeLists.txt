# SPDX-FileCopyrightText: 2026 Icaro2M
# SPDX-License-Identifier: Apache-2.0

# ==============================================================================
# 1. CONFIGURAÇÕES DO ECOSSISTEMA QT 6 E C++ MODERN0
# ==============================================================================
# Força o uso do C++20 para garantir que o backend matemátivo não falhe
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# [CORREÇÃO CRÍTICA] Vacina contra colisão de macros no MSVC (Impede o Windows 
# de corromper as bibliotecas <algorithm> e <cmath> do backend).
if(MSVC)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN _USE_MATH_DEFINES)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets)

# ==============================================================================
# 2. CARREGAMENTO DOS MÓDULOS DO KERNEL (BACKEND)
# ==============================================================================
add_subdirectory(kernel)
add_subdirectory(graphics)
add_subdirectory(editor)
add_subdirectory(application)

# ==============================================================================
# 3. ALVO PRINCIPAL: INTERFACE GRÁFICA (FRONTEND QT 6)
# ==============================================================================
add_executable(Locus3D)

set_target_properties(Locus3D PROPERTIES WIN32_EXECUTABLE ON)

target_sources(Locus3D
    PRIVATE
        "launcher/main_qt.cpp"
        "gui/MainWindow.h"
        "gui/MainWindow.cpp"
        "gui/MainWindow.ui"
        "gui/Locus3DViewport.h"
        "gui/Locus3DViewport.cpp"
        "${GLAD_DIR}/src/glad.c"
)

target_include_directories(Locus3D
    PRIVATE
        ${LOCUS3D_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/gui
        ${GLAD_DIR}/include
)

target_link_libraries(Locus3D
    PRIVATE
        Locus3D::Application
        Locus3D::Graphics
        Locus3D::Geometry
        Locus3D::Editor
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::OpenGLWidgets
)

target_compile_definitions(Locus3D PRIVATE _CRT_SECURE_NO_WARNINGS)

add_custom_command(
    TARGET Locus3D POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${LOCUS3D_ASSETS_DIR}"
        "$<TARGET_FILE_DIR:Locus3D>/assets"
)

# ==============================================================================
# 4. ALVO SECUNDÁRIO: SMOKE TESTS DO BACKEND (ISOLAMENTO)
# ==============================================================================
add_executable(Locus3D_SmokeTests
    "launcher/main.cpp"
    "${GLAD_DIR}/src/glad.c"
)

target_include_directories(Locus3D_SmokeTests PRIVATE
    ${LOCUS3D_SOURCE_DIR}
    ${GLAD_DIR}/include
)

target_link_libraries(Locus3D_SmokeTests PRIVATE
    Locus3D::Application
    Locus3D::Geometry
    Locus3D::Graphics
    Locus3D::Editor
)

target_compile_definitions(Locus3D_SmokeTests PRIVATE _CRT_SECURE_NO_WARNINGS)  