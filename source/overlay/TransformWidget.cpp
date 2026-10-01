#include "overlay/TransformWidget.h"
#include "overlay/EditorBridge.h"
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSignalBlocker>
#include <QVBoxLayout>

namespace locus::overlay {
TransformWidget::TransformWidget(EditorBridge& bridge, QWidget* parent) : QWidget(parent), bridge_(bridge)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    subject_ = new QLabel(tr("Nenhum objeto selecionado"), this);
    subject_->setObjectName("panelHeading");
    layout->addWidget(subject_);
    const QString names[3] = {tr("Posição"), tr("Rotação (°)"), tr("Escala")};
    const QString axes[3] = {"X", "Y", "Z"};
    for (int group = 0; group < 3; ++group) {
        auto* box = new QGroupBox(names[group], this);
        auto* form = new QFormLayout(box);
        for (int axis = 0; axis < 3; ++axis) {
            const int index = group * 3 + axis;
            auto* spin = new QDoubleSpinBox(box);
            spin->setDecimals(3);
            spin->setRange(group == 2 ? -10000.0 : -1000000.0, group == 1 ? 360000.0 : 1000000.0);
            spin->setSingleStep(group == 1 ? 1.0 : 0.1);
            spin->setKeyboardTracking(false);
            spin->setEnabled(false);
            spin->setValue(group == 2 ? 1.0 : 0.0);
            fields_[index] = spin;
            form->addRow(axes[axis], spin);
            connect(spin, &QDoubleSpinBox::valueChanged, this,
                [this, index](double value) { bridge_.set_transform(index, value); });
        }
        layout->addWidget(box);
    }
    geometry_ = new QLabel(this);
    geometry_->setWordWrap(true);
    layout->addWidget(geometry_);
    layout->addStretch();
    refresh();
}

void TransformWidget::refresh()
{
    const auto values = bridge_.selected_transform();
    const auto mesh = bridge_.selected_mesh();
    subject_->setText(values ? (mesh ? mesh->name : tr("Nó selecionado")) : tr("Nenhum objeto selecionado"));
    geometry_->setText(mesh ? tr("%1 vértices  ·  %2 arestas  ·  %3 faces")
        .arg(mesh->vertices).arg(mesh->edges).arg(mesh->faces) : QString{});
    for (int i = 0; i < 9; ++i) {
        fields_[i]->setEnabled(values.has_value());
        if (values && !fields_[i]->hasFocus()) {
            QSignalBlocker block(fields_[i]);
            fields_[i]->setValue(values->values[i]);
        }
    }
}
}
