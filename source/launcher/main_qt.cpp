/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include <QApplication>
#include <QSurfaceFormat>
#include "gui/MainWindow.h" // Importa a janela isolada que blindamos na pasta GUI

int main(int argc, char* argv[]) {
    // 1. Configuração estrita do contexto OpenGL global (Perfil Core, moderno)
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(4, 1); 
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(4); 
    QSurfaceFormat::setDefaultFormat(format);

    // 2. Inicialização do motor de eventos do Qt
    QApplication app(argc, argv);
    app.setApplicationName("Locus3D");
    app.setOrganizationName("Locus3D Team");

    // 3. Instancia e exibe a nossa janela principal (que por sua vez carrega o .ui e a Viewport)[cite: 1]
    locus::gui::MainWindow window;
    window.show();

    // 4. Inicia o loop de interação do usuário
    return app.exec();
}