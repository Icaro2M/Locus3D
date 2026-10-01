add_executable(Locus3D WIN32
    main.cpp
    EditorBridge.h EditorBridge.cpp
    LocusViewport.h LocusViewport.cpp
    MainWindow.h MainWindow.cpp
    OutlinerWidget.h OutlinerWidget.cpp
    TransformWidget.h TransformWidget.cpp
    ManufacturingWidget.h ManufacturingWidget.cpp
    ${GLAD_DIR}/src/glad.c
)
set_target_properties(Locus3D PROPERTIES AUTOMOC ON)
target_include_directories(Locus3D PRIVATE ${LOCUS3D_SOURCE_DIR} ${GLAD_DIR}/include)
target_link_libraries(Locus3D PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGLWidgets
    Locus3D::Application Locus3D::Graphics Locus3D::Editor Locus3D::Geometry
)
target_compile_definitions(Locus3D PRIVATE _CRT_SECURE_NO_WARNINGS)
add_custom_command(TARGET Locus3D POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${LOCUS3D_ASSETS_DIR}" "$<TARGET_FILE_DIR:Locus3D>/assets")

if(WIN32)
    find_program(LOCUS3D_WINDEPLOYQT NAMES windeployqt6 windeployqt
        HINTS "${Qt6_DIR}/../../../bin")
    if(LOCUS3D_WINDEPLOYQT)
        add_custom_target(Locus3DDeploy
            COMMAND "${LOCUS3D_WINDEPLOYQT}" --no-translations
                --dir "$<TARGET_FILE_DIR:Locus3D>" "$<TARGET_FILE:Locus3D>"
            DEPENDS Locus3D
            VERBATIM)
    endif()
endif()

if(LOCUS3D_BUILD_TESTS)
    add_executable(Locus3DQtBridgeTests
        tests/EditorBridgeTests.cpp
        EditorBridge.h EditorBridge.cpp
        ${GLAD_DIR}/src/glad.c
    )
    set_target_properties(Locus3DQtBridgeTests PROPERTIES AUTOMOC ON)
    target_include_directories(Locus3DQtBridgeTests PRIVATE ${LOCUS3D_SOURCE_DIR} ${GLAD_DIR}/include)
    target_link_libraries(Locus3DQtBridgeTests PRIVATE
        Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGLWidgets
        Locus3D::Application Locus3D::Graphics Locus3D::Editor Locus3D::Geometry
    )
endif()
