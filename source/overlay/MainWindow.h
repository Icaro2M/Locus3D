#pragma once
#include <QMainWindow>
#include "overlay/EditorBridge.h"

class QAction;
class QCloseEvent;
namespace locus::overlay {
class LocusViewport;
class OutlinerWidget;
class TransformWidget;
class ManufacturingWidget;

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
    bool viewport_healthy() const noexcept;
protected:
    void closeEvent(QCloseEvent* event) override;
private:
    bool confirm_discard();
    bool save_as();
    void open_project();
    void import_model();
    void export_model();
    void refresh_ui();
    EditorBridge bridge_{};
    LocusViewport* viewport_ = nullptr;
    OutlinerWidget* outliner_ = nullptr;
    TransformWidget* transform_ = nullptr;
    ManufacturingWidget* manufacturing_ = nullptr;
    QAction* undoAction_ = nullptr;
    QAction* redoAction_ = nullptr;
    QAction* deleteAction_ = nullptr;
    QAction* analysisAction_ = nullptr;
    QAction* objectAction_ = nullptr;
    QAction* vertexAction_ = nullptr;
    QAction* edgeAction_ = nullptr;
    QAction* faceAction_ = nullptr;
};
}
