# Visibility crash investigation and regression

## Reproduction before the fix

Environment: Debug, `qt-mingw-debug`, Qt 6.11.2 MinGW 64-bit, GCC 13.1,
Windows, real OpenGL 4.5 viewport. The production `Locus3D` process received
SIGSEGV after toggling the selected cube's visibility. The same failure was
reproduced under GDB using `Locus3DQtVisibilityTests`, which links the unchanged
production `main.cpp`, MainWindow, stylesheet and viewport. Its driver creates
the cube and clicks the actual Mostrar indicator using QtTest QWindow events.
QWidget-only synthetic clicks initially did not reproduce the crash.

The captured failing stack contains:

```text
Thread 1 received signal SIGSEGV
QTreeWidget::setColumnCount(int)             Qt6Widgets.dll
QTreeWidget::setHeaderLabels(...)            Qt6Widgets.dll
QStyledItemDelegate::editorEvent(...)
QAbstractItemView::commitData(...)
QAbstractItemView::edit(...)
QAbstractItemView::mouseReleaseEvent(...)
...
QTest::mouseEvent(...)
reproduce(MainWindow&)
main                                      source/overlay/main.cpp
```

The distributed Qt DLL lacks private debug symbols. The first two names are
GDB's nearest exported symbols, not evidence that Locus3D changed headers or
column counts. The instruction faulted with an invalid heap pointer
(`rbx = 0xbaadf00d00000003`). A separate breakpoint immediately before the
old `tree_->clear()` captured the application call chain:

```text
OutlinerWidget::refresh                     OutlinerWidget.cpp:50 (before fix)
MainWindow::refresh_ui                     MainWindow.cpp:253
EditorBridge::changed
EditorBridge::execute                      EditorBridge.cpp:213
EditorBridge::set_visibility               EditorBridge.cpp:245
OutlinerWidget itemChanged lambda          OutlinerWidget.cpp:32
... Qt item delegate is still processing the checkbox edit
```

Full local evidence is retained in
`out/build/qt-mingw-debug/visibility-crash-gdb.log` and
`out/build/qt-mingw-debug/visibility-reset-gdb.log` (ignored build artifacts).

## Cause and correction

The Outliner signature included visibility and other metadata. Changing a
checkbox synchronously executed SetNodeVisibilityCommand, emitted bridge.changed,
and re-entered refresh_ui/OutlinerWidget::refresh before the delegate finished.
refresh cleared the entire tree, deleting the item and invalidating the model
index still in use by Qt. When Qt resumed the mouse-release edit it accessed
invalid state. The violation also occurred on hide; heap/timing determined
which later show operation crashed.

The signature now describes only node IDs and parent IDs. Metadata changes
update the existing items in place under QSignalBlocker; only a structural
scene change rebuilds the tree. Selection is still read from the backend.
No command, dirty flag, render sync, GPU cache or picking implementation was
changed, and no exception is swallowed or rendering update skipped.

Investigation of EditorSync, RenderSceneSync, SceneRenderAdapter,
MeshNodeRenderAdapter, MeshRenderCache, PickingSync and EditorViewport found
that the production viewport uses includeHiddenNodes=true: hidden objects
remain in RenderScene with visible/selectable=false. Selection highlights
filter them out; picking excludes them. Visibility does not change the mesh
revision or evict its GPU cache entry. The regression also explicitly uses
includeHiddenNodes=false to verify actual omission/reinsertion and retained
GpuMesh ownership. Gizmo rendering is exercised with the transform tool active.

## Regression coverage

- Actual MainWindow: 20 native Qt checkbox clicks, framebuffer changes,
  viewport health, retained selection/item identity, valid persistent index,
  no modelReset, and four Undo/Redo cycles through the application actions.
- One and two objects, selected and unselected cube: repeated
  visible -> hidden -> visible -> hidden -> visible, unchanged name, node,
  transform matrix, mesh identity, topology counts and mesh revision.
- Real cached rendering: stable valid GpuMesh pointers; the other object
  remains visible. Hidden objects receive no selection outline.
- Actual picking framebuffer: hidden cube ID absent; visible cube and the
  second object present and selectable. Selection remains consistent.
- Four Undo/Redo cycles per scene scenario, plus explicit omission/reinsertion
  without duplicate cache entries. GPU resources are released with GL current.

The new CTest entry is `overlay.integration.visibility` and requires a desktop
session with OpenGL 4.5. Build and run it with the current toolchain:

```powershell
cmake --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug --target Locus3D Locus3DQtVisibilityTests
$env:PATH = 'C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.11.2\mingw_64\bin;' + $env:PATH
$env:QT_QPA_PLATFORM_PLUGIN_PATH = 'C:\Qt\6.11.2\mingw_64\plugins\platforms'
ctest --test-dir out/build/qt-mingw-debug -R '^overlay\.integration\.visibility$' --output-on-failure -V
```

For a debugger run, use a command file (PowerShell native argument quoting can
split multiword `-ex` commands):

```powershell
& C:\Qt\Tools\mingw1310_64\bin\gdb.exe --batch -x source/overlay/tests/visibility.gdb --args out/build/qt-mingw-debug/source/overlay/Locus3DQtVisibilityTests.exe
```

Validation of this change passed the graphics regression and the existing
bridge, selection, render, gizmo, scene commands (including visibility) and
history suites. The production executable also passed `--smoke-ui`.
Desktop automation was unavailable because its Node runtime could not start;
the interaction verification above used real Qt window events, not a claim
that a human manually clicked the corrected application.
