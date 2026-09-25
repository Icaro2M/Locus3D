/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <QOpenGLWidget>
#include <QPoint>
#include <memory>

namespace locus::application {
    class EditorViewport;
    class DocumentSession;
}

namespace locus::gui {

class Locus3DViewport : public QOpenGLWidget {
    Q_OBJECT

public:
    explicit Locus3DViewport(QWidget* parent = nullptr);
    ~Locus3DViewport() override;

    void adicionarCubo();

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void seedDemoScene();

    std::unique_ptr<locus::application::DocumentSession> m_document;
    std::unique_ptr<locus::application::EditorViewport>  m_editorViewport;

    QPoint m_lastMousePos;
    bool m_isOrbiting = false;
    bool m_isPanning = false;
};

} // namespace locus::gui