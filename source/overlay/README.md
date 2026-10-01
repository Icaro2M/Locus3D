# Locus3D Qt desktop

`Locus3D` is the desktop application. The older GLFW/ImGui test harness remains available as `Locus3DDemo`. The Qt UI uses the existing `DocumentSession`, `EditorViewport`, editor commands, tools, history, importers, exporters and manufacturing analysis through `EditorBridge`.

On the current Windows development machine, open PowerShell in the repository root and run:

```powershell
$env:PATH = 'C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.11.2\mingw_64\bin;' + $env:PATH
cmake -S . -B out/build/qt-make-debug -G 'MinGW Makefiles' `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_C_COMPILER=C:/Qt/Tools/mingw1310_64/bin/gcc.exe `
  -DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe `
  -DCMAKE_PREFIX_PATH=C:/Qt/6.11.2/mingw_64 `
  -DLOCUS3D_BUILD_TESTS=ON
cmake --build out/build/qt-make-debug --target Locus3D --parallel 4
cmake --build out/build/qt-make-debug --target Locus3DQtBridgeTests --parallel 4
ctest --test-dir out/build/qt-make-debug -R 'overlay.integration.bridge' --output-on-failure
```

Run `out/build/qt-make-debug/source/overlay/Locus3D.exe` from the same shell. `--smoke-ui` opens the application, verifies that the first OpenGL frame rendered, and exits automatically. To copy the required Qt runtime next to the executable, build the optional `Locus3DDeploy` target.

The current backend supports scene editing, STL/OBJ import and export, native `.locus` projects, modeling tools and FDM geometry diagnostics. It does not provide slicing or G-code generation; the interface therefore exposes diagnostics but no slicing controls. Import and analysis currently run on the UI/render thread, so large meshes may temporarily pause interaction.
