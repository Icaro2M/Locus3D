# Locus3D - Project Context & Strategic Overview

## 1. Origem do Projeto e Contexto Estratégico
O **Locus3D** nasceu como um motor gráfico e modelador geométrico 3D focado em performance e controle arquitetural estrito. Até o momento atual, o projeto possui um *backend* altamente capaz, desenvolvido e mantido paralelamente por outro engenheiro da equipe na branch `locus3d-v2`. Este backend já gerencia estruturas complexas como grafos de cena (*Scene Graph*), nós de malha (*Mesh Nodes*), seleção geométrica, operações topológicas e renderização via OpenGL.

O ponto de virada (e o motivo da atual fase do projeto) é a transição da interface de usuário. Inicialmente, o Locus3D utilizava uma janela GLFW monolítica e básica, que servia apenas como um *canvas* de renderização para testes do motor. O objetivo estratégico agora é transformar esse motor em um software de nível profissional, substituindo o GLFW por uma interface rica, estruturada e modular utilizando **Qt6**.

## 2. O Problema que Resolve
Motores 3D caseiros ou acadêmicos frequentemente falham em escalar porque misturam a lógica de renderização com a lógica de interface em arquivos monolíticos intransponíveis. O Locus3D resolve esse problema ao impor uma barreira arquitetural rígida: o motor (backend) não sabe que a interface (frontend) existe. 
A nova interface em Qt6 visa fornecer ao usuário final ferramentas de controle de alta fidelidade (Outliner, Inspetor de Propriedades, Gizmos visuais e Barras de Ferramentas) sem comprometer a performance ou poluir a lógica de manipulação geométrica do núcleo.

## 3. Funcionamento Esperado
O software deve se comportar como uma aplicação *Desktop* clássica de criação de conteúdo digital (DCC - *Digital Content Creation*), seguindo os paradigmas visuais e operacionais de softwares consolidados como Blender e Maya. 
O usuário abrirá o aplicativo e encontrará:
* Um tema escuro (*Dark Theme*) nativo e imersivo.
* Uma Viewport 3D central (acelerada por hardware na GPU, renderizando a 60+ FPS) onde a cena é manipulada.
* Painéis laterais contendo a hierarquia da cena (Outliner) e coordenadas detalhadas do objeto selecionado.
* Barras de ferramentas superiores para alternar modos de interação (vértice, aresta, face, translação, rotação, escala).

## 4. Diferenciais do Produto
* **Backend de Alta Performance:** Utiliza OpenGL 4.5 Core Profile com suporte a DSA (*Direct State Access*), reduzindo o overhead de *binding* no driver da GPU.
* **Carregador Resiliente:** Um sistema de carregamento de extensões OpenGL (GLAD) customizado que possui fallback direto para a `opengl32.dll` do Windows, prevenindo *crashes* silenciosos em ambientes com drivers gráficos problemáticos.
* **Modularidade Extrema:** A interface Qt (Overlay) é tratada como um módulo puramente injetável (`source/overlay/`), garantindo que o núcleo do Locus3D possa ser compilado *headless* ou portado para outras plataformas no futuro.

## 5. Público-Alvo e Mercado
* **Público-Alvo:** Artistas 3D técnicos, engenheiros, desenvolvedores de jogos e estudantes de computação gráfica que necessitam de um modelador geométrico rápido e customizável.
* **Tipos de Empresas Atendidas:** Estúdios de desenvolvimento de jogos *indie*, escritórios de engenharia/arquitetura (para visualização de polígonos) e instituições de pesquisa.
* **Modelo Comercial:** Atualmente concebido como uma aplicação Desktop *standalone* (não é um SaaS cloud-based).

## 6. Funcionalidades Discutidas e Contexto de Engenharia
Nesta etapa de transição, discutimos e estabelecemos as fundações críticas para a nova GUI:
* **Integração `QOpenGLWidget`:** O coração da interface, responsável por abrigar a renderização do `EditorViewport` original sem quebrar o ciclo de vida do motor.
* **Depuração de Contexto Gráfico (Histórico Crítico):** O projeto sofreu falhas de segmentação (*access violations*) severas durante a transição inicial. Foi diagnosticado que o OpenGL estava em um perfil incompatível (4.1) com as chamadas DSA (`glCreateVertexArrays`) exigidas pelo backend. A versão foi elevada e travada em **4.5 Core Profile**.
* **Redirecionamento de Logs (Windows):** Como aplicações GUI no Windows perdem acesso ao terminal padrão, foi implementado um attach de console no `main_qt.cpp` combinado com `qInstallMessageHandler` para garantir visibilidade total dos logs do motor e do Qt no terminal do VS Code.
* **Sincronização de Estado:** O clique na Viewport deve refletir na UI (ex: atualizar a caixa XYZ) e vice-versa, exigindo um mapeamento preciso de *signals/slots* com o `SelectionController` e o `ToolManager` já existentes no backend.

## 7. Visão de Longo Prazo
O Locus3D planeja se consolidar como uma ferramenta completa de modelagem topológica. O sucesso desta fase (Handoff para a inteligência artificial assumir a interface Qt) ditará a velocidade com que novas funcionalidades complexas — como modificadores booleanos, texturização avançada e animação — poderão ser conectadas à interface sem necessidade de refatorações estruturais. O código escrito a partir de agora deve suportar anos de expansão modular.