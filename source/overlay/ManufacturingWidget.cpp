#include "overlay/ManufacturingWidget.h"
#include "overlay/EditorBridge.h"
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QListWidget>
#include <QSignalBlocker>
#include <QVBoxLayout>

namespace locus::overlay {
ManufacturingWidget::ManufacturingWidget(EditorBridge& bridge, QWidget* parent) : QWidget(parent), bridge_(bridge)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    auto* heading = new QLabel(tr("PREPARAÇÃO PARA IMPRESSÃO"), this);
    heading->setObjectName("panelHeading");
    layout->addWidget(heading);
    enabled_ = new QCheckBox(tr("Mostrar análise de fabricabilidade"), this);
    layout->addWidget(enabled_);
    connect(enabled_, &QCheckBox::toggled, this, [this](bool value) { bridge_.set_analysis_enabled(value); });
    layout->addWidget(new QLabel(tr("Espessura mínima de parede (unidades do modelo)"), this));
    minimumWall_ = new QDoubleSpinBox(this);
    minimumWall_->setRange(0, 10000);
    minimumWall_->setDecimals(3);
    minimumWall_->setSingleStep(0.1);
    minimumWall_->setSpecialValueText(tr("Não definida"));
    minimumWall_->setKeyboardTracking(false);
    layout->addWidget(minimumWall_);
    connect(minimumWall_, &QDoubleSpinBox::valueChanged, this,
        [this](double value) { bridge_.set_minimum_wall(value); });
    layout->addWidget(new QLabel(tr("Tamanho mínimo de detalhe (unidades do modelo)"), this));
    minimumFeature_ = new QDoubleSpinBox(this);
    minimumFeature_->setRange(0, 10000);
    minimumFeature_->setDecimals(3);
    minimumFeature_->setSingleStep(0.1);
    minimumFeature_->setSpecialValueText(tr("Não definido"));
    minimumFeature_->setKeyboardTracking(false);
    layout->addWidget(minimumFeature_);
    connect(minimumFeature_, &QDoubleSpinBox::valueChanged, this,
        [this](double value) { bridge_.set_minimum_feature(value); });
    layout->addWidget(new QLabel(tr("Ângulo máximo sem suporte (°)"), this));
    maximumOverhang_ = new QDoubleSpinBox(this);
    maximumOverhang_->setRange(0, 90);
    maximumOverhang_->setDecimals(1);
    maximumOverhang_->setSingleStep(1);
    maximumOverhang_->setSpecialValueText(tr("Não definido"));
    maximumOverhang_->setKeyboardTracking(false);
    layout->addWidget(maximumOverhang_);
    connect(maximumOverhang_, &QDoubleSpinBox::valueChanged, this,
        [this](double value) { bridge_.set_maximum_overhang(value); });
    layout->addWidget(new QLabel(tr("Diagnósticos"), this));
    issues_ = new QListWidget(this);
    issues_->setWordWrap(true);
    layout->addWidget(issues_, 1);
    auto* note = new QLabel(tr("Análise geométrica; este aplicativo não gera G-code."), this);
    note->setWordWrap(true);
    note->setObjectName("mutedLabel");
    layout->addWidget(note);
    refresh();
}

void ManufacturingWidget::refresh()
{
    QSignalBlocker block(enabled_);
    enabled_->setChecked(bridge_.analysis_enabled());
    if (!minimumWall_->hasFocus()) {
        QSignalBlocker wallBlock(minimumWall_);
        minimumWall_->setValue(bridge_.minimum_wall());
    }
    if (!minimumFeature_->hasFocus()) {
        QSignalBlocker featureBlock(minimumFeature_);
        minimumFeature_->setValue(bridge_.minimum_feature());
    }
    if (!maximumOverhang_->hasFocus()) {
        QSignalBlocker angleBlock(maximumOverhang_);
        maximumOverhang_->setValue(bridge_.maximum_overhang());
    }
    const auto reports = bridge_.analysis();
    QString next = enabled_->isChecked() ? "1" : "0";
    for (const auto& item : reports) next += item.title + item.detail + QString::number(item.severity);
    if (next == signature_) return;
    signature_ = next;
    issues_->clear();
    if (!enabled_->isChecked()) {
        issues_->addItem(tr("Ative a análise para ver os diagnósticos."));
    } else if (reports.isEmpty()) {
        issues_->addItem(tr("Nenhuma ocorrência no estado analisado."));
    } else {
        for (const auto& item : reports) {
            auto* entry = new QListWidgetItem(item.title + " — " + item.detail, issues_);
            if (item.severity >= 2) entry->setForeground(QColor("#f08080"));
            else if (item.severity == 1) entry->setForeground(QColor("#e8bc74"));
        }
    }
}
}
