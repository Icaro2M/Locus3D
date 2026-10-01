# SPDX-FileCopyrightText: 2026 Icaro2M
# SPDX-License-Identifier: Apache-2.0

add_executable(Locus3DDemo)

add_subdirectory(kernel)
add_subdirectory(graphics)
add_subdirectory(editor)
add_subdirectory(application)

if(LOCUS3D_BUILD_QT)
    find_package(Qt6 COMPONENTS Core Gui Widgets OpenGLWidgets REQUIRED)
    message(STATUS "Locus3D desktop: Qt ${Qt6_VERSION} from ${Qt6_DIR}")
    add_subdirectory(overlay)
endif()

target_sources(Locus3DDemo
    PRIVATE
        "launcher/main.cpp"
)

target_sources(Locus3DDemo
    PRIVATE
        "${GLAD_DIR}/src/glad.c"
        "${IMGUI_DIR}/imgui.cpp"
        "${IMGUI_DIR}/imgui_draw.cpp"
        "${IMGUI_DIR}/imgui_tables.cpp"
        "${IMGUI_DIR}/imgui_widgets.cpp"
        "${IMGUI_DIR}/imgui_impl_glfw.cpp"
        "${IMGUI_DIR}/imgui_impl_opengl3.cpp"
)

target_include_directories(Locus3DDemo
    PRIVATE
        ${LOCUS3D_SOURCE_DIR}
        ${GLAD_DIR}/include
        ${IMGUI_DIR}
)

target_link_libraries(Locus3DDemo
    PRIVATE
        Locus3D::Application
        Locus3D::Graphics
        Locus3D::Geometry
        Locus3D::Editor
)

target_compile_definitions(Locus3DDemo
    PRIVATE
        _CRT_SECURE_NO_WARNINGS
)

add_custom_command(
    TARGET Locus3DDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${LOCUS3D_ASSETS_DIR}"
        "$<TARGET_FILE_DIR:Locus3DDemo>/assets"
)
