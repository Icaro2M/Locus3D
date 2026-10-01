FILE: AGENTS.md

# Locus3D - Agent Operational Guide

## 1. Objetivo do Projeto
O Locus3D é um modelador geométrico 3D. O objetivo imediato desta fase do projeto é realizar a integração completa de um backend maduro e já funcional (que gerencia grafos de cena, topologia, ferramentas e renderização) com uma nova **Interface Gráfica (GUI) avançada baseada em Qt6**, substituindo a antiga implementação monolítica baseada em GLFW. O foco atual é construir uma interface profissional (semelhante ao Blender ou Maya) e realizar a injeção (binding) das funções do motor gráfico no frontend.

## 2. Tecnologias Utilizadas
* **Linguagem:** C++ (Padrão moderno).
* **Framework GUI:** Qt6 (módulo Core, Gui, Widgets e OpenGLWidgets).
* **API Gráfica:** OpenGL 4.5 Core Profile (com retrocompatibilidade explícita tratada no código).
* **Extension Loader:** GLAD (configurado com fallbacks específicos para o SO).
* **Matemática:** GLM.
* **Build System:** CMake + MinGW.
* **Plataforma Principal Atual:** Windows (requer injeção de DLL e redirecionamento de logs via Console API).
* **Controle de Versão:** Git (Branch ativa: `locus3d-v2`).

## 3. Arquitetura Geral
O Locus3D adota uma arquitetura rigorosamente modularizada, separando a lógica de negócio/motor 3D da camada de apresentação:
* **Backend (Existente):** Gerencia `DocumentSession`, `Editor`, `ToolManager`, `Scene Graph` (nós e malhas), `SelectionController` e `TopologyBuilder`. O backend não tem conhecimento explícito de widgets de interface.
* **Frontend (Em Desenvolvimento):** A interface Qt6 age como um invólucro (Overlay). A viewport 3D é um `QOpenGLWidget` que recebe o contexto do `EditorViewport`.
* **Padrão de Arquivos:** Todo o código é rigidamente dividido em definição (`.h` / `#pragma once`) e implementação (`.cpp`). **É terminantemente proibido o uso de arquivos monolíticos.**

## 4. Princípios que Devem ser Respeitados
* **Modularidade Extrema:** A interface não pode ser um God Object. Deve ser fracionada (ex: `MainWindow`, `OutlinerWidget`, `TransformInspector`, `ToolbarWidget`).
* **Non-Destructive UI:** O código da interface deve consumir os dados do backend via ponteiros/referências ou signals/slots, nunca duplicando o estado de domínio (Single Source of Truth).
* **Performance Gráfica:** O laço de renderização e os eventos de mouse (picking, órbita) não devem ser bloqueados por atualizações síncronas da árvore de UI.

## 5. Regras Importantes de Implementação
1. **Diretório Alvo UI:** TODO novo código responsável pela interface gráfica Qt deve ser alocado exclusivamente dentro do diretório `source/overlay/` (crie subpastas contextuais como `inspector/`, `toolbar/`, etc.).
2. **Contexto OpenGL:** O perfil da superfície (`QSurfaceFormat`) deve ser estritamente inicializado como `(4, 5)` e `CoreProfile` antes de instanciar a aplicação Qt. Isso é inegociável para garantir o funcionamento do DSA (Direct State Access) no backend.
3. **Loader Seguro:** Sempre utilizar o wrapper do carregador GLAD com fallback para a `opengl32.dll` do Windows. Jamais confie cegamente no retorno de extensões sem tratar o ponteiro nulo.

## 6. Documentos que o Codex Deve Consultar
Antes de propor ou refatorar qualquer linha de código, o agente DEVE ler obrigatoriamente os seguintes documentos estruturais localizados na pasta `docs/`:
* `docs/PROJECT_CONTEXT.md`: Para entender o cenário e histórico atual da migração.
* `docs/REQUIREMENTS.md`: Para listar as features obrigatórias da nova interface (Outliner, Caixas de Transformação, Toolbar).
* `docs/ARCHITECTURE.md`: Para entender a separação Frontend/Backend.
* `docs/DECISIONS.md`: CRÍTICO. Para evitar que decisões resolvidas a duras penas (como os *crashes* de inicialização gráfica) sejam desfeitas acidentalmente.
* `docs/INTEGRATIONS.md`: Para entender as chamadas ao backend gráfico (Viewport, Gizmo, Picking).

## 7. Prioridades do Projeto (Estado Atual)
1. **Estruturar a GUI Completa:** Desenhar o layout em Qt (MainWindow, Inspector com QTreeWidget para Outliner e QDoubleSpinBox para Transforms X/Y/Z, e Toolbar superior).
2. **Integração (Binding):** Conectar os componentes visuais às funções prontas do backend (ex: clicar no Outliner -> chama `SelectionController`).
3. **Estilização Avançada:** Aplicar Dark Theme nativo via QSS/StyleSheet focado em UX profissional para modeladores 3D.
4. **Habilitar Ferramentas de Viewport:** Plugar os botões de ferramentas (1-15, Mover, Escala, Rotação) no `ToolManager` e `GizmoMode`.

## 8. Comportamentos que o Codex Deve Preservar
* **Aja como um Integrador:** A maioria das funções lógicas (criar malha, deletar, selecionar) JÁ EXISTE no backend. Não reimplemente a roda. Identifique o header correto no namespace `locus::editor` ou `locus::application` e faça a chamada.
* **Comunicação Ativa:** Quando a documentação e o código divergirem, assuma o código existente na branch `locus3d-v2` como a verdade primária, mas notifique o usuário da inconsistência.
* **Segurança de Ponteiros:** Ao lidar com o contexto gráfico do Qt misturado com o ciclo de vida de objetos do motor, garanta checagens estritas de `nullptr` antes de manipular instâncias de renderização.

## 9. Instrução de Boot do Agente
Ao iniciar qualquer tarefa neste repositório, execute internamente o seguinte fluxo:
1. Leia `AGENTS.md` (este arquivo).
2. Processe a pasta `docs/` inteira.
3. Compare mentalmente as orientações do `docs/TODO.md` com a tarefa solicitada pelo usuário.
4. Inspecione a arquitetura em `source/overlay/` e `source/gui/`.
5. Apresente um plano de ação confirmando as integrações backend que precisará consumir.