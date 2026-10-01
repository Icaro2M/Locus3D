#include <glad/glad.h>
#include "overlay/LocusViewport.h"
#include "overlay/EditorBridge.h"

#include "editor/tools/core/ToolContext.h"
#include "editor/tools/selection/SelectTool.h"
#include "editor/tools/transform/TransformTool.h"
#include "graphics/camera/CameraRayBuilder.h"
#include "graphics/gpu/Framebuffer.h"

#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QTimer>
#include <QWheelEvent>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#endif

namespace locus::overlay {
namespace {
void* load_gl(const char* name)
{
    if (auto* context = QOpenGLContext::currentContext()) {
        if (auto address = context->getProcAddress(name)) return reinterpret_cast<void*>(address);
    }
#ifdef _WIN32
    static HMODULE module = LoadLibraryA("opengl32.dll");
    if (module) return reinterpret_cast<void*>(GetProcAddress(module, name));
#endif
    return nullptr;
}
editor::ToolModifiers modifiers_of(Qt::KeyboardModifiers mods)
{
    auto value = editor::ToolModifiers::None;
    if (mods.testFlag(Qt::ShiftModifier)) value |= editor::ToolModifiers::Additive;
    if (mods.testFlag(Qt::ControlModifier)) value |= editor::ToolModifiers::Toggle;
    if (mods.testFlag(Qt::AltModifier)) value |= editor::ToolModifiers::Alternate;
    return value;
}
}

LocusViewport::LocusViewport(EditorBridge& bridge, QWidget* parent)
    : QOpenGLWidget(parent), bridge_(bridge)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    auto* timer = new QTimer(this);
    timer->setInterval(16);
    connect(timer, &QTimer::timeout, this, QOverload<>::of(&LocusViewport::update));
    timer->start();
    connect(&bridge_, &EditorBridge::changed, this, QOverload<>::of(&LocusViewport::update));
}

LocusViewport::~LocusViewport() { shutdown_gl(); }
void LocusViewport::request_frame() { update(); }

void LocusViewport::initializeGL()
{
    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(&load_gl)) == 0 || !GLAD_GL_VERSION_4_5) {
        emit bridge_.errorOccurred(tr("OpenGL 4.5 não está disponível neste dispositivo"));
        return;
    }
    graphics::Framebuffer::set_default_framebuffer(defaultFramebufferObject());
    auto result = bridge_.viewport().initialize(std::max(1, static_cast<int>(width() * devicePixelRatioF())),
                                                 std::max(1, static_cast<int>(height() * devicePixelRatioF())));
    if (!result) { emit bridge_.errorOccurred(QString::fromStdString(result.error().message)); return; }
    ready_ = true;
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, [this] { shutdown_gl(); }, Qt::DirectConnection);
}

void LocusViewport::shutdown_gl()
{
    if (!ready_) return;
    makeCurrent();
    bridge_.viewport().shutdown();
    graphics::Framebuffer::set_default_framebuffer(0);
    doneCurrent();
    ready_ = false;
}

void LocusViewport::resizeGL(int w, int h)
{
    if (ready_) bridge_.viewport().resize(std::max(1, static_cast<int>(w * devicePixelRatioF())),
                                           std::max(1, static_cast<int>(h * devicePixelRatioF())));
}

void LocusViewport::paintGL()
{
    if (!ready_ || renderFailed_) return;
    graphics::Framebuffer::set_default_framebuffer(defaultFramebufferObject());
    graphics::Framebuffer::bind_default();
    const auto result = bridge_.viewport().render(bridge_.document());
    if (!result) {
        renderFailed_ = true;
        emit bridge_.errorOccurred(QString::fromStdString(result.error().message));
    } else frameRendered_ = true;
}

void LocusViewport::dispatch(editor::ToolEvent event)
{
    auto& doc = bridge_.document();
    editor::ToolContext context(doc.editor(), doc.command_dispatcher(), doc.history(), doc.editor_sync().picking_sync());
    const auto before = doc.history().undo_size();
    const bool transformPress = event.type == editor::ToolEventType::PointerPress &&
        dynamic_cast<editor::TransformTool*>(doc.tool_manager().active_tool()) != nullptr;
    const auto result = doc.tool_manager().handle_event(context, event);
    if (result.failed()) emit bridge_.errorOccurred(QString::fromStdString(result.message));
    if (transformPress && !result.was_consumed()) {
        bridge_.activate_select();
        editor::ToolContext selectContext(doc.editor(), doc.command_dispatcher(), doc.history(), doc.editor_sync().picking_sync());
        const auto selection = doc.tool_manager().handle_event(selectContext, event);
        if (selection.failed()) emit bridge_.errorOccurred(QString::fromStdString(selection.message));
    }
    if (doc.history().undo_size() != before) doc.mark_history_changed();
    if (event.type != editor::ToolEventType::PointerMove || result.code == editor::ToolResultCode::Confirmed)
        bridge_.refresh();
}

