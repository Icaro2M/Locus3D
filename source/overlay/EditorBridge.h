#pragma once

#include "application/document/DocumentManager.h"
#include "application/viewport/EditorViewport.h"
#include "editor/gizmo/GizmoMode.h"
#include "editor/scene/SceneNodeId.h"
#include "editor/selection/SelectionGranularity.h"
#include "kernel/geometry/primitives/PrimitiveParameters.h"

#include <QObject>
#include <QString>
#include <QVector>
#include <array>
#include <memory>
#include <optional>

namespace locus::editor { class ICommand; }

namespace locus::overlay {

struct NodeView {
    editor::SceneNodeId id{};
    editor::SceneNodeId parent{};
    QString name;
    bool mesh = false;
    bool visible = true;
    bool locked = false;
    bool selectable = true;
};

struct TransformView {
    editor::SceneNodeId id{};
    std::array<double, 9> values{}; // position, Euler degrees, scale
};

struct MeshView {
    QString name;
    std::size_t vertices = 0;
    std::size_t edges = 0;
    std::size_t faces = 0;
};

struct AnalysisView {
    QString title;
    QString detail;
    int severity = 0;
};

class EditorBridge final : public QObject {
    Q_OBJECT
public:
    explicit EditorBridge(QObject* parent = nullptr);
    ~EditorBridge() override = default;

    application::DocumentSession& document();
    application::EditorViewport& viewport();
    QVector<NodeView> nodes() const;
    editor::SceneNodeId selected_node() const;
    std::optional<TransformView> selected_transform() const;
    std::optional<MeshView> selected_mesh() const;
    QVector<AnalysisView> analysis() const;
    QString document_name() const;
    bool dirty() const;
    bool can_undo() const;
    bool can_redo() const;
    bool can_delete();
    editor::SelectionGranularity granularity() const;
    QString active_tool() const;
    bool analysis_enabled() const;
    double minimum_wall() const;
    double minimum_feature() const;
    double maximum_overhang() const;

    void new_document();
    void open_document(const QString& path);
    bool save_document(const QString& path = {});
    void import_mesh(const QString& path);
    void add_primitive(kernel::geometry::PrimitiveType type);
    void export_selected_mesh(const QString& path);
    void select_node(editor::SceneNodeId id);
    void set_transform(int index, double value);
    void rename_node(editor::SceneNodeId id, const QString& name);
    void set_visibility(editor::SceneNodeId id, bool visible);
    void delete_selection();
    void undo();
    void redo();
    void set_granularity(editor::SelectionGranularity granularity);
    void activate_select();
    void activate_transform(editor::GizmoMode mode);
    void activate_mesh_tool(const QString& id);
    void set_analysis_enabled(bool enabled);
    void set_minimum_wall(double value);
    void set_minimum_feature(double value);
    void set_maximum_overhang(double value);
    void refresh();

signals:
    void changed();
    void errorOccurred(const QString& message);
    void statusChanged(const QString& message);

private:
    bool execute(std::unique_ptr<editor::ICommand> command, bool persistent = true);
    application::DocumentManager documents_{};
    application::EditorViewport viewport_{};
};

} // namespace locus::overlay
