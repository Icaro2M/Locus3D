#pragma once
#include <QWidget>
#include <QString>
#include "editor/scene/SceneNodeId.h"

class QTreeWidget;
class QTreeWidgetItem;
namespace locus::overlay {
class EditorBridge;
class OutlinerWidget final : public QWidget {
    Q_OBJECT
public:
    explicit OutlinerWidget(EditorBridge& bridge, QWidget* parent = nullptr);
public slots:
    void refresh();
private:
    editor::SceneNodeId id_of(QTreeWidgetItem* item) const;
    EditorBridge& bridge_;
    QTreeWidget* tree_ = nullptr;
    QString signature_;
};
}
