#include "overlay/OutlinerWidget.h"
#include "overlay/EditorBridge.h"
#include <QBrush>
#include <QColor>
#include <QHeaderView>
#include <QLabel>
#include <QSignalBlocker>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <unordered_map>

namespace locus::overlay {
OutlinerWidget::OutlinerWidget(EditorBridge& bridge, QWidget* parent) : QWidget(parent), bridge_(bridge)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    auto* heading = new QLabel(tr("CENA"), this);
    heading->setObjectName("panelHeading");
    layout->addWidget(heading);
    tree_ = new QTreeWidget(this);
    tree_->setHeaderLabels({tr("Objeto"), tr("Mostrar")});
    tree_->header()->setStretchLastSection(false);
    tree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    tree_->setColumnWidth(1, 64);
    tree_->setAlternatingRowColors(true);
    tree_->setRootIsDecorated(true);
    tree_->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(tree_);
    connect(tree_, &QTreeWidget::itemSelectionChanged, this, [this] {
        if (auto* item = tree_->currentItem()) bridge_.select_node(id_of(item));
    });
    connect(tree_, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem* item, int column) {
        if (column == 0) bridge_.rename_node(id_of(item), item->text(0));
        else if (column == 1) bridge_.set_visibility(id_of(item), item->checkState(1) == Qt::Checked);
    });
    refresh();
}

editor::SceneNodeId OutlinerWidget::id_of(QTreeWidgetItem* item) const
{ return item ? editor::SceneNodeId{item->data(0, Qt::UserRole).toULongLong()} : editor::SceneNodeId{}; }

void OutlinerWidget::refresh()
{
    const auto nodes = bridge_.nodes();
    QString next;
    for (const auto& node : nodes)
        next += QString::number(node.id.value) + ':' + QString::number(node.parent.value) + ';';
    QSignalBlocker block(tree_);
    if (next != structureSignature_) {
        structureSignature_ = next;
        tree_->clear();
        std::unordered_map<editor::SceneNodeIdValue, QTreeWidgetItem*> items;
        for (const auto& node : nodes) {
            auto* item = new QTreeWidgetItem();
            item->setData(0, Qt::UserRole, QVariant::fromValue<qulonglong>(node.id.value));
            item->setText(0, node.name);
            item->setFlags(item->flags() | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
            item->setCheckState(1, node.visible ? Qt::Checked : Qt::Unchecked);
            item->setToolTip(0, node.mesh ? tr("Malha") : tr("Nó vazio"));
            if (node.locked || !node.selectable) item->setForeground(0, QColor("#8390a0"));
            items.emplace(node.id.value, item);
        }
        for (const auto& node : nodes) {
            auto* item = items.at(node.id.value);
            auto found = items.find(node.parent.value);
            if (found == items.end()) tree_->addTopLevelItem(item);
            else found->second->addChild(item);
        }
        tree_->expandAll();
    }
    // itemChanged synchronously executes a command and re-enters refresh().
    // Keep the item/index alive until Qt's delegate finishes that edit event;
    // metadata changes must not reset the model or delete the edited item.
    std::unordered_map<editor::SceneNodeIdValue, const NodeView*> views;
    for (const auto& node : nodes) views.emplace(node.id.value, &node);
    const auto selected = bridge_.selected_node();
    QTreeWidgetItem* current = nullptr;
    for (auto iterator = QTreeWidgetItemIterator(tree_); *iterator; ++iterator) {
        auto* item = *iterator;
        const auto id = id_of(item);
        const auto found = views.find(id.value);
        if (found != views.end()) {
            const auto& node = *found->second;
            if (item->text(0) != node.name) item->setText(0, node.name);
            const auto checkState = node.visible ? Qt::Checked : Qt::Unchecked;
            if (item->checkState(1) != checkState) item->setCheckState(1, checkState);
            const QBrush foreground = node.locked || !node.selectable ? QBrush(QColor("#8390a0")) : QBrush{};
            if (item->foreground(0) != foreground) item->setForeground(0, foreground);
        }
        if (id == selected) current = item;
    }
    if (tree_->currentItem() != current) tree_->setCurrentItem(current);
}
}
