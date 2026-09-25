/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "MainWindow.h"
#include "ui_MainWindow.h" 
#include "Locus3DViewport.h"
#include <iostream>

namespace locus::gui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_ui(std::make_unique<Ui::MainWindow>()) {
    
    std::cout << "  -> Carregando definicoes do arquivo .ui..." << std::endl;
    m_ui->setupUi(this);

    if (!m_ui->viewport3D) {
        std::cerr << "  -> [ALERTA CRITICO] O widget 'viewport3D' nao foi encontrado no MainWindow.ui!" << std::endl;
    } else {
        std::cout << "  -> viewport3D localizado com sucesso." << std::endl;
        m_viewport = m_ui->viewport3D;
        m_viewport->setFocus();
    }

    this->setStyleSheet(R"(
        QMainWindow, QDockWidget, QWidget { background-color: #1e1e1e; color: #e0e0e0; }
        QTreeWidget { background-color: #181818; color: #ffffff; border: 1px solid #2d2d2d; }
        QDoubleSpinBox { background-color: #121212; color: #ffffff; border: 1px solid #333333; padding: 2px; }
        QToolBar { background-color: #1a1a1a; border-bottom: 1px solid #2d2d2d; spacing: 4px; }
        QToolButton { background-color: #2b2b2b; color: #ffffff; border-radius: 2px; padding: 3px 6px; }
        QToolButton:hover { background-color: #3d3d3d; }
        QLabel[text="X"] { color: #e04545; font-weight: bold; }
        QLabel[text="Y"] { color: #45b565; font-weight: bold; }
        QLabel[text="Z"] { color: #6565e0; font-weight: bold; }
    )");

    if (m_ui->actTool1) {
        connect(m_ui->actTool1, &QAction::triggered, this, [this]() {
            if (m_viewport) {
                m_viewport->adicionarCubo();
                m_viewport->setFocus();
            }
        });
    }

    if (m_viewport) {
        m_viewport->update();
    }

    std::cout << "  -> MainWindow inicializada completamente." << std::endl;
}

MainWindow::~MainWindow() = default;

} // namespace locus::gui