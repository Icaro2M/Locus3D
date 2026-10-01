# Locus3D - Requirements Specification

## 1. Requisitos Funcionais (Funcionalidades Principais)

### 1.1. Janela Principal (MainWindow)
* O sistema deve exibir uma janela principal gerenciada pelo Qt6 (`QMainWindow`), orquestrando o layout central e os painéis encaixáveis (Dock Widgets).
* A janela deve conter uma Viewport 3D (Central Widget), um Painel Inspetor (à direita) e uma Barra de Ferramentas (no topo).
* A janela deve suportar e aplicar um tema escuro (Dark Theme) profissional, utilizando QSS (Qt Style Sheets) ou `QPalette`, assemelhando-se a interfaces de softwares como Blender e Maya.

### 1.2. Viewport 3D (Renderização e Câmera)
* O sistema deve encapsular o renderizador nativo (`EditorViewport`) dentro de um `QOpenGLWidget` (classe `Locus3DViewport`).
* A viewport deve permitir a navegação espacial nativa:
  * **Órbita (Orbit):** Ativada com o Botão Direito do Mouse ou `Alt + Botão Esquerdo`.
  * **Panoramização (Pan):** Ativada com o Botão do Meio do Mouse.
  * **Zoom:** Ativado pela roda de rolagem (Scroll / `wheelEvent`).
* A viewport deve renderizar ferramentas de auxílio visual: Grid no chão e eixos (Gizmo).
* A viewport deve reagir a cliques do mouse acionando o *Raycasting*/*Picking* do backend, permitindo a seleção visual de malhas e manipulação do Gizmo 3D (arrastar eixos para mover, rotacionar ou escalar).

### 1.3. Painel Inspetor (Inspector)
* **Outliner (Árvore de Cena):** 
  * Deve conter um `QTreeWidget` listando os nós instanciados na cena (ex: "Cube A", "Cube B").
  * O clique em um item da árvore deve acionar o `SelectionController` do backend, selecionando o objeto na Viewport.
  * A seleção de um objeto na Viewport deve selecionar automaticamente o item correspondente no `QTreeWidget`.
* **Painel de Transformação:**
  * Deve exibir *inputs* numéricos avançados (`QDoubleSpinBox`) correspondentes aos eixos X, Y e Z.
  * O painel deve ser segmentado em seções de **Posição**, **Rotação** e **Escala**.
  * Os valores exibidos devem refletir o estado exato da transformação (`NodeTransform`) do objeto selecionado.
  * Edições manuais nos campos numéricos devem atualizar o backend de imediato e marcar as flags de atualização do editor (ex: `EditorDirtyFlags::Scene`, `EditorDirtyFlags::Mesh`, etc.).

### 1.4. Barra de Ferramentas (Toolbar)
* O sistema deve apresentar botões de ação rápidos (identificados na documentação original como botões "1 a 15, S, R").
* Os botões devem injetar ações no `ToolManager` e definir o `GizmoMode` do backend (Modo Objeto, Modo Vértice, Modo Aresta, Modo Face, Ferramenta Mover, Ferramenta Rotacionar, Ferramenta Escala, Ferramenta Extrusão).

### 1.5. Ações e Atalhos de Teclado
* O sistema deve responder a teclas de atalho essenciais da modelagem 3D:
  * `Delete`: Excluir o nó/malha selecionado.
  * `Ctrl+Z`: Desfazer ação, chamando a pilha de histórico (History) do backend.
  * Teclas rápidas para transformação (translação, rotação, escala) a serem definidas ou mapeadas para o `ToolManager`.

---

## 2. Requisitos Não Funcionais (Técnicos e Arquiteturais)

### 2.1. Arquitetura de Interface e Modularidade
* **Anti-Monolito:** A implementação da interface é estritamente proibida de utilizar arquivos monolíticos.
* O código da interface Qt deve ser segregado em módulos (ex: `InspectorWidget`, `OutlinerWidget`, `TransformWidget`, `ToolbarWidget`).
* **Estrutura de Arquivos:** Todos os componentes devem obrigatoriamente adotar o padrão de separação C++ (arquivos `.h` para declaração e `.cpp` para implementação).
* **Diretório Alvo:** Todo o código inerente à sobreposição da GUI Qt deve residir em `source/overlay/`.

### 2.2. Integração Gráfica e OpenGL
* **Contexto Gráfico:** A aplicação deve forçar a requisição do **OpenGL 4.5 Core Profile**.
* **Direct State Access (DSA):** O motor backend exige funções do padrão DSA (ex: `glCreateVertexArrays`). O perfil da superfície Qt (`QSurfaceFormat`) deve estar adequadamente alinhado a essa exigência `format.setVersion(4, 5)`.
* **Segurança do Carregador (Loader):** O `GLAD` deve ser instanciado através de um *fallback customizado* que busque funções primeiro no contexto Qt (`QOpenGLContext::getProcAddress`) e, em caso de falha silenciosa no Windows, busque diretamente na biblioteca estática (`LoadLibraryA("opengl32.dll")`).

### 2.3. Depuração e Observabilidade
* O sistema (quando compilado e rodando no Windows) deve acoplar-se ativamente ao console de origem através de `AttachConsole(ATTACH_PARENT_PROCESS)` e redirecionar `stdout` / `stderr`.
* Mensagens de erro silenciosas emitidas pelos componentes Qt devem ser interceptadas via `qInstallMessageHandler` e exibidas no log da aplicação, de forma que anomalias de OpenGL ou falhas de plugins não ocorram silenciosamente.

---

## 3. Restrições (Constraints)
* A equipe de frontend *não tem permissão* para alterar, duplicar ou modificar as regras lógicas e topológicas do *backend*. O frontend (Qt) opera apenas como um cliente (consumer) das classes no namespace `locus::application` e `locus::editor`.
* O uso do `QOpenGLFunctions` nativo do Qt deve ser evitado se causar sobreposição de macros/conflitos com o cabeçalho `<glad/glad.h>`. O gerenciamento de ponteiros de funções OpenGL deve ficar inteiramente sob controle do GLAD e dos renderizadores do próprio Locus3D.

---

## 4. Funcionalidades Futuras (Backlog / Visão a Longo Prazo)
* Integração das ferramentas de modelagem poligonal avançada (seleção por loop, extrusão, subdivisão, booleanos).
* Suporte a múltiplos painéis simultâneos e Layouts docáveis que podem ser desacoplados para uso em múltiplos monitores.
* Inclusão de um editor visual de materiais/texturização.
* Editor de animação de chaves (*Keyframe / Timeline*).

---

## 5. Atores (Usuários) e Permissões
O projeto atual é um aplicativo Desktop Standalone executado localmente na máquina do usuário.
* **Perfil do Usuário:** O sistema é desenhado para *power users* (artistas técnicos e programadores gráficos). O fluxo (UX) não foca em simplificação excessiva (drag-and-drop casual), mas sim na precisão numérica, agilidade com atalhos de teclado e acesso total aos dados topológicos da cena.
* **Permissões:** Não há sistema de autenticação, roles de usuário, multitenancy ou administração centralizada nesta etapa, visto que o software opera de maneira isolada no S.O.