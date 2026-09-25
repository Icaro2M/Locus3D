/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef _WIN32
#include <windows.h>
#include <cstdio>
#endif

#include <QApplication>
#include <QSurfaceFormat>
#include <QDebug>
#include <iostream>
#include "gui/MainWindow.h"

// Redireciona todos os logs e avisos internos do Qt direto para o terminal do VS Code
void qtMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    std::cout << "[Qt] " << msg.toStdString() << std::endl;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
#endif

    qInstallMessageHandler(qtMessageOutput);

    std::cout << "[Locus3D 1/5] Configurando perfil OpenGL..." << std::endl;

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(4, 5); // <<-- Altere de (4, 1) para (4, 5)
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(4); 
    QSurfaceFormat::setDefaultFormat(format);

    std::cout << "[Locus3D 2/5] Instanciando QApplication..." << std::endl;
    QApplication app(argc, argv);
    app.setApplicationName("Locus3D");
    app.setOrganizationName("Locus3D Team");

    try {
        std::cout << "[Locus3D 3/5] Construindo MainWindow..." << std::endl;
        locus::gui::MainWindow window;

        std::cout << "[Locus3D 4/5] Exibindo janela (show)..." << std::endl;
        window.show();

        std::cout << "[Locus3D 5/5] Janela aberta com sucesso! Executando loop..." << std::endl;
        return app.exec();
    } catch (const std::exception& e) {
        std::cerr << "[Locus3D ERRO] Excecao capturada: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "[Locus3D ERRO] Falha critica desconhecida." << std::endl;
        return 1;
    }
}