# Locus3D - Estado Atual e Plano de Ação (Backlog)

Este documento mapeia o estado exato da base de código no momento do *handoff* e define as tarefas imediatas e futuras para o desenvolvimento da interface gráfica em Qt6. O agente (Codex) deve utilizar este ficheiro como o roteiro principal para as suas implementações.

---

## 1. CONCLUÍDO (Infraestrutura de Base e Sobrevivência Gráfica)
* **Integração do Motor com Qt6:** A Viewport nativa (`EditorViewport`) foi acoplada com sucesso num `QOpenGLWidget` (através da classe `Locus3DViewport`). O ciclo de renderização está ativo e apresenta a cena corretamente (cubos de teste e grelha visíveis).
* **Resolução do Perfil OpenGL:** A superfície global do Qt (`QSurfaceFormat`) está estritamente bloqueada no **OpenGL 4.5 Core Profile**, permitindo a compilação de *shaders* modernos e funções de núcleo exigidas pelo backend.
* **Estabilidade do Carregador GLAD:** Implementado um *fallback* robusto para instanciar as funções do OpenGL utilizando chamadas nativas do Windows (`LoadLibraryA("opengl32.dll")` e `GetProcAddress`), prevenindo *crashes* silenciosos caso o contexto Qt falhe na devolução do ponteiro de função.
* **Correção Crítica de DSA (*Direct State Access*):** O ficheiro `VertexArray.cpp` foi refatorado. A rotina `glCreateVertexArrays` valida agora o ponteiro; em caso nulo, faz a regressão segura para `glGenVertexArrays`, impedindo a falha de segmentação (*Access Violation*) que encerrava o programa na compilação do `ScreenSpaceLineRenderer`.
* **Redirecionamento de Logs (Console API):** A aplicação anexa agora o terminal pai e reencaminha `stdout`/`stderr`, em conjunto com a captura de `qInstallMessageHandler` para os avisos do ecossistema Qt.
* **Câmara Base:** As interações primárias da câmara (Órbita com o botão direito do rato e Zoom com a roda de rolagem) estão funcionais no `Locus3DViewport`.
* **Sincronização de Repositório:** O ambiente foi fundido de volta e alinhado com a branch atualizada `locus3d-v2`, garantindo que o backend em uso é a versão mais recente.

---

## 2. EM DESENVOLVIMENTO (Fase Atual)
* **Arquitetura da Nova GUI (Overlay):** Criação da estrutura de pastas em `source/overlay/` para isolar todo o código da nova interface Qt.
* **Estilização (Dark Theme):** O motor de injeção de *stylesheets* (QSS) para dar ao software a aparência profissional de aplicações como Blender e Maya está a ser preparado.

---

## 3. PRÓXIMAS ETAPAS (Ação Imediata para o Agente)
Estas são as tarefas que devem ser programadas e integradas **imediatamente** nesta nova fase de desenvolvimento, respeitando estritamente a arquitetura de ficheiros `.h`/`.cpp` separados.

* **[TAREFA 1] Criação do Layout da Janela Principal (MainWindow):**
  * Configurar o `QMainWindow` para instanciar a `Locus3DViewport` como *Central Widget*.
  * Criar as áreas de encaixe (*Dock Areas*) à direita para o Inspetor e no topo (ou esquerda) para a Barra de Ferramentas.
* **[TAREFA 2] Implementação do Outliner Widget (`source/overlay/OutlinerWidget`):**
  * Criar um componente contendo um `QTreeWidget`.
  * Ler o estado inicial da cena (`Editor->scene`) e popular a árvore com os nomes/IDs dos nós (ex: "Cube A", "Cube B").
  * **Integração:** Ligar o evento de *clique* no item da árvore ao `SelectionController` do backend, forçando a seleção do objeto na Viewport 3D.
* **[TAREFA 3] Implementação do Transform Widget (`source/overlay/TransformWidget`):**
  * Criar um painel (Inspetor) focado na modificação das propriedades espaciais.
  * Inserir componentes `QDoubleSpinBox` avançados agrupados por: **Posição (X,Y,Z)**, **Rotação (X,Y,Z)** e **Escala (X,Y,Z)**.
  * **Integração:** Ler a `NodeTransform` do objeto selecionado e exibir os valores reais. Quando o utilizador alterar o valor no `QDoubleSpinBox`, propagar a alteração para a `NodeTransform` no backend e invocar as *flags* de atualização (`EditorDirtyFlags`) para forçar o *repaint*.
* **[TAREFA 4] Implementação da Toolbar Widget (`source/overlay/ToolbarWidget`):**
  * Construir uma barra com botões de ação ("1 a 15", "S", "R").
  * **Integração:** Estes botões devem comunicar diretamente com o `ToolManager` e os modos do Gizmo (Modo Vértice, Aresta, Face, Translação, Rotação e Escala) já existentes no backend.
* **[TAREFA 5] Deteção de Seleção pela Viewport (Picking to UI Sync):**
  * Quando o utilizador clicar numa malha na Viewport 3D (via *Raycasting/Picking* nativo já funcional no backend), o Qt deve detetar essa mudança de estado e selecionar automaticamente o nó correto na árvore do *Outliner* e atualizar os valores nas caixas de Transformação.

---

## 4. BACKLOG
* **Mapeamento de Atalhos de Teclado (Shortcuts):**
  * Integrar o atalho `Delete` na interface global do Qt (ou na Viewport) para invocar o comando de destruição do nó selecionado no backend.
  * Integrar o atalho `Ctrl+Z` para invocar o `DocumentSession::history()->undo()`.
* **Diálogos de Sistema (Save/Load):**
  * Criar interfaces nativas Qt (`QFileDialog`) para Abrir, Guardar e Exportar cenas (quando o formato de serialização de ficheiros estiver estabelecido no backend).
* **Painel de Operações Avançadas (Modifiers):**
  * Preparar o layout lateral para futuras ferramentas topológicas como Extrusão, Inserção (Inset), *Bevel* e Subdivisão.

---

## 5. IDEIAS FUTURAS
* **Editor Visual de Materiais:** Janela de nós baseada em grafos para criar sombreadores e aplicar texturas.
* **Timeline e Animação:** Painel inferior com *keyframes* e controlo de reprodução, caso o Locus3D suporte animação topológica futuramente.
* **Multi-Viewports:** Possibilidade de dividir o `Central Widget` em 4 ecrãs (Topo, Frente, Direita e Perspetiva) instanciando múltiplos contextos `Locus3DViewport` que partilham a mesma sessão de renderização.

---

## 6. PENDÊNCIAS DE DECISÃO
* **Padrão de Observação de Estado (Observer vs Signals/Slots):** Ainda está por decidir se o motor C++ deverá notificar a UI de que a cena foi alterada emitindo eventos genéricos, ou se a UI deve consultar o estado ativamente (polling/dirty checking) no fim de cada *frame*. A equipa de *frontend* (IA) deverá avaliar a melhor opção arquitetural e implementá-la em coerência com os padrões existentes.