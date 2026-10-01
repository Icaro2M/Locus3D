#pragma once
#include <QWidget>
class QCheckBox;
class QDoubleSpinBox;
class QListWidget;
namespace locus::overlay {
class EditorBridge;
class ManufacturingWidget final : public QWidget {
    Q_OBJECT
public:
    explicit ManufacturingWidget(EditorBridge& bridge, QWidget* parent = nullptr);
public slots:
    void refresh();
private:
    EditorBridge& bridge_;
    QCheckBox* enabled_ = nullptr;
    QDoubleSpinBox* minimumWall_ = nullptr;
    QDoubleSpinBox* minimumFeature_ = nullptr;
    QDoubleSpinBox* maximumOverhang_ = nullptr;
    QListWidget* issues_ = nullptr;
    QString signature_;
};
}
