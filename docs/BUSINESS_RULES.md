# Locus3D - Regras de Negócio e Lógica de Domínio

No contexto do Locus3D (um software de modelação 3D / *Digital Content Creation*), as "regras de negócio" traduzem-se nas regras de domínio do motor gráfico, nas diretrizes de sincronização de estado entre a interface e o núcleo, e nas restrições de manipulação da topologia geométrica.

## 1. Princípio da Fonte Única de Verdade (Single Source of Truth)
* **Backend como Dono dos Dados:** O estado real do projeto, as malhas, os nós de cena e os valores de transformação residem estrita e exclusivamente no Backend (`locus::application::DocumentSession` e `locus::editor::Editor`).
* **UI como Consumidor Passivo:** A interface gráfica (Qt6) NUNCA deve armazenar cópias locais dos dados geométricos ou de transformação para evitar dessincronização. A UI atua apenas como um "espelho" do estado atual do motor e um "emissor" de comandos.

## 2. Regras de Sincronização UI <-> Backend
* **Mutações a partir do Inspetor:** Quando o utilizador altera um valor numérico nas caixas de rotação/escala/posição (`QDoubleSpinBox`) no Painel Inspetor, o frontend deve enviar o novo valor ao nó correspondente (via `NodeTransform`). Imediatamente após a alteração, a interface DEVE assinalar a cena como "suja" utilizando as flags do editor (ex: `EditorDirtyFlags::Scene | EditorDirtyFlags::Mesh | EditorDirtyFlags::Render`).
* **Seleção Bidirecional:** 
  * Se um objeto for clicado na Viewport 3D (via *Raycasting/Picking*), a árvore do *Outliner* no Inspetor deve focar e destacar automaticamente o item selecionado.
  * Se um item for clicado no *Outliner*, o controlador de seleção (`SelectionController`) do motor deve ser invocado com o respetivo `SceneNodeId` para destacar a malha na Viewport.
* **Comandos de Ferramentas:** A barra de ferramentas (modos de objeto, vértice, aresta, translação, escala) atua enviando instruções de alteração de modo de ferramenta diretamente ao `ToolManager`. O estado visual dos botões no frontend deve refletir o estado do `ToolManager`.

## 3. Regras e Restrições do Ciclo de Vida Gráfico (OpenGL & Viewport)
* **Prevenção de Dimensões Nulas:** Durante o ciclo de criação e dimensionamento (layout) inicial da janela Qt, as dimensões `width` e `height` da Viewport podem regressar com o valor 0. É uma regra estrita evitar a passagem de dimensões nulas para os buffers de seleção (*picking buffer*) ou projeção de câmara, para evitar exceções de divisão por zero. A largura e altura devem ser forçadas a um mínimo de 1 pixel (`if (height <= 0) height = 1;`).
* **Gestão de Contexto Gráfico:** Todas as invocações destrutivas na Viewport ou encerramento da sessão gráfica DEVEM ser antecedidas por um `makeCurrent()` e sucedidas por um `doneCurrent()` do `QOpenGLWidget`, garantindo que os recursos na memória VRAM da GPU sejam libertados na *thread* correta.
* **Direct State Access (DSA):** Sendo o projeto dependente de OpenGL 4.5 Core Profile, qualquer função que envolva a criação de *Vertex Arrays* ou *Buffers* deve utilizar, preferencialmente, o padrão DSA (`glCreateVertexArrays`).

## 4. Manipulação de Cena e Elementos (Tool e Gizmo)
* **Identificadores Únicos:** Cada objeto geométrico criado na cena recebe um `SceneNodeId` único atribuído pelo `Editor`. O frontend deve utilizar este ID como a "chave primária" para consultar e identificar elementos no *Outliner*.
* **Interação com Gizmo:** O Gizmo 3D possui estados próprios dependendo da ferramenta ativa (ex: Eixo X, Eixo Y, Eixo Z, Planos XY, XZ, YZ). A seleção de um eixo do Gizmo pela UI substitui o comportamento normal de seleção de malha e entra num estado de operação de transformação contínua.
* **Operações de Malha (Mesh Drag):** Ferramentas que arrastam vértices, arestas ou faces criam uma sessão de antevisão (*operation preview*). A renderização final da malha atualizada só é consolidada (*commit*) quando a operação (ex: soltar o clique do rato) é terminada, invocando o histórico do documento para suportar o comando `Ctrl+Z` (Desfazer).

## 5. Exceções e Tratamentos Especiais (Histórico de Correções)
* **Exceção de Fallback do GLAD:** No sistema operativo Windows, o carregador padrão de extensões OpenGL por vezes não consegue encontrar funções legado como `glGetString` usando apenas os métodos nativos do Qt. É uma **regra estrita** manter o carregador customizado no ficheiro `Locus3DViewport.cpp` que executa um *fallback* seguro recorrendo à invocação direta de `LoadLibraryA("opengl32.dll")` e `GetProcAddress`.
* **Exceção de DSA em VertexArray:** Caso o controlador de vídeo (*driver*) da GPU limite o perfil, as funções como `glCreateVertexArrays` (DSA) podem retornar ponteiros nulos. Foi estabelecida a regra no código (ver `VertexArray.cpp`) de verificar se o ponteiro é nulo e, caso afirmativo, executar um *fallback* seguro gerando o array via `glGenVertexArrays` seguido de *bind*. A ausência desta regra provocou falhas de segmentação (*Access Violations*) críticas no passado e não pode ser revogada.
* **Tratamento de Ficheiros de Shaders:** O resolvedor de caminhos (`shader_root_has_viewport_assets`) não deduz as extensões automaticamente. Se a extensão de um *shader* mudar (ex: de `.glsl` para `.vert`), as funções que procuram os *assets* da viewport têm de ser atualizadas ou a aplicação falhará em tempo de execução logo a seguir a `EditorViewport: line shader loaded`.