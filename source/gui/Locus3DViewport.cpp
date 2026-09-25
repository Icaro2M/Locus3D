/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef _WIN32
#include <windows.h>
#endif

#include <glad/glad.h>

#include "Locus3DViewport.h"
#include "application/viewport/EditorViewport.h"
#include "application/document/DocumentSession.h"
#include "application/document/DocumentId.h"
#include "editor/Editor.h"
#include "editor/EditorTypes.h"
#include "editor/scene/MeshNode.h"
#include "editor/scene/NodeTransform.h"
#include "editor/selection/SelectionGranularity.h"
#include "kernel/geometry/topology/TopologyBuilder.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QOpenGLContext>
#include <QMouseEvent>
#include <QWheelEvent>
#include <iostream>
#include <glm/vec3.hpp>

namespace locus::gui {

Locus3DViewport::Locus3DViewport(QWidget* parent)
    : QOpenGLWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    try {
        m_document = std::make_unique<locus::application::DocumentSession>(
            locus::application::DocumentId{ 1u }
        );
    } catch (const std::exception& e) {
        std::cerr << "[Viewport ERRO] Falha ao criar DocumentSession: " << e.what() << std::endl;
    }
}

Locus3DViewport::~Locus3DViewport() {
    makeCurrent();
    if (m_editorViewport) {
        m_editorViewport->shutdown();
    }
    doneCurrent();
}

void Locus3DViewport::initializeGL() {
    std::cout << "  -> [Viewport] 1/4 Inicializando carregador seguro do GLAD..." << std::endl;

    // Carregador com fallback para a opengl32.dll do Windows
    GLADloadproc glLoader = [](const char* name) -> void* {
        void* proc = nullptr;
        if (auto* ctx = QOpenGLContext::currentContext()) {
            proc = reinterpret_cast<void*>(ctx->getProcAddress(QByteArray(name)));
        }
#ifdef _WIN32
        if (!proc) {
            static HMODULE opengl32Module = LoadLibraryA("opengl32.dll");
            if (opengl32Module) {
                proc = reinterpret_cast<void*>(GetProcAddress(opengl32Module, name));
            }
        }
#endif
        return proc;
    };

    if (!gladLoadGLLoader(glLoader)) {
        std::cerr << "  -> [Viewport ERRO] Falha critica: GLAD nao conseguiu carregar ponteiros OpenGL." << std::endl;
        return;
    }

    std::cout << "  -> [Viewport] 2/4 GLAD inicializado com sucesso. OpenGL Versao: "
              << glGetString(GL_VERSION) << std::endl;

    // Resolução de caminho absoluto/relativo para os shaders
    std::string shaderPath = "assets/shaders";
    QString appShaderDir = QCoreApplication::applicationDirPath() + "/assets/shaders";
    if (QDir(appShaderDir).exists()) {
        shaderPath = appShaderDir.toStdString();
    }

    std::cout << "  -> [Viewport] 3/4 Carregando EditorViewport em: " << shaderPath << std::endl;
    m_editorViewport = std::make_unique<locus::application::EditorViewport>();
    auto initResult = m_editorViewport->initialize(width(), height(), shaderPath);

    if (!initResult) {
        std::cerr << "  -> [Viewport AVISO] EditorViewport::initialize retornou erro: "
                  << initResult.error().message << std::endl;
    } else {
        std::cout << "  -> [Viewport] EditorViewport inicializado com exito." << std::endl;
    }

    std::cout << "  -> [Viewport] 4/4 Populando cena inicial..." << std::endl;
    seedDemoScene();
    std::cout << "  -> [Viewport] initializeGL concluido com sucesso." << std::endl;
}

void Locus3DViewport::resizeGL(int width, int height) {
    if (height <= 0) height = 1;
    glViewport(0, 0, width, height);

    if (m_editorViewport && m_editorViewport->initialized()) {
        m_editorViewport->resize(width, height);
    }
}

