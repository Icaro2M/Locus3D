#include "MainWindow.h"
#include "ui_MainWindow.h" 
#include "Locus3DViewport.h"

namespace locus::gui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_ui(std::make_unique<Ui::MainWindow>()) {
    
    m_ui->setupUi(this);
    m_viewport = m_ui->viewport3D;
    m_viewport->setFocus(); // Garante que o 3D receba o teclado/mouse

    // =========================================================================
    // 1. APLICAÇÃO DO TEMA ESCURO PROFISSIONAL (QSS)
    // =========================================================================
    this->setStyleSheet(R"(
        QMainWindow, QDockWidget, QWidget { background-color: #1e1e1e; color: #e0e0e0; }
        QTreeWidget { background-color: #181818; color: #ffffff; border: 1px solid #2d2d2d; }
        QDoubleSpinBox { background-color: #121212; color: #ffffff; border: 1px solid #333333; padding: 2px; }
        QToolBar { background-color: #1a1a1a; border-bottom: 1px solid #2d2d2d; spacing: 4px; }
        QToolButton { background-color: #2b2b2b; color: #ffffff; border-radius: 2px; padding: 3px 6px; }
        QToolButton:hover { background-color: #3d3d3d; }
        /* Colore os eixos XYZ no Inspetor */
        QLabel[text="X"] { color: #e04545; font-weight: bold; }
        QLabel[text="Y"] { color: #45b565; font-weight: bold; }
        QLabel[text="Z"] { color: #6565e0; font-weight: bold; }
    )");

    // =========================================================================
    // 2. CONEXÃO DA FERRAMENTA "1" (Cubo)
    // =========================================================================
    connect(m_ui->actTool1, &QAction::triggered, this, [this]() {
        
        // [ATENÇÃO] COLE A CHAMADA DA SUA ENGINE QUE GERA O POLÍGONO AQUI!
        // Exemplo: m_viewport->adicionarCubo();

        m_viewport->update(); // Redesenha a tela
        m_viewport->setFocus();
    });
    
    // Força a pintura do primeiro frame (o Grid) ao abrir
    m_viewport->update();
}

MainWindow::~MainWindow() = default;

} // namespace locus::gui