# Locus3D - Arquitetura Técnica

Este documento descreve a arquitetura técnica estabelecida para o Locus3D, detalhando a separação entre o motor 3D (Backend) e a nova interface de utilizador (Frontend em Qt6), bem como o pipeline gráfico. 

**Nota de Leitura:** O estado de cada componente ou decisão arquitetural está classificado como **[DECIDIDO]**, **[PROPOSTO]** ou **[AINDA NÃO DEFINIDO]**.

## 1. Visão Geral da Arquitetura
**[DECIDIDO]** O Locus3D é uma aplicação *Desktop Standalone* desenvolvida em C++ moderno. A arquitetura segue o padrão *Model-View-Controller* (MVC) adaptado para motores gráficos, onde o estado da cena geométrica (Model) e a lógica de ferramentas (Controller) estão isolados da interface gráfica Qt (View).

## 2. Organização dos Módulos (File Structure)
**[DECIDIDO]** A base de código está rigidamente dividida para impedir ficheiros monolíticos e acoplamento indevido:
*   `source/kernel/`: Lógica matemática pura, topologia (`TopologyBuilder`) e algoritmos agnósticos a UI/Gráficos.
*   `source/graphics/`: Abstrações da GPU (`VertexArray`, `Buffer`, `ShaderManager`), renderizadores (`ScreenSpaceLineRenderer`) e ciclo de vida OpenGL.
*   `source/editor/`: Estado da aplicação 3D (`Editor`, `SelectionController`, `ToolManager`, `DocumentSession`).
*   `source/application/`: Gestão de viewports e eventos de entrada globais (`EditorViewport`).
*   `source/gui/`: Integração base do Qt6 (ex: `MainWindow`, `Locus3DViewport`).
*   `source/overlay/`: **[DECIDIDO]** Diretório alvo exclusivo para todos os novos painéis, *widgets* e elementos flutuantes da interface Qt (ex: *Inspector*, *Outliner*, *Toolbar*).

## 3. Frontend (Interface Gráfica)
**[DECIDIDO]** 
*   **Framework:** Qt6 (Módulos: Core, Gui, Widgets, OpenGLWidgets).
*   **Janela Principal:** `QMainWindow` gere os painéis acopláveis (*Dock Widgets*) e o tema visual (*Dark Theme* aplicado via Qt Style Sheets / QSS).
*   **Viewport 3D:** A classe `Locus3DViewport` herda de `QOpenGLWidget` (e possivelmente `QOpenGLFunctions`). Atua como a ponte entre o sistema de janelas do SO e o motor gráfico, injetando eventos de rato e teclado para as ferramentas do `EditorViewport`.
*   **Gestão de Ficheiros:** A implementação de todos os componentes UI deve ter uma estrita separação entre ficheiro de cabeçalho (`.h`) e ficheiro de implementação (`.cpp`).

## 4. Backend (Motor 3D e Ferramentas)
**[DECIDIDO]**
*   **Scene Graph:** O mundo 3D é gerido através de um grafo de cena que contém `SceneNode` e `MeshNode`.
*   **Tool Manager:** Um sistema de estado finito (FSM) que gere qual a ferramenta ativa (Órbita, *Pan*, Zoom, Modo Vértice/Aresta/Face, Translação, Rotação, Escala).
*   **Selection Controller:** Sub-sistema dedicado a resolver consultas de *Raycasting/Picking*, decidindo qual a malha, vértice ou eixo do *Gizmo* foi clicado.
*   **Document Session e History:** Gere as instâncias do projeto ativo e fornece o suporte transacional para as operações de "Desfazer/Refazer" (`Ctrl+Z`).

## 5. Pipeline Gráfico e GPU
**[DECIDIDO]**
*   **API Alvo:** OpenGL 4.5 Core Profile.
*   **Carregador (Loader):** `GLAD` (com geração explícita). A inicialização possui um *fallback* de segurança vitalício para procurar os ponteiros de função diretamente em `opengl32.dll` se o `QOpenGLContext` falhar.
*   **Direct State Access (DSA):** A arquitetura gráfica adota o DSA (ex: `glCreateVertexArrays`) para reduzir mudanças de estado (*state changes*) globais.
*   **Prevenção de Falhas (Fallback):** Foi implementada uma regra estrutural na abstração gráfica (em `VertexArray.cpp`) onde as chamadas DSA verificam a validade dos ponteiros (`!= nullptr`). Em caso de falha silenciosa do controlador da placa gráfica, o código deve usar as rotinas compatíveis de legado (`glGenVertexArrays`).
*   **Shaders:** A compilação é assíncrona ou diferida na inicialização, lendo de `source/assets/shaders/`. O motor procura especificamente caminhos exatos (como `viewport/screen_space_line_vert.glsl`).
*   **Picking (Seleção):** A deteção de objetos é feita via renderização fora do ecrã (*offscreen rendering*) num *framebuffer* de seleção (*picking buffer*), lendo o pixel colorido exato clicado pelo rato.

## 6. Observabilidade e Depuração
**[DECIDIDO]**
*   **Startup Logging:** O motor escreve ativamente passos críticos num ficheiro chamado `locus3d_startup.log` (via chamadas a `append_startup_log`). Utilizado para rastrear até que *shader* ou *buffer* a inicialização conseguiu progredir.
*   **Console do Windows:** No Windows, a aplicação anexa o processo de terminal através de `AttachConsole(ATTACH_PARENT_PROCESS)` para impedir a supressão natural de `stdout`/`stderr` por aplicações GUI.
*   **Redirecionamento Qt:** Utilização de `qInstallMessageHandler` no `main_qt.cpp` para capturar todos os `qDebug()`, `qWarning()`, e `qCritical()` e imprimi-los no terminal do Visual Studio Code.

## 7. Build System e Deploy
**[DECIDIDO]** 
*   **Gerador de Projeto:** CMake.
*   **Compilador:** MinGW-w64 / GCC (no ambiente Windows).
*   **Geração Automática do Qt:** O CMake está configurado para correr o MOC (*Meta-Object Compiler*), UIC e RCC do Qt de forma automática (`set(CMAKE_AUTOMOC ON)`, etc.).

## 8. Elementos AINDA NÃO DEFINIDOS
*   **[AINDA NÃO DEFINIDO]** Implementação exata e final do protocolo de *Data Binding* (se através de `Qt Signals/Slots` puros ou de eventos do motor que forçam uma repintura completa do Qt).
*   **[AINDA NÃO DEFINIDO]** Modelo final de persistência (formato de gravação nativo do ficheiro 3D, exportação para OBJ/FBX/GLTF).
*   **[AINDA NÃO DEFINIDO]** Infraestrutura para computação paralela ou renderização *multithread* (atualmente o ciclo gráfico é presumido como estando atado à *thread* principal ou da janela Qt).