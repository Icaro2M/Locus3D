#include "overlay/EditorBridge.h"

#include "application/operation/DocumentOperations.h"
#include "editor/actions/core/ActionContext.h"
#include "editor/actions/core/IEditorAction.h"
#include "editor/actions/edit/RegisterEditActions.h"
#include "editor/command/scene/RenameNodeCommand.h"
#include "editor/command/document/ImportMeshCommand.h"
#include "editor/command/scene/SetNodeVisibilityCommand.h"
#include "editor/command/selection/SetObjectSelectionCommand.h"
#include "editor/command/selection/SetSelectionGranularityCommand.h"
#include "editor/command/transform/SetNodeTransformCommand.h"
#include "editor/tools/core/ToolContext.h"
#include "editor/tools/selection/SelectTool.h"
#include "editor/tools/transform/TransformTool.h"
#include "kernel/io/ObjImporter.h"
#include "kernel/io/ObjExporter.h"
#include "kernel/io/StlImporter.h"
#include "kernel/io/StlExporter.h"
#include "kernel/geometry/primitives/PrimitiveRegistry.h"
#include "kernel/geometry/mesh/editing/geometry/GeometryTransform.h"
#include "editor/scene/SceneTransforms.h"

#include <QFileInfo>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <filesystem>

namespace locus::overlay {

namespace {
QString label(const std::string& value) { return QString::fromStdString(value); }
std::filesystem::path path_of(const QString& value) { return std::filesystem::path(value.toStdWString()); }
}

EditorBridge::EditorBridge(QObject* parent) : QObject(parent) { (void)documents_.create_document(); }
application::DocumentSession& EditorBridge::document() { return *documents_.active_document(); }
application::EditorViewport& EditorBridge::viewport() { return viewport_; }

QVector<NodeView> EditorBridge::nodes() const
{
    QVector<NodeView> result;
    const auto& scene = documents_.active_document()->editor().scene();
    for (auto id : scene.tree().node_ids()) {
        const auto* node = scene.find_node(id);
        if (!node) continue;
        result.push_back({id, node->parent(), label(node->metadata().name),
            node->type() == editor::NodeType::Mesh, node->metadata().visible,
            node->metadata().locked, node->metadata().selectable});
    }
    std::sort(result.begin(), result.end(), [](const NodeView& a, const NodeView& b) { return a.id.value < b.id.value; });
    return result;
}

editor::SceneNodeId EditorBridge::selected_node() const
{ return documents_.active_document()->editor().selection().objects().active(); }

std::optional<TransformView> EditorBridge::selected_transform() const
{
    auto id = selected_node();
    const auto* node = documents_.active_document()->editor().scene().find_node(id);
    if (!node) return {};
    const auto& t = node->transform();
    const glm::vec3 degrees = glm::degrees(glm::eulerAngles(t.rotation()));
    return TransformView{id, {t.position().x, t.position().y, t.position().z,
        degrees.x, degrees.y, degrees.z, t.scale().x, t.scale().y, t.scale().z}};
}

std::optional<MeshView> EditorBridge::selected_mesh() const
{
    const auto* node = documents_.active_document()->editor().scene().find_mesh(selected_node());
    if (!node) return {};
    const auto& mesh = node->mesh();
    return MeshView{label(node->metadata().name), mesh.vertex_count(), mesh.edge_count(), mesh.face_count()};
}

QVector<AnalysisView> EditorBridge::analysis() const
{
    QVector<AnalysisView> output;
    const auto& sync = documents_.active_document()->editor_sync().manufacturing_sync();
    if (!sync.enabled()) return output;
    for (const auto* result : sync.results()) {
        if (!result || !result->valid) continue;
        const auto* node = documents_.active_document()->editor().scene().find_node(result->nodeId);
        const QString name = node ? label(node->metadata().name) : tr("Modelo");
        for (const auto& issue : result->report.issues()) {
            output.push_back({name, label(issue.message), static_cast<int>(issue.severity)});
        }
    }
    return output;
}

QString EditorBridge::document_name() const { return label(documents_.active_document()->display_name()); }
bool EditorBridge::dirty() const { return documents_.active_document()->is_dirty(); }
bool EditorBridge::can_undo() const { return documents_.active_document()->history().can_undo(); }
bool EditorBridge::can_redo() const { return documents_.active_document()->history().can_redo(); }
bool EditorBridge::can_delete()
{
    editor::ActionContext context(document().editor(), document().command_dispatcher(), document().history());
    const auto* action = document().action_registry().find(
        editor::ActionId{std::string(editor::edit_actions::DeleteId)});
    return action && action->can_execute(context);
}
editor::SelectionGranularity EditorBridge::granularity() const { return documents_.active_document()->editor().selection().granularity(); }
QString EditorBridge::active_tool() const { return label(documents_.active_document()->tool_manager().active_tool_id().value); }
bool EditorBridge::analysis_enabled() const { return documents_.active_document()->editor_sync().manufacturing_sync().enabled(); }
double EditorBridge::minimum_wall() const
{
    const auto& limits = documents_.active_document()->editor_sync().manufacturing_sync().analysis_settings().profile.limits();
    return limits.minimumWallThickness.value_or(0.0);
}
double EditorBridge::minimum_feature() const
{
    const auto& limits = documents_.active_document()->editor_sync().manufacturing_sync().analysis_settings().profile.limits();
    return limits.minimumFeatureSize.value_or(0.0);
}
double EditorBridge::maximum_overhang() const
{
    const auto& limits = documents_.active_document()->editor_sync().manufacturing_sync().analysis_settings().profile.limits();
    return limits.maximumUnsupportedOverhangAngleDegrees.value_or(0.0);
}

void EditorBridge::new_document()
{
    const auto old = document().id();
    (void)documents_.close_document(old);
    (void)documents_.create_document();
    emit changed();
    emit statusChanged(tr("Novo projeto"));
}

void EditorBridge::open_document(const QString& path)
{
    if (path.isEmpty()) return;
    const auto result = application::open_document(document(), path_of(path));
    if (!result) emit errorOccurred(label(result.error().message));
    else { emit changed(); emit statusChanged(tr("Projeto aberto")); }
}

bool EditorBridge::save_document(const QString& path)
{
    const auto result = path.isEmpty() ? application::save_document(document())
        : application::save_document(document(), path_of(path));
    if (!result) { emit errorOccurred(label(result.error().message)); return false; }
    emit changed();
    emit statusChanged(tr("Projeto salvo"));
    return true;
}

void EditorBridge::import_mesh(const QString& path)
{
    if (path.isEmpty()) return;
    const std::filesystem::path native = path_of(path);
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix != "stl" && suffix != "obj") { emit errorOccurred(tr("Formato não suportado")); return; }
    const auto mesh = suffix == "stl" ? kernel::io::StlImporter{}.import_mesh(native)
                                      : kernel::io::ObjImporter{}.import_mesh(native);
    if (mesh.is_error()) { emit errorOccurred(label(mesh.error().message)); return; }
    execute(std::make_unique<editor::ImportMeshCommand>(mesh.value(), QFileInfo(path).completeBaseName().toStdString()));
}

