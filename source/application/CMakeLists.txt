# SPDX-FileCopyrightText: 2026 Icaro2M
# SPDX-License-Identifier: Apache-2.0

add_library(locus3d_application STATIC)
add_library(Locus3D::Application ALIAS locus3d_application)

target_sources(locus3d_application
    PRIVATE
        
)

target_include_directories(locus3d_application
    PUBLIC
        ${LOCUS3D_SOURCE_DIR}
        ${GLM_DIR}/include
)

target_link_libraries(locus3d_application
    PUBLIC
        Locus3D::Editor
        Locus3D::Graphics
)

target_compile_definitions(locus3d_application
    PRIVATE
        _CRT_SECURE_NO_WARNINGS
)
