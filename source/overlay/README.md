# Locus3D Qt desktop

`Locus3D` is the desktop application. The older GLFW/ImGui test harness remains available as `Locus3DDemo`. The Qt UI uses the existing `DocumentSession`, `EditorViewport`, editor commands, tools, history, importers, exporters and manufacturing analysis through `EditorBridge`.

The development toolchain is Qt **6.11.2 MinGW 64-bit** at
`C:/Qt/6.11.2/mingw_64`, with GCC/G++ **13.1.0** from
`C:/Qt/Tools/mingw1310_64/bin`. CMake 3.21 or newer is required for the version 3
presets. The generator is `MinGW Makefiles`; the preset pins `gcc.exe`, `g++.exe`
and `mingw32-make.exe`, and supplies the toolchain/runtime PATH automatically.

Open PowerShell in the repository root and run:

```powershell
cmake --preset qt-mingw-debug
cmake --build --preset qt-mingw-debug
cmake --build --preset qt-mingw-bridge-debug
ctest --preset qt-mingw-bridge-debug

# The configure/build presets supply PATH to their own processes.
# A separately launched application also needs the Qt/MinGW runtime environment.
$env:PATH = 'C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.11.2\mingw_64\bin;' + $env:PATH
$env:QT_QPA_PLATFORM_PLUGIN_PATH = 'C:\Qt\6.11.2\mingw_64\plugins\platforms'
& .\out\build\qt-mingw-debug\source\overlay\Locus3D.exe --smoke-ui
```

Omit `--smoke-ui` to use the application normally. Smoke mode opens the application,
verifies that the first OpenGL frame rendered, and exits automatically.
For Release, use `cmake --preset qt-mingw-release` followed by
`cmake --build --preset qt-mingw-release`. Its executable is under
`out/build/qt-mingw-release/source/overlay/`.
To copy the required Qt runtime next to the Debug executable, build the optional
`cmake --build out/build/qt-mingw-debug --target Locus3DDeploy` target.

## VS Code

Open the repository folder and install the recommended CMake Tools and C/C++
extensions. In **CMake: Select Configure Preset**, select `qt-mingw-debug`, then
select the build preset of the same name and `Locus3D` as the launch target.
Workspace settings use presets and disable Visual Studio developer-environment
injection; IntelliSense uses the CMake configuration with a MinGW fallback.

**Ctrl+Shift+B** runs the default task, which configures and builds `Locus3D` with
the Qt/MinGW Debug preset. **F5** uses the checked-in `Locus3D - Qt/MinGW Debug`
launch configuration, builds first, and debugs with the matching MinGW GDB.
**Ctrl+F5** uses that same executable without debugging. The `Locus3D: run
Qt/MinGW Debug`, `Locus3D: smoke Qt/MinGW Debug` and bridge-test tasks are also
available through **Tasks: Run Task**. Launches and terminals have the Qt/MinGW
DLL PATH and Qt platform-plugin path set locally for this workspace.

The former `x64-debug` / `x64-release` names are compatibility aliases for the
Qt/MinGW presets and now share their new build directories. The old
`out/build/x64-debug` MSVC cache is not reused. Never switch compilers in an
existing build cache: use the new preset directory, or use **CMake: Delete Cache
and Reconfigure** with the selected Qt/MinGW preset if that directory was
configured manually with another compiler.

The x86 MSVC presets remain legacy configurations with `LOCUS3D_BUILD_QT=OFF`;
they do not build the Qt desktop. The desktop build presets explicitly target
`Locus3D`. The GLFW demo is outside this development flow: its vendored
`glfw3.lib` was built for MSVC and cannot be linked into a MinGW demo without a
matching GLFW library. No compiler flag such as `/Zc:__cplusplus` can replace
consistent Qt and compiler ABIs.

Preset/environment behavior follows the official
[CMake presets reference](https://cmake.org/cmake/help/v3.22/manual/cmake-presets.7.html)
and [CMake Tools preset workflow](https://github.com/microsoft/vscode-cmake-tools/blob/main/docs/cmake-presets.md).

The current backend supports scene editing, STL/OBJ import and export, native `.locus` projects, modeling tools and FDM geometry diagnostics. It does not provide slicing or G-code generation; the interface therefore exposes diagnostics but no slicing controls. Import and analysis currently run on the UI/render thread, so large meshes may temporarily pause interaction.

The [visibility regression and debugger notes](tests/README.md) describe the
Outliner item-lifetime fix and the real Qt/OpenGL test `overlay.integration.visibility`.
