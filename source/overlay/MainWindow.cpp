#include "overlay/MainWindow.h"
#include "overlay/LocusViewport.h"
#include "overlay/OutlinerWidget.h"
#include "overlay/TransformWidget.h"
#include "overlay/ManufacturingWidget.h"
#include "editor/tools/mesh/edge/BevelTool.h"
#include "editor/tools/mesh/edge/EdgeSlideTool.h"
#include "editor/tools/mesh/face/ExtrudeFaceTool.h"
#include "editor/tools/mesh/face/InsetFaceTool.h"
#include "editor/tools/mesh/face/SolidifyTool.h"
#include "editor/tools/mesh/topology/LoopCutTool.h"
#include "kernel/geometry/primitives/PrimitiveParameters.h"

#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenuBar>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <functional>
#include <string_view>

namespace locus::overlay {
namespace {
QAction* add_action(QMenu* menu, const QString& name, const QKeySequence& shortcut,
                    QObject* receiver, std::function<void()> callback)
{
    auto* action = menu->addAction(name);
    if (!shortcut.isEmpty()) action->setShortcut(shortcut);
    QObject::connect(action, &QAction::triggered, receiver, [callback = std::move(callback)] { callback(); });
    return action;
}
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setObjectName("Locus3DMainWindow");
    setWindowTitle(tr("Locus3D — Sem título"));
    resize(1440, 900);
    setMinimumSize(960, 600);
    setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowTabbedDocks);

    viewport_ = new LocusViewport(bridge_, this);
    setCentralWidget(viewport_);
    outliner_ = new OutlinerWidget(bridge_, this);
    transform_ = new TransformWidget(bridge_, this);
    manufacturing_ = new ManufacturingWidget(bridge_, this);
    auto* sceneDock = new QDockWidget(tr("Estrutura"), this);
    sceneDock->setObjectName("SceneDock");
    sceneDock->setWidget(outliner_);
    sceneDock->setMinimumWidth(230);
    addDockWidget(Qt::LeftDockWidgetArea, sceneDock);
    auto* propertiesDock = new QDockWidget(tr("Propriedades"), this);
    propertiesDock->setObjectName("PropertiesDock");
    propertiesDock->setWidget(transform_);
    propertiesDock->setMinimumWidth(260);
    addDockWidget(Qt::RightDockWidgetArea, propertiesDock);
    auto* analysisDock = new QDockWidget(tr("Impressão 3D"), this);
    analysisDock->setObjectName("AnalysisDock");
    analysisDock->setWidget(manufacturing_);
    addDockWidget(Qt::RightDockWidgetArea, analysisDock);
    tabifyDockWidget(propertiesDock, analysisDock);
    propertiesDock->raise();

    auto* file = menuBar()->addMenu(tr("Arquivo"));
    add_action(file, tr("Novo projeto"), QKeySequence::New, this, [this] {
        if (confirm_discard()) bridge_.new_document();
    });
    add_action(file, tr("Abrir projeto…"), QKeySequence::Open, this, [this] { open_project(); });
    add_action(file, tr("Importar modelo STL/OBJ…"), QKeySequence(Qt::CTRL | Qt::Key_I), this,
        [this] { import_model(); });
    add_action(file, tr("Exportar malha selecionada…"), {}, this, [this] { export_model(); });
    file->addSeparator();
    add_action(file, tr("Salvar"), QKeySequence::Save, this, [this] {
        if (bridge_.document().has_path()) bridge_.save_document(); else save_as();
    });
    add_action(file, tr("Salvar como…"), QKeySequence::SaveAs, this, [this] { save_as(); });
    file->addSeparator();
    add_action(file, tr("Sair"), QKeySequence::Quit, this, [this] { close(); });

    auto* edit = menuBar()->addMenu(tr("Editar"));
    undoAction_ = add_action(edit, tr("Desfazer"), QKeySequence::Undo, this, [this] { bridge_.undo(); });
    redoAction_ = add_action(edit, tr("Refazer"), QKeySequence::Redo, this, [this] { bridge_.redo(); });
    edit->addSeparator();
    deleteAction_ = add_action(edit, tr("Excluir seleção"), QKeySequence::Delete, this,
        [this] { bridge_.delete_selection(); });

