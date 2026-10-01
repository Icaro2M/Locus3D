# Locus3D - Registo de Decisões de Arquitetura (ADR)

Este documento centraliza as decisões técnicas arquiteturais tomadas durante a fase de transição do motor Locus3D. O objetivo principal deste registo é fornecer contexto histórico, garantindo que futuros engenheiros (ou agentes de IA) não revertam soluções críticas que mitigaram falhas profundas no passado.

---

**DECISÃO:** Transição do framework de janelas de GLFW para Qt6.
* **CONTEXTO:** O motor 3D estava a ser executado numa janela GLFW básica que servia apenas como um "canvas" de testes. O objetivo do projeto mudou para a criação de um software de nível profissional (semelhante ao Blender/Maya) com Outliner, Inspetor de Propriedades e Barras de Ferramentas.
* **MOTIVO:** O GLFW é excelente para contextos puramente gráficos, mas é manifestamente insuficiente para construir interfaces de utilizador complexas e ricas em dados (GUI) de forma modular. O ecossistema Qt6 fornece os painéis de acoplamento (`QDockWidget`), árvores hierárquicas (`QTreeWidget`) e inputs numéricos (`QDoubleSpinBox`) nativos.
* **IMPACTO:** Necessidade de reescrever a inicialização do OpenGL, removendo o contexto GLFW e passando a instanciar a classe nativa do Qt `QOpenGLWidget`.
* **ALTERNATIVAS DISCUTIDAS:** Utilizar ImGui em cima do GLFW (Rejeitado por não fornecer um fluxo de trabalho *desktop* nativo de alta escalabilidade para dezenas de ferramentas simultâneas).
* **STATUS:** CONCLUÍDO e consolidado na branch `locus3d-v2`.

---

**DECISÃO:** Exigência forçada do OpenGL 4.5 Core Profile.
* **CONTEXTO:** O frontend em Qt tentou inicializar o contexto gráfico na versão 4.1. 
* **MOTIVO:** O backend original, desenvolvido por outro engenheiro, utiliza chamadas modernas de Direct State Access (DSA), nomeadamente `glCreateVertexArrays`. Estas funções só foram padronizadas no OpenGL 4.5. Quando compilado e executado sob o perfil 4.1, o controlador da GPU retornava ponteiros nulos (`nullptr`) para estas funções, resultando em falhas de segmentação (*Access Violation*) logo no início da renderização do *Screen Space Line Renderer*.
* **IMPACTO:** O ficheiro `main_qt.cpp` define explicitamente `format.setVersion(4, 5)` e `QSurfaceFormat::CoreProfile` antes do arranque da aplicação Qt.
* **ALTERNATIVAS DISCUTIDAS:** Nenhuma viável. Fazer o *downgrade* do backend do motor implicaria reescrever todo o pipeline gráfico e perder a otimização de CPU ganha com o DSA.
* **STATUS:** CONCLUÍDO.

---

**DECISÃO:** Implementação de Fallback Dinâmico no Carregador GLAD.
* **CONTEXTO:** Mesmo solicitando um contexto Qt 4.5, funções de núcleo antigas ou chamadas intermédias apresentavam ponteiros nulos devido à forma como o driver do Windows empacota extensões no contexto isolado do Qt.
* **MOTIVO:** Prevenir *crashes* silenciosos do motor em ambientes Windows heterogéneos.
* **IMPACTO:** O processo de carregamento de funções GLAD na inicialização verifica primariamente através da rotina nativa do Qt (`QOpenGLContext::getProcAddress`). Se falhar, executa um *fallback* carregando a biblioteca estática de sistema `LoadLibraryA("opengl32.dll")` e mapeando via `GetProcAddress`.
* **ALTERNATIVAS DISCUTIDAS:** Usar puramente `QOpenGLFunctions` do Qt (Rejeitado para evitar sobreposição/conflito com o cabeçalho `<glad/glad.h>` utilizado extensivamente por todo o backend).
* **STATUS:** CONCLUÍDO.

---

**DECISÃO:** Prevenção de Null Pointers no encapsulamento de `VertexArray.cpp`.
* **CONTEXTO:** O log de inicialização (em `locus3d_startup.log`) revelou um travamento exato no momento de carregar o *renderizador de linhas de topologia*. A causa raiz foi identificada no método `VertexArray::create()`.
* **MOTIVO:** Em certos drivers, mesmo num contexto 4.5, o ponteiro de função `glCreateVertexArrays` pode falhar no *bind*. Uma chamada nula encerra o programa abruptamente.
* **IMPACTO:** O código foi modificado para suportar o caminho primário do DSA (`glCreateVertexArrays`), mas detém agora uma verificação de ponteiro estrita. Se `nullptr`, faz o *fallback* para o caminho clássico do OpenGL 3.3/4.1 (`glGenVertexArrays` seguido de um bind explícito).
* **ALTERNATIVAS DISCUTIDAS:** Confiar inteiramente no carregador (Rejeitado por provar ser instável no nosso ambiente).
* **STATUS:** CONCLUÍDO.

---

**DECISÃO:** Anexação do Terminal e Redirecionamento Global de Logs no Windows.
* **CONTEXTO:** Ao mudar o tipo de aplicação no CMake de Consola para Janela (GUI Qt), todos os `cout` e logs de *startup* desapareceram, prejudicando o diagnóstico de erros.
* **MOTIVO:** O Windows dissocia os processos de subsistema GUI de terminais visíveis por defeito.
* **IMPACTO:** O `main_qt.cpp` incorpora a chamada `#include <windows.h>` e executa `AttachConsole(ATTACH_PARENT_PROCESS)` e `freopen` no início da rotina. Adicionalmente, foi instalado um interceptador Qt (`qInstallMessageHandler`) para garantir que mesmo os avisos e erros nativos do ecossistema Qt são reencaminhados para o console no VS Code.
* **ALTERNATIVAS DISCUTIDAS:** Ler apenas os logs no ficheiro `locus3d_startup.log` (Rejeitado, pois não captura os *crashes* de segmento em tempo real na stdout).
* **STATUS:** CONCLUÍDO.

---

**DECISÃO:** Estrutura Modular Proibindo Ficheiros Monolíticos.
* **CONTEXTO:** A transição para a nova interface Qt6 levanta o risco comum de se alojar toda a UI num único ficheiro enorme `MainWindow.cpp`, misturando os painéis X/Y/Z, barras de ferramentas e visualizadores.
* **MOTIVO:** A escalabilidade do código e a redução de complexidade cíclica. O motor do Locus3D foi desenvolvido seguindo um padrão rígido e estrito em C++.
* **IMPACTO:** O código da nova GUI foi mandado obrigatoriamente para a pasta `/source/overlay/`. Qualquer componente visual deve ser isolado no seu próprio par de ficheiros `.h` (declaração) e `.cpp` (implementação). Um *Inspetor* de objetos não deve ser programado na *MainWindow*, mas sim instanciado nela como um widget à parte.
* **ALTERNATIVAS DISCUTIDAS:** Implementar tudo rapidamente para testar (Rejeitado; o débito técnico tornar-se-ia incontrolável, violando a regra arquitetural primária).
* **STATUS:** ATIVO (Aplica-se estritamente ao desenvolvimento futuro).