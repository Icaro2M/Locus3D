# SPDX-FileCopyrightText: 2026 Icaro2M
# SPDX-License-Identifier: Apache-2.0

add_executable(Locus3D)

add_subdirectory(kernel)
add_subdirectory(graphics)

target_sources(Locus3D PRIVATE
    "launcher/main.cpp"
)

target_sources(Locus3D PRIVATE
    "${GLAD_DIR}/src/glad.c"

    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/imgui_impl_opengl3.cpp"
)

target_include_directories(Locus3D PRIVATE
    ${LOCUS3D_SOURCE_DIR}

    ${GLAD_DIR}/include
    ${IMGUI_DIR}
)

target_link_libraries(Locus3D PRIVATE
    Locus3D::Graphics
    Locus3D::Geometry
)

target_compile_definitions(Locus3D PRIVATE
    _CRT_SECURE_NO_WARNINGS
)

add_custom_command(TARGET Locus3D POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${LOCUS3D_ASSETS_DIR}"
        "$<TARGET_FILE_DIR:Locus3D>/assets"
)