    auto* view = menuBar()->addMenu(tr("Vista"));
    add_action(view, tr("Frente"), QKeySequence(Qt::Key_F1), this, [this] {
        bridge_.viewport().set_view_orientation(application::ViewOrientation::Front); viewport_->update(); });
    add_action(view, tr("Topo"), QKeySequence(Qt::Key_F2), this, [this] {
        bridge_.viewport().set_view_orientation(application::ViewOrientation::Top); viewport_->update(); });
    add_action(view, tr("Perspectiva / ortográfica"), QKeySequence(Qt::Key_F3), this, [this] {
        bridge_.viewport().toggle_projection_mode(); viewport_->update(); });
    view->addSeparator();
    add_action(view, tr("Sólido / aramado"), {}, this, [this] {
        bridge_.viewport().toggle_shading_mode(); viewport_->update(); });
    add_action(view, tr("Orientação das faces"), {}, this, [this] {
        bridge_.viewport().toggle_face_orientation(); viewport_->update(); });
    view->addSeparator();
    view->addAction(sceneDock->toggleViewAction());
    view->addAction(propertiesDock->toggleViewAction());
    view->addAction(analysisDock->toggleViewAction());

    auto* toolbar = addToolBar(tr("Ferramentas"));
    toolbar->setObjectName("ToolsToolbar");
    toolbar->setMovable(false);
    auto* modeGroup = new QActionGroup(this);
    auto addMode = [this, toolbar, modeGroup](const QString& title, editor::SelectionGranularity value, int key) {
        auto* action = toolbar->addAction(title);
        action->setShortcut(QKeySequence(key));
        action->setCheckable(true);
        modeGroup->addAction(action);
        connect(action, &QAction::triggered, this, [this, value] {
            bridge_.set_granularity(value);
            bridge_.activate_select();
        });
        return action;
    };
    objectAction_ = addMode(tr("Objeto"), editor::SelectionGranularity::Object, Qt::Key_1);
    vertexAction_ = addMode(tr("Vértice"), editor::SelectionGranularity::Vertex, Qt::Key_2);
    edgeAction_ = addMode(tr("Aresta"), editor::SelectionGranularity::Edge, Qt::Key_3);
    faceAction_ = addMode(tr("Face"), editor::SelectionGranularity::Face, Qt::Key_4);
    toolbar->addSeparator();
    auto addTransform = [this, toolbar](const QString& title, editor::GizmoMode mode, int key) {
        auto* action = toolbar->addAction(title);
        action->setShortcut(QKeySequence(key));
        connect(action, &QAction::triggered, this, [this, mode] { bridge_.activate_transform(mode); });
    };
    addTransform(tr("Mover"), editor::GizmoMode::Translate, Qt::Key_G);
    addTransform(tr("Rotacionar"), editor::GizmoMode::Rotate, Qt::Key_R);
    addTransform(tr("Escalar"), editor::GizmoMode::Scale, Qt::Key_S);
    toolbar->addSeparator();

    auto* tools = menuBar()->addMenu(tr("Modelagem"));
    auto* addMenu = tools->addMenu(tr("Adicionar"));
    auto addPrimitive = [this, toolbar, addMenu](const QString& title, kernel::geometry::PrimitiveType type) {
        auto trigger = [this, type] {
            bridge_.add_primitive(type);
            bridge_.activate_transform(editor::GizmoMode::Translate);
            viewport_->update();
        };
        add_action(addMenu, title, {}, this, trigger);
        auto* toolbarAction = toolbar->addAction(title);
        connect(toolbarAction, &QAction::triggered, this, trigger);
    };
    addPrimitive(tr("Cubo"), kernel::geometry::PrimitiveType::Box);
    addPrimitive(tr("Esfera"), kernel::geometry::PrimitiveType::Sphere);
    addPrimitive(tr("Cilindro"), kernel::geometry::PrimitiveType::Cylinder);
    addPrimitive(tr("Cone"), kernel::geometry::PrimitiveType::Cone);
    addPrimitive(tr("Toro"), kernel::geometry::PrimitiveType::Torus);
    toolbar->addSeparator();
    tools->addSeparator();
    auto addTool = [this, tools](const QString& title, std::string_view id) {
        add_action(tools, title, {}, this, [this, id = std::string(id)] { bridge_.activate_mesh_tool(QString::fromStdString(id)); });
    };
    addTool(tr("Extrudir face"), editor::ExtrudeFaceTool::Id);
    addTool(tr("Inserir face"), editor::InsetFaceTool::Id);
    addTool(tr("Solidificar"), editor::SolidifyTool::Id);
    addTool(tr("Deslizar aresta"), editor::EdgeSlideTool::Id);
    addTool(tr("Chanfro"), editor::BevelTool::Id);
    addTool(tr("Corte em loop"), editor::LoopCutTool::Id);

    auto* analysis = menuBar()->addMenu(tr("Análise"));
    analysisAction_ = analysis->addAction(tr("Mostrar diagnósticos de impressão"));
    analysisAction_->setCheckable(true);
    connect(analysisAction_, &QAction::toggled, this, [this, analysisDock](bool enabled) {
        bridge_.set_analysis_enabled(enabled);
        analysisDock->raise();
    });

    statusBar()->showMessage(tr("Pronto. Importe um modelo STL ou OBJ para começar."));
    connect(&bridge_, &EditorBridge::changed, this, &MainWindow::refresh_ui);
    connect(&bridge_, &EditorBridge::statusChanged, statusBar(), [this](const QString& text) {
        statusBar()->showMessage(text, 5000);
    });
    connect(&bridge_, &EditorBridge::errorOccurred, this, [this](const QString& text) {
        statusBar()->showMessage(text, 8000);
        if (!QCoreApplication::arguments().contains("--smoke-ui"))
            QTimer::singleShot(0, this, [this, text] { QMessageBox::critical(this, tr("Locus3D"), text); });
    });
    auto* refreshTimer = new QTimer(this);
    refreshTimer->setInterval(200);
    connect(refreshTimer, &QTimer::timeout, this, &MainWindow::refresh_ui);
    refreshTimer->start();
    refresh_ui();
}

MainWindow::~MainWindow() { delete viewport_; }
bool MainWindow::viewport_healthy() const noexcept { return viewport_ && viewport_->healthy(); }

bool MainWindow::save_as()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("Salvar projeto"),
        bridge_.document().has_path() ? QString::fromStdWString(bridge_.document().path().wstring()) : QString{},
        tr("Projeto Locus3D (*.locus)"));
    if (path.isEmpty()) return false;
    return bridge_.save_document(path);
}

bool MainWindow::confirm_discard()
{
    if (!bridge_.dirty()) return true;
    const auto answer = QMessageBox::question(this, tr("Alterações não salvas"),
        tr("Salvar o projeto antes de continuar?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (answer == QMessageBox::Cancel) return false;
    if (answer == QMessageBox::Discard) return true;
    return bridge_.document().has_path() ? bridge_.save_document() : save_as();
}

void MainWindow::open_project()
{
    if (!confirm_discard()) return;
    const QString path = QFileDialog::getOpenFileName(this, tr("Abrir projeto"), {}, tr("Projeto Locus3D (*.locus)"));
    if (!path.isEmpty()) bridge_.open_document(path);
}

void MainWindow::import_model()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Importar modelo"), {}, tr("Modelos 3D (*.stl *.obj)"));
    if (path.isEmpty()) return;
    statusBar()->showMessage(tr("Importando modelo…"));
    QApplication::setOverrideCursor(Qt::WaitCursor);
    bridge_.import_mesh(path);
    QApplication::restoreOverrideCursor();
    viewport_->update();
}

void MainWindow::export_model()
{
    if (!bridge_.selected_mesh()) {
        QMessageBox::information(this, tr("Exportar"), tr("Selecione uma malha antes de exportar."));
        return;
    }
    QString filter;
    QString path = QFileDialog::getSaveFileName(this, tr("Exportar malha"), {},
        tr("STL (*.stl);;Wavefront OBJ (*.obj)"), &filter);
    if (path.isEmpty()) return;
    if (QFileInfo(path).suffix().isEmpty()) path += filter.contains("*.obj") ? ".obj" : ".stl";
    bridge_.export_selected_mesh(path);
}

void MainWindow::refresh_ui()
{
    outliner_->refresh();
    transform_->refresh();
    manufacturing_->refresh();
    setWindowTitle(tr("Locus3D — %1%2").arg(bridge_.document_name(), bridge_.dirty() ? " *" : ""));
    undoAction_->setEnabled(bridge_.can_undo());
    redoAction_->setEnabled(bridge_.can_redo());
    deleteAction_->setEnabled(bridge_.can_delete());
    objectAction_->setChecked(bridge_.granularity() == editor::SelectionGranularity::Object);
    vertexAction_->setChecked(bridge_.granularity() == editor::SelectionGranularity::Vertex);
    edgeAction_->setChecked(bridge_.granularity() == editor::SelectionGranularity::Edge);
    faceAction_->setChecked(bridge_.granularity() == editor::SelectionGranularity::Face);
    QSignalBlocker block(analysisAction_);
    analysisAction_->setChecked(bridge_.analysis_enabled());
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (confirm_discard()) event->accept(); else event->ignore();
}
}
