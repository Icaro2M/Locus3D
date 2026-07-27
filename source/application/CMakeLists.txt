# SPDX-FileCopyrightText: 2026 Icaro2M
# SPDX-License-Identifier: Apache-2.0

add_library(locus3d_application STATIC)
add_library(Locus3D::Application ALIAS locus3d_application)

target_sources(locus3d_application
    PRIVATE
        "Application.h"
        "ApplicationConfig.h"
        "ApplicationError.h"
        "ApplicationResult.h"
        "document/DocumentId.h"
        "document/DocumentManager.h"
        "document/DocumentManager.cpp"
        "document/DocumentSession.h"
        "document/DocumentSession.cpp"
        "input/InputEvent.h"
        "input/InputState.h"
        "input/InputState.cpp"
        "input/InputCapture.h"
        "input/InputRouter.h"
        "input/InputRouter.cpp"
        "viewport/EditorViewport.h"
        "viewport/EditorViewport.cpp"
        "runtime/ApplicationRuntime.h"
        "runtime/ApplicationRuntime.cpp"
        "runtime/ApplicationState.h"
        "runtime/FrameContext.h"
        "runtime/FrameClock.h"
        "runtime/FrameClock.cpp"
        "window/ApplicationWindow.h"
        "window/ApplicationWindow.cpp"
)

target_include_directories(locus3d_application
    PUBLIC
        ${LOCUS3D_SOURCE_DIR}
)

target_compile_features(locus3d_application
    PUBLIC
        cxx_std_17
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