void EditorBridge::add_primitive(kernel::geometry::PrimitiveType type)
{
    const auto parameters = kernel::geometry::PrimitiveRegistry::default_parameters(type);
    auto mesh = kernel::geometry::PrimitiveRegistry::build(parameters);
    if (mesh.vertex_count() == 0 || mesh.face_count() == 0) {
        emit errorOccurred(tr("Nao foi possivel criar a primitiva"));
        return;
    }

    QString name;
    switch (type) {
    case kernel::geometry::PrimitiveType::Box: name = tr("Cubo"); break;
    case kernel::geometry::PrimitiveType::Cylinder: name = tr("Cilindro"); break;
    case kernel::geometry::PrimitiveType::Sphere: name = tr("Esfera"); break;
    case kernel::geometry::PrimitiveType::Cone: name = tr("Cone"); break;
    case kernel::geometry::PrimitiveType::Torus: name = tr("Toro"); break;
    }

    if (execute(std::make_unique<editor::ImportMeshCommand>(mesh, name.toStdString())))
        emit statusChanged(tr("%1 adicionado").arg(name));
}

void EditorBridge::export_selected_mesh(const QString& path)
{
    const auto& scene = document().editor().scene();
    const auto* node = scene.find_mesh(selected_node());
    if (!node) { emit errorOccurred(tr("Selecione uma malha para exportar")); return; }
    kernel::geometry::LEM mesh = node->mesh();
    std::vector<kernel::geometry::VertexHandle> vertices;
    vertices.reserve(mesh.vertex_count());
    for (std::size_t i = 0; i < mesh.vertices().size(); ++i) {
        kernel::geometry::VertexHandle handle{static_cast<kernel::IdValue>(i)};
        if (mesh.is_valid(handle)) vertices.push_back(handle);
    }
    kernel::geometry::LEMDiff diff;
    kernel::geometry::GeometryTransform(mesh, diff).transform_vertices(vertices,
        editor::SceneTransforms::world_matrix(scene, selected_node()));
    const std::filesystem::path native = path_of(path);
    const auto result = QFileInfo(path).suffix().toLower() == "obj"
        ? kernel::io::ObjExporter{}.export_mesh(mesh, native)
        : kernel::io::StlExporter{}.export_mesh(mesh, native);
    if (result.is_error()) emit errorOccurred(label(result.error().message));
    else emit statusChanged(tr("Malha exportada"));
}

bool EditorBridge::execute(std::unique_ptr<editor::ICommand> command, bool persistent)
{
    const auto result = persistent ? document().history().execute(document().command_dispatcher(), std::move(command))
                                   : document().command_dispatcher().execute(std::move(command));
    if (!result.success) { emit errorOccurred(label(result.message)); return false; }
    if (persistent) document().mark_history_changed();
    emit changed();
    if (!result.message.empty()) emit statusChanged(label(result.message));
    return true;
}

