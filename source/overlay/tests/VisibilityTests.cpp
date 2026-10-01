#include "overlay/MainWindow.h"
#include "overlay/LocusViewport.h"
#include "overlay/tests/VisibilitySceneTests.h"

#include <QAction>
#include <QApplication>
#include <QHeaderView>
#include <QPersistentModelIndex>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>
#include <QWindow>

#include <cstdio>

namespace {
bool check(bool condition, const char* message)
{
    if (!condition) std::fprintf(stderr, "Visibility UI regression: %s\n", message);
    return condition;
}

bool run_window_tests(locus::overlay::MainWindow& window)
{
    if (!check(QTest::qWaitForWindowExposed(&window), "window not exposed")) return false;
    QTest::qWait(200);
    QToolButton* cubeButton = nullptr;
    for (auto* button : window.findChildren<QToolButton*>())
        if (button->text() == QString::fromUtf8("Cubo")) cubeButton = button;
    if (!check(cubeButton != nullptr, "cube tool missing")) return false;
    QTest::mouseClick(cubeButton, Qt::LeftButton);
    QTest::qWait(200);
    auto* tree = window.findChild<QTreeWidget*>();
    auto* viewport = window.findChild<locus::overlay::LocusViewport*>();
    if (!check(tree && viewport && tree->topLevelItemCount() == 1 && window.viewport_healthy(),
               "cube/viewport not initialized")) return false;
    auto* cube = tree->topLevelItem(0);
    const QPersistentModelIndex index(tree->model()->index(0, 1));
    const auto id = cube->data(0, Qt::UserRole);
    const auto name = cube->text(0);
    QSignalSpy resets(tree->model(), &QAbstractItemModel::modelReset);
    if (!check(tree->currentItem() == cube, "created cube not selected")) return false;

    // QWindow events retain the native Qt mouse route that reproduced the crash.
    for (int step = 0; step < 20; ++step) {
        const bool visible = step % 2 != 0;
        const QImage before = viewport->grabFramebuffer();
        const QPoint checkbox(tree->header()->sectionViewportPosition(1) + 8,
                              tree->visualItemRect(cube).center().y());
        const QPoint windowPosition = tree->viewport()->mapTo(&window, checkbox);
        QTest::mouseMove(window.windowHandle(), windowPosition);
        QTest::qWait(100);
        QTest::mouseClick(window.windowHandle(), Qt::LeftButton, Qt::NoModifier, windowPosition);
        QTest::qWait(200);
        if (!check(index.isValid() && resets.isEmpty(), "visibility reset/deleted the edited row")) return false;
        if (!check(tree->topLevelItem(0) == cube && cube->data(0, Qt::UserRole) == id && cube->text(0) == name,
                   "cube row identity/name changed")) return false;
        if (!check((cube->checkState(1) == Qt::Checked) == visible && tree->currentItem() == cube,
                   "visibility or selection inconsistent")) return false;
        const QImage after = viewport->grabFramebuffer();
        if (!check(window.viewport_healthy() && !before.isNull() && !after.isNull() && before != after,
                   "checkbox did not change the rendered framebuffer")) return false;
    }

    QAction* undo = nullptr;
    QAction* redo = nullptr;
    for (auto* action : window.findChildren<QAction*>()) {
        if (action->text() == QString::fromUtf8("Desfazer")) undo = action;
        if (action->text() == QString::fromUtf8("Refazer")) redo = action;
    }
    if (!check(undo && redo && undo->isEnabled(), "history actions missing")) return false;
    for (int step = 0; step < 4; ++step) {
        undo->trigger();
        QTest::qWait(80);
        if (!check(index.isValid() && cube->checkState(1) == Qt::Unchecked && redo->isEnabled(),
                   "UI undo did not hide cube")) return false;
        redo->trigger();
        QTest::qWait(80);
        if (!check(index.isValid() && cube->checkState(1) == Qt::Checked && window.viewport_healthy(),
                   "UI redo did not show cube")) return false;
    }
    if (!check(resets.isEmpty(), "history reset the tree")) return false;
    std::fprintf(stderr, "Visibility UI regression: 20 native checkbox clicks + 4 undo/redo cycles passed\n");
    return true;
}

void schedule_visibility_tests()
{
    QTimer::singleShot(1000, qApp, [] {
        for (auto* widget : QApplication::topLevelWidgets()) {
            if (auto* window = qobject_cast<locus::overlay::MainWindow*>(widget)) {
                const bool passed = run_window_tests(*window) && locus::overlay::tests::run_visibility_scene_tests();
                QCoreApplication::exit(passed ? 0 : 1);
                return;
            }
        }
        QCoreApplication::exit(1);
    });
}
}

// Reuse the production main, including surface format, theme and MainWindow.
Q_COREAPP_STARTUP_FUNCTION(schedule_visibility_tests)