void Locus3DViewport::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_editorViewport && m_editorViewport->initialized() && m_document) {
        auto renderResult = m_editorViewport->render(*m_document);
        if (!renderResult) {
            std::cerr << "  -> [Viewport AVISO] Erro no render: "
                      << renderResult.error().message << std::endl;
        }
    }
}

void Locus3DViewport::seedDemoScene() {
    if (!m_document) return;

    try {
        locus::editor::Editor& editor = m_document->editor();

        const locus::editor::SceneNodeId cubeAId = editor.scene().create_mesh("Cube A");
        const locus::editor::SceneNodeId cubeBId = editor.scene().create_mesh("Cube B");

        locus::editor::MeshNode* cubeA = editor.scene().find_mesh(cubeAId);
        locus::editor::MeshNode* cubeB = editor.scene().find_mesh(cubeBId);

        if (!cubeA || !cubeB) {
            std::cerr << "  -> [Viewport ERRO] Falha ao recuperar nos de malha criados." << std::endl;
            return;
        }

        (void)locus::kernel::geometry::TopologyBuilder::build_box_into(cubeA->mesh());
        (void)locus::kernel::geometry::TopologyBuilder::build_box_into(cubeB->mesh());

        cubeA->transform().set_position(glm::vec3{ -1.4f, 0.0f, 0.0f });
        cubeB->transform().set_position(glm::vec3{ 1.4f, 0.0f, 0.0f });

        editor.selection_controller().select_object(cubeBId);

        editor.mark_dirty(
            locus::editor::EditorDirtyFlags::Scene |
            locus::editor::EditorDirtyFlags::Mesh |
            locus::editor::EditorDirtyFlags::Render |
            locus::editor::EditorDirtyFlags::Picking
        );
        std::cout << "  -> [Viewport] Objetos de teste inseridos na cena." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "  -> [Viewport ERRO] Excecao em seedDemoScene: " << e.what() << std::endl;
    }
}

void Locus3DViewport::adicionarCubo() {
    if (!m_document) return;

    locus::editor::Editor& editor = m_document->editor();
    static int cubeCount = 2;

    const std::string name = "Cube " + std::to_string(++cubeCount);
    const locus::editor::SceneNodeId newId = editor.scene().create_mesh(name);

    if (locus::editor::MeshNode* node = editor.scene().find_mesh(newId)) {
        (void)locus::kernel::geometry::TopologyBuilder::build_box_into(node->mesh());
        node->transform().set_position(glm::vec3{ 0.0f, 0.0f, 0.0f });

        editor.selection_controller().select_object(newId);
        editor.mark_dirty(
            locus::editor::EditorDirtyFlags::Scene |
            locus::editor::EditorDirtyFlags::Mesh |
            locus::editor::EditorDirtyFlags::Render
        );
    }

    update();
}

void Locus3DViewport::mousePressEvent(QMouseEvent* event) {
    m_lastMousePos = event->pos();

    if (event->button() == Qt::RightButton || 
       (event->button() == Qt::LeftButton && (event->modifiers() & Qt::AltModifier))) {
        m_isOrbiting = true;
    } else if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
    }

    setFocus();
}

void Locus3DViewport::mouseMoveEvent(QMouseEvent* event) {
    const QPoint delta = event->pos() - m_lastMousePos;
    m_lastMousePos = event->pos();

    if (!m_editorViewport) return;

    if (m_isOrbiting) {
        m_editorViewport->orbit_camera(static_cast<double>(delta.x()), static_cast<double>(delta.y()));
        update();
    } else if (m_isPanning) {
        m_editorViewport->pan_camera(static_cast<double>(delta.x()), static_cast<double>(delta.y()));
        update();
    }
}

void Locus3DViewport::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton || event->button() == Qt::LeftButton) {
        m_isOrbiting = false;
    }
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = false;
    }
}

void Locus3DViewport::wheelEvent(QWheelEvent* event) {
    if (m_editorViewport) {
        const double delta = event->angleDelta().y() / 120.0;
        m_editorViewport->zoom_camera(delta);
        update();
    }
}

} // namespace locus::gui