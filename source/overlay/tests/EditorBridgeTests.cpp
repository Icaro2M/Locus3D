#include "overlay/EditorBridge.h"

#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>

#include <cmath>
#include <cstdio>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir temporary;
    if (!temporary.isValid()) return 1;

    auto verify = [](bool condition, const char* message) {
        if (!condition) std::fprintf(stderr, "EditorBridge integration: %s\n", message);
        return condition;
    };
    const QString modelPath = temporary.filePath("triangle.obj");
    QFile model(modelPath);
    if (!verify(model.open(QIODevice::WriteOnly), "cannot create OBJ fixture")) return 1;
    model.write("v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n");
    model.close();

    locus::overlay::EditorBridge bridge;
    QString error;
    QObject::connect(&bridge, &locus::overlay::EditorBridge::errorOccurred,
                     &app, [&error](const QString& message) { error = message; });
    const auto initialCount = bridge.nodes().size();
    bridge.import_mesh(modelPath);
    if (!verify(error.isEmpty() && bridge.nodes().size() == initialCount + 1,
                "OBJ import did not create a scene node")) return 1;

    const auto node = bridge.nodes().back().id;
    bridge.select_node(node);
    const auto mesh = bridge.selected_mesh();
    if (!verify(mesh && mesh->vertices == 3 && mesh->faces == 1,
                "selection did not expose imported topology")) return 1;

    bridge.set_transform(0, 12.5);
    auto transform = bridge.selected_transform();
    if (!verify(transform && std::fabs(transform->values[0] - 12.5) < 0.001 && bridge.dirty(),
                "transform command did not update the document")) return 1;
    bridge.undo();
    transform = bridge.selected_transform();
    if (!verify(transform && std::fabs(transform->values[0]) < 0.001,
                "undo did not restore the previous transform")) return 1;
    bridge.redo();
    transform = bridge.selected_transform();
    if (!verify(transform && std::fabs(transform->values[0] - 12.5) < 0.001,
                "redo did not restore the edited transform")) return 1;

    bridge.set_minimum_wall(1.2);
    bridge.set_minimum_feature(0.5);
    bridge.set_maximum_overhang(50);
    if (!verify(bridge.minimum_wall() == 1.2 && bridge.minimum_feature() == 0.5 &&
                bridge.maximum_overhang() == 50, "manufacturing settings did not reach the backend")) return 1;

    const QString invalidPath = modelPath + "/project.locus";
    if (!verify(!bridge.save_document(invalidPath) && bridge.dirty() && !error.isEmpty(),
                "failed save incorrectly reported success")) return 1;
    error.clear();
    const QString projectPath = temporary.filePath("project.locus");
    if (!verify(bridge.save_document(projectPath) && !bridge.dirty() && QFile::exists(projectPath),
                "project save failed")) return 1;

    bridge.new_document();
    if (!verify(bridge.nodes().size() == initialCount, "new document retained the previous scene")) return 1;
    bridge.open_document(projectPath);
    if (!verify(error.isEmpty() && bridge.nodes().size() == initialCount + 1 && !bridge.dirty(),
                "saved project did not reopen")) return 1;
    bridge.select_node(bridge.nodes().back().id);
    transform = bridge.selected_transform();
    if (!verify(transform && std::fabs(transform->values[0] - 12.5) < 0.001,
                "reopened project lost its transform")) return 1;

    const QString exportPath = temporary.filePath("export.obj");
    bridge.export_selected_mesh(exportPath);
    QFile exported(exportPath);
    if (!verify(error.isEmpty() && exported.open(QIODevice::ReadOnly), "selected mesh export failed")) return 1;
    if (!verify(exported.readAll().contains("v 12.5 0 0"),
                "exported geometry did not include the world transform")) return 1;
    return 0;
}
