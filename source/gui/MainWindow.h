#pragma once

#include <QMainWindow>
#include <memory>

// Declaração antecipada para evitar dependência circular de compilação
namespace Ui {
    class MainWindow;
}

namespace locus::gui {

class Locus3DViewport;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    std::unique_ptr<Ui::MainWindow> m_ui;
    Locus3DViewport* m_viewport{nullptr};
};

} // namespace locus::gui