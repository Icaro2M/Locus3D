# Locus3D - Integrações e Comunicação Sistémica

Neste projeto, o conceito de "integrações" não se refere apenas a APIs web de terceiros, mas sim às integrações críticas entre as barreiras do sistema operativo (Windows), o *framework* de interface (Qt6) e a API gráfica de baixo nível (OpenGL). O historial de desenvolvimento revelou que a estabilidade do Locus3D depende estritamente destas ligações.

Abaixo estão documentadas todas as integrações de fronteira abordadas e implementadas.

---

## 1. Integração Qt6 <-> OpenGL Backend (`QOpenGLWidget`)
* **Finalidade:** Embutir o motor de renderização nativo (desenhado inicialmente para GLFW) dentro da nova arquitetura de interface do Qt6.
* **Dados Enviados (Frontend -> Backend):** Eventos de rato (cliques, arrastos, scroll), eventos de teclado, e redimensionamento da janela (`resizeEvent` com *width* e *height*).
* **Dados Recebidos (Backend -> Frontend):** O *buffer* de cor final renderizado pela GPU e sinais de que a cena foi atualizada (marcada como *dirty*).
* **Fluxo Esperado:** 
  1. A classe `Locus3DViewport` (derivada de `QOpenGLWidget`) é instanciada.
  2. No método `initializeGL()`, o perfil de superfície é verificado e o `EditorViewport` nativo é inicializado.
  3. No método `paintGL()`, o motor desenha o *frame*.
* **Decisões já tomadas:** A `QSurfaceFormat` deve ser forçada globalmente no `main_qt.cpp` para **OpenGL 4.5 Core Profile**. Sem isto, o motor recusa-se a arrancar corretamente devido à dependência em funções DSA (Direct State Access).
* **Limitações Conhecidas:** A Viewport Qt pode, no momento da sua construção geométrica inicial, reportar dimensões nulas (0x0). O motor não está preparado para criar matrizes de projeção ou *buffers* com tamanho 0. Foi decidido tratar estas dimensões na camada Qt, forçando um mínimo de 1 pixel.

---

## 2. Integração GLAD <-> Windows OS (`opengl32.dll` Fallback)
* **Finalidade:** Carregar os ponteiros das funções OpenGL modernas (DSA, ex: `glCreateVertexArrays`) para dentro do espaço de memória da aplicação.
* **Autenticação / Permissões:** Requer privilégios base de execução do sistema operativo para invocar a API Win32 (`LoadLibraryA`).
* **Fluxo Esperado:** 
  1. O sistema tenta usar a forma canónica do Qt para resolver a função gráfica (ex: `QOpenGLContext::currentContext()->getProcAddress()`).
  2. **Tratamento de Erros:** Se o Qt retornar `nullptr` (comum em funções de legado ou conflitos de perfil no Windows), o sistema aciona o *fallback*.
  3. O *fallback* invoca a `opengl32.dll` nativa do Windows e usa `GetProcAddress` para resgatar a função.
* **Dependências:** Sistema Operativo Windows, biblioteca dinâmica `opengl32.dll`.
* **Pendências:** Se o Locus3D for portado para Linux ou macOS, este *fallback* terá de ser isolado através de diretivas de pré-compilação (`#ifdef _WIN32`).

---

## 3. Integração de Observabilidade (Logs do Terminal VS Code)
* **Finalidade:** Unificar os fluxos de saída (*stdout* e *stderr*) do motor C++ e os *logs* internos do Qt, apresentando-os diretamente no terminal do VS Code durante o desenvolvimento em Windows.
* **Fluxo Esperado:**
  1. Aplicações GUI em Windows (compiladas com subsistema *windows* em vez de *console*) descartam os *logs* impressos com `std::cout` ou `printf`.
  2. O ficheiro `main_qt.cpp` invoca a API do Windows `AttachConsole(ATTACH_PARENT_PROCESS)` logo no início da execução.
  3. Os *streams* padrão (`stdout`, `stderr`) são reencaminhados para este terminal.
* **Sincronização:** O Qt possui o seu próprio gestor de mensagens (`qDebug()`, `qWarning()`). Para que estes erros apareçam no mesmo terminal, foi utilizado o `qInstallMessageHandler` para injetar os avisos do Qt no nosso fluxo padrão de *logs*.

---

## 4. Integração do Sistema de Ficheiros (Resolvedor de Shaders)
* **Finalidade:** Carregar, em tempo de execução, os ficheiros de código GLSL da placa gráfica a partir do disco (`source/assets/shaders/`).
* **Fluxo Esperado:**
  1. O método `EditorViewport::initialize` tenta resolver o caminho raiz.
  2. Invoca `shader_root_has_viewport_assets`, que procura por presenças estritas de ficheiros (ex: `viewport/grid_vert.glsl` e `viewport/point_marker_vert.glsl`).
* **Tratamento de Erros:** O carregamento escreve o seu progresso no ficheiro físico `locus3d_startup.log`. Se falhar no carregamento de um renderizador (ex: *Screen Space Line Renderer* por falha DSA), o sistema gera um *Access Violation* em vez de um erro amigável.
* **Decisões:** Os nomes dos ficheiros e as suas extensões (`.glsl`) estão fixos no código fonte (*hardcoded* na compilação do C++). Alterações aos nomes na pasta implicarão obrigatoriamente a atualização do C++.

---

## 5. Integração CMake <-> Qt Meta-Object System
* **Finalidade:** Compilar a interface e os *widgets* C++ do Qt sem necessitar de scripts manuais.
* **Fluxo Esperado:**
  * O CMake gere a integração utilizando as diretivas:
    * `set(CMAKE_AUTOMOC ON)` - Processa automaticamente a macro `Q_OBJECT`.
    * `set(CMAKE_AUTOUIC ON)` - Converte os eventuais ficheiros `.ui` desenhados no Qt Designer (se aplicável no futuro).
    * `set(CMAKE_AUTORCC ON)` - Compila os recursos (ícones, *stylesheets* QSS) no executável.