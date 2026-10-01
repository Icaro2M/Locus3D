#pragma once

#include <QOpenGLWidget>
#include <QPointF>

#include "editor/tools/core/ToolEvent.h"

namespace locus::overlay {
class EditorBridge;

class LocusViewport final : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit LocusViewport(EditorBridge& bridge, QWidget* parent = nullptr);
    ~LocusViewport() override;
    bool ready() const noexcept { return ready_; }
    bool healthy() const noexcept { return ready_ && frameRendered_ && !renderFailed_; }
    void request_frame();

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private:
    enum class Navigation { None, Orbit, Pan };
    void shutdown_gl();
    void handle_pointer(editor::ToolEventType type, const QPointF& pos, const QPointF& delta,
                        Qt::KeyboardModifiers modifiers);
    void dispatch(editor::ToolEvent event);
    EditorBridge& bridge_;
    Navigation navigation_ = Navigation::None;
    QPointF lastPosition_{};
    bool ready_ = false;
    bool renderFailed_ = false;
    bool frameRendered_ = false;
    bool pointerDown_ = false;
};
}
