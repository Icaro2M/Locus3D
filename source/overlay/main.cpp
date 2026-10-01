#include "overlay/MainWindow.h"
#include <QApplication>
#include <QSurfaceFormat>
#include <QTimer>
#include <cstdio>
#ifdef _WIN32
#include <windows.h>
#endif

namespace {
void qt_message(QtMsgType type, const QMessageLogContext&, const QString& message)
{
    std::fprintf(stderr, "[Qt %d] %s\n", static_cast<int>(type), message.toUtf8().constData());
}
}

int main(int argc, char** argv)
{
#ifdef _WIN32
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE* output = nullptr;
        freopen_s(&output, "CONOUT$", "w", stdout);
        freopen_s(&output, "CONOUT$", "w", stderr);
    }
#endif
    QSurfaceFormat format;
    format.setVersion(4, 5);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);
    qInstallMessageHandler(qt_message);
    QApplication app(argc, argv);
    app.setApplicationName("Locus3D");
    app.setStyle("Fusion");
    app.setStyleSheet(R"(
        QWidget { background: #20262d; color: #e2e8ef; font: 10pt 'Segoe UI'; }
        QMainWindow, QMenuBar, QMenu, QDockWidget { background: #20262d; }
        QMenuBar::item:selected, QMenu::item:selected { background: #38546b; }
        QToolBar { background: #29313a; border-bottom: 1px solid #3a4652; spacing: 5px; padding: 5px; }
        QToolButton { border-radius: 4px; padding: 6px 9px; }
        QToolButton:hover, QToolButton:checked { background: #357296; color: white; }
        QDockWidget::title { background: #29313a; padding: 7px; }
        QTreeWidget, QListWidget { background: #252d36; alternate-background-color: #2b3540; border: 1px solid #3a4652; }
        QTreeWidget::item:selected, QListWidget::item:selected { background: #306d92; }
        QHeaderView::section { background: #303b46; border: 0; padding: 5px; }
        QDoubleSpinBox { background: #303a45; border: 1px solid #4b5a69; border-radius: 3px; padding: 3px; }
        QDoubleSpinBox:focus { border-color: #54a4d0; }
        QGroupBox { border: 1px solid #3a4652; border-radius: 4px; margin-top: 14px; padding: 8px; }
        QGroupBox::title { subcontrol-origin: margin; left: 8px; color: #9bc9e3; }
        QLabel#panelHeading { color: #84b6d1; font-weight: bold; letter-spacing: 1px; }
        QLabel#mutedLabel { color: #94a3b1; }
        QStatusBar { background: #29313a; border-top: 1px solid #3a4652; }
    )");
    locus::overlay::MainWindow window;
    window.show();
    if (app.arguments().contains("--smoke-ui")) {
        QTimer::singleShot(1800, &app, [&app, &window] {
            const bool healthy = window.viewport_healthy();
            std::fprintf(stderr, "Locus3D smoke: %s\n", healthy ? "ready" : "failed");
            app.exit(healthy ? 0 : 1);
        });
    }
    return app.exec();
}