void LocusViewport::handle_pointer(editor::ToolEventType type, const QPointF& pos, const QPointF& delta,
                                   Qt::KeyboardModifiers mods)
{
    if (!ready_ || renderFailed_) return;
    makeCurrent();
    graphics::Framebuffer::set_default_framebuffer(defaultFramebufferObject());
    auto& view = bridge_.viewport();
    auto& doc = bridge_.document();
    const auto pick = view.update_hover(doc, application::InputVector2{pos.x(), pos.y()},
                                       std::max(1, width()), std::max(1, height()), true, false);
    if (!pick) {
        emit bridge_.errorOccurred(QString::fromStdString(pick.error().message));
        doneCurrent();
        return;
    }
    const auto& hit = pick.value();
    if (hit.status != application::ViewportPickingStatus::Hit &&
        hit.status != application::ViewportPickingStatus::Background &&
        hit.status != application::ViewportPickingStatus::OutsideViewport) { doneCurrent(); return; }

    editor::ToolEvent event{};
    event.type = type;
    event.button = type == editor::ToolEventType::PointerMove ? editor::ToolPointerButton::None
                                                                : editor::ToolPointerButton::Primary;
    event.modifiers = modifiers_of(mods);
    event.pointer.viewportPosition = {static_cast<float>(hit.framebufferX), static_cast<float>(hit.framebufferY)};
    event.pointer.viewportDelta = {static_cast<float>(delta.x()), static_cast<float>(delta.y())};
    event.pointer.viewportSize = {static_cast<float>(view.viewport().state().rect.width),
                                  static_cast<float>(view.viewport().state().rect.height)};
    if (hit.has_hit()) event.pointer.pickingId = hit.pickingId;
    const auto& camera = view.viewport().camera();
    event.pointer.viewDirection = camera.forward();
    event.pointer.viewRight = camera.right();
    event.pointer.viewUp = camera.up();
    event.pointer.cameraPosition = camera.position();
    event.pointer.orthographicProjection = camera.projection().type() == graphics::ProjectionType::Orthographic;
    event.pointer.viewProjection = camera.view_projection_matrix();
    event.pointer.visualScale = view.visual_scale_at(view.orbit_rig().target());
    const auto rect = view.viewport().state().rect;
    const auto ray = graphics::CameraRayBuilder::from_viewport_pixel(camera, rect,
        static_cast<float>(rect.x + hit.framebufferX), static_cast<float>(rect.y + rect.height - 1 - hit.framebufferY));
    event.pointer.worldRay.origin = ray.origin;
    event.pointer.worldRay.direction = ray.direction;
    if (auto* transform = dynamic_cast<editor::TransformTool*>(doc.tool_manager().active_tool())) {
        editor::ToolContext context(doc.editor(), doc.command_dispatcher(), doc.history(), doc.editor_sync().picking_sync());
        transform->refresh_gizmo_state(context);
        if (transform->gizmo_state().visible)
            event.pointer.visualScale = view.visual_scale_at(transform->gizmo_state().pivot);
    }
    if (type == editor::ToolEventType::PointerRelease) {
        if (auto* select = dynamic_cast<editor::SelectTool*>(doc.tool_manager().active_tool());
            select && select->is_box_selecting()) {
            auto region = view.read_picking_region(doc, select->selection_rect());
            if (region) event.pointer.regionalPickingIds = region.value();
            else emit bridge_.errorOccurred(QString::fromStdString(region.error().message));
        }
    }
    dispatch(event);
    doneCurrent();
    update();
}

void LocusViewport::mousePressEvent(QMouseEvent* event)
{
    lastPosition_ = event->position();
    if (event->button() == Qt::RightButton || (event->button() == Qt::LeftButton && event->modifiers().testFlag(Qt::AltModifier)))
        navigation_ = Navigation::Orbit;
    else if (event->button() == Qt::MiddleButton) navigation_ = Navigation::Pan;
    else if (event->button() == Qt::LeftButton) {
        pointerDown_ = true;
        handle_pointer(editor::ToolEventType::PointerPress, lastPosition_, {}, event->modifiers());
    }
    setFocus();
}

void LocusViewport::mouseMoveEvent(QMouseEvent* event)
{
    const auto delta = event->position() - lastPosition_;
    lastPosition_ = event->position();
    if (navigation_ == Navigation::Orbit) bridge_.viewport().orbit_camera(delta.x(), delta.y());
    else if (navigation_ == Navigation::Pan) bridge_.viewport().pan_camera(delta.x(), delta.y());
    else handle_pointer(editor::ToolEventType::PointerMove, lastPosition_, delta, event->modifiers());
    update();
}

void LocusViewport::mouseReleaseEvent(QMouseEvent* event)
{
    if (navigation_ != Navigation::None) navigation_ = Navigation::None;
    else if (event->button() == Qt::LeftButton && pointerDown_)
        handle_pointer(editor::ToolEventType::PointerRelease, event->position(), {}, event->modifiers());
    pointerDown_ = false;
}

void LocusViewport::wheelEvent(QWheelEvent* event)
{
    bridge_.viewport().zoom_camera(event->angleDelta().y() / 120.0);
    update();
}

void LocusViewport::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        dispatch(editor::ToolEvent{editor::ToolEventType::Cancel});
        update();
    } else QOpenGLWidget::keyPressEvent(event);
}

void LocusViewport::focusOutEvent(QFocusEvent* event)
{
    navigation_ = Navigation::None;
    pointerDown_ = false;
    dispatch(editor::ToolEvent{editor::ToolEventType::FocusLost});
    QOpenGLWidget::focusOutEvent(event);
}

} // namespace locus::overlay
