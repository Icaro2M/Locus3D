#pragma once
#include <QWidget>
#include <array>
class QDoubleSpinBox;
class QLabel;
namespace locus::overlay {
class EditorBridge;
class TransformWidget final : public QWidget {
    Q_OBJECT
public:
    explicit TransformWidget(EditorBridge& bridge, QWidget* parent = nullptr);
public slots:
    void refresh();
private:
    EditorBridge& bridge_;
    QLabel* subject_ = nullptr;
    QLabel* geometry_ = nullptr;
    std::array<QDoubleSpinBox*, 9> fields_{};
};
}