void EditorBridge::select_node(editor::SceneNodeId id)
{
    execute(std::make_unique<editor::SetObjectSelectionCommand>(
        std::vector<editor::SceneNodeId>{id}, editor::SelectionOperation::Replace), false);
}

void EditorBridge::set_transform(int index, double value)
{
    const auto id = selected_node();
    const auto* node = document().editor().scene().find_node(id);
    if (!node || index < 0 || index >= 9) return;
    editor::NodeTransform t = node->transform();
    if (index < 3) {
        auto p = t.position(); p[index] = static_cast<float>(value); t.set_position(p);
    } else if (index < 6) {
        glm::vec3 angles = glm::degrees(glm::eulerAngles(t.rotation()));
        angles[index - 3] = static_cast<float>(value);
        t.set_rotation(glm::quat(glm::radians(angles)));
    } else {
        auto s = t.scale(); s[index - 6] = static_cast<float>(value); t.set_scale(s);
    }
    execute(std::make_unique<editor::SetNodeTransformCommand>(id, t));
}

void EditorBridge::rename_node(editor::SceneNodeId id, const QString& name)
{ execute(std::make_unique<editor::RenameNodeCommand>(id, name.trimmed().toStdString())); }
void EditorBridge::set_visibility(editor::SceneNodeId id, bool visible)
{ execute(std::make_unique<editor::SetNodeVisibilityCommand>(id, visible)); }

void EditorBridge::delete_selection()
{
    editor::ActionContext context(document().editor(), document().command_dispatcher(), document().history());
    const auto before = document().history().undo_size();
    const auto result = document().action_executor().execute(context,
        editor::ActionId{std::string(editor::edit_actions::DeleteId)});
    if (result.code != editor::ActionResultCode::Executed) { emit errorOccurred(label(result.message)); return; }
    if (document().history().undo_size() != before) document().mark_history_changed();
    emit changed();
}

void EditorBridge::undo()
{
    const auto result = document().history().undo(document().command_dispatcher());
    if (!result.success) emit errorOccurred(label(result.message));
    else { document().mark_history_changed(); emit changed(); }
}
void EditorBridge::redo()
{
    const auto result = document().history().redo(document().command_dispatcher());
    if (!result.success) emit errorOccurred(label(result.message));
    else { document().mark_history_changed(); emit changed(); }
}
void EditorBridge::set_granularity(editor::SelectionGranularity value)
{ execute(std::make_unique<editor::SetSelectionGranularityCommand>(value), false); }

void EditorBridge::activate_select() { activate_mesh_tool(QString::fromUtf8(editor::SelectTool::Id)); }
void EditorBridge::activate_transform(editor::GizmoMode mode)
{
    activate_mesh_tool(QString::fromUtf8(editor::TransformTool::Id));
    if (auto* tool = dynamic_cast<editor::TransformTool*>(document().tool_manager().active_tool())) {
        if (!tool->set_mode(mode)) emit errorOccurred(tr("Finalize o arrasto antes de trocar a ferramenta"));
        else {
            editor::ToolContext context(document().editor(), document().command_dispatcher(), document().history(),
                document().editor_sync().picking_sync());
            tool->refresh_gizmo_state(context);
            emit changed();
        }
    }
}
void EditorBridge::activate_mesh_tool(const QString& id)
{
    editor::ToolContext context(document().editor(), document().command_dispatcher(), document().history(),
        document().editor_sync().picking_sync());
    const auto result = document().tool_manager().activate_tool(context, editor::ToolId{id.toStdString()});
    if (result.failed()) emit errorOccurred(label(result.message));
    else emit changed();
}
void EditorBridge::set_analysis_enabled(bool enabled)
{
    viewport_.set_manufacturing_analysis_enabled(document(), enabled);
    emit changed();
}
void EditorBridge::set_minimum_wall(double value)
{
    auto& sync = document().editor_sync().manufacturing_sync();
    sync.analysis_settings().profile.limits().minimumWallThickness = value > 0 ? std::optional<double>(value) : std::nullopt;
    sync.bump_analysis_revision();
    emit changed();
}
void EditorBridge::set_minimum_feature(double value)
{
    auto& sync = document().editor_sync().manufacturing_sync();
    sync.analysis_settings().profile.limits().minimumFeatureSize = value > 0 ? std::optional<double>(value) : std::nullopt;
    sync.bump_analysis_revision();
    emit changed();
}
void EditorBridge::set_maximum_overhang(double value)
{
    auto& sync = document().editor_sync().manufacturing_sync();
    sync.analysis_settings().profile.limits().maximumUnsupportedOverhangAngleDegrees =
        value > 0 ? std::optional<double>(value) : std::nullopt;
    sync.bump_analysis_revision();
    emit changed();
}
void EditorBridge::refresh() { emit changed(); }

} // namespace locus::overlay
