# Locus3D - Modelo de Dados e Entidades Principais

Este documento reconstrói o modelo de dados e as entidades de domínio identificadas no histórico do projeto Locus3D. Dado tratar-se de um motor gráfico em C++, as entidades são frequentemente representadas por classes, estruturas (`structs`) e identificadores opacos.

Onde a informação não foi explicitada no histórico, encontra-se a marcação **A DEFINIR**.

---

## 1. DocumentSession (Sessão de Documento)
* **Objetivo:** Representa a sessão ativa do projeto/documento aberto. Age como o agregador de alto nível para os sistemas de edição e histórico.
* **Campos / Composição:**
  * `editor` (Tipo: Referência para `Editor`) - Obrigatório.
  * `tool_manager` (Tipo: Referência para `ToolManager`) - Obrigatório.
  * `command_dispatcher` (Tipo: Referência para Dispatcher de Comandos) - Obrigatório.
  * `history` (Tipo: Referência para Gestor de Histórico / Undo-Redo) - Obrigatório.
  * `editor_sync` (Tipo: Referência para Sincronizador de Estado) - Obrigatório.
* **Identificadores:** A DEFINIR (Atualmente inferido como *singleton* de sessão ou por ponteiro).
* **Relacionamentos:** Detém ou faz a ponte para o `Editor`, `ToolManager` e sistema de comandos.
* **Regras de Validação:** A sessão do documento tem de estar válida durante todo o ciclo de vida da interface gráfica.
* **Dependências:** Injeta contexto de ferramenta (`ToolContext`).

---

## 2. Editor (Editor 3D)
* **Objetivo:** Armazena o estado central da cena, as seleções ativas e a lógica principal do domínio 3D.
* **Campos / Composição:**
  * `scene` (Tipo: Gestor de Grafo de Cena) - Obrigatório.
  * `selection` (Tipo: `SelectionState`) - Obrigatório.
* **Identificadores:** A DEFINIR.
* **Relacionamentos:** O motor gráfico lê o estado do `Editor` para saber o que renderizar.
* **Regras de Validação:** Nenhuma camada de interface (Frontend) pode alterar o estado do `Editor` de forma arbitrária; devem ser usados comandos (Commands) para tal.
* **Dependências:** Nenhuma direta do lado da UI.

---

## 3. SceneNodeId (Identificador de Nó)
* **Objetivo:** Atuar como a chave primária ("ID") para qualquer objeto instanciado na cena 3D.
* **Campos:** ID numérico ou *hash* opaco (Tipo exato: `struct SceneNodeId` ou `uint32_t`/`uint64_t`).
* **Identificadores:** É, por si só, o identificador.
* **Relacionamentos:** Usado pelo `SelectionController`, `SceneGraph` e ferramentas de arrasto para referenciar um alvo.
* **Regras de Validação:** Possui o método `.is_valid()` para assegurar que não aponta para um nó inexistente ou nulo.

---

## 4. SceneNode & MeshNode (Nó de Cena / Malha)
* **Objetivo:** Representar as entidades visíveis e transformáveis no mundo 3D. `MeshNode` herda e expande o `SceneNode` com dados topológicos.
* **Campos:**
  * `id` (Tipo: `SceneNodeId`) - Obrigatório.
  * `transform` (Tipo: `NodeTransform`) - Obrigatório.
  * (Para MeshNode) `topology/geometry` (Tipo: A DEFINIR) - Os dados geométricos da malha.
* **Identificadores:** `SceneNodeId`.
* **Relacionamentos:** Pertence à `scene` do `Editor`.
* **Regras de Validação:** Só pode ser modificado através de operações transacionais que afetem a topologia.

---

## 5. NodeTransform (Transformação Numérica)
* **Objetivo:** Guardar o estado espacial matemático do objeto no mundo 3D.
* **Campos:**
  * `Position` (Tipo: Vetor 3D - X, Y, Z) - Obrigatório.
  * `Rotation` (Tipo: Vetor 3D / Quaternião - X, Y, Z) - Obrigatório.
  * `Scale` (Tipo: Vetor 3D - X, Y, Z) - Obrigatório.
* **Identificadores:** Não possui ID próprio, pertence a um `SceneNode`.
* **Relacionamentos:** Ligado diretamente aos campos numéricos (`QDoubleSpinBox`) do painel de Transformação no Inspetor da UI.
* **Regras de Validação:** Valores inseridos pela UI devem ser sincronizados de volta para este componente, marcando a cena como `dirty` (suja/desatualizada).

---

## 6. GizmoHit, GizmoMode & GizmoAxis
* **Objetivo:** Representar o estado e a interação com o Gizmo (ferramenta visual 3D para mover/rodar/escalar).
* **Campos de GizmoMode (Enum):** `None`, `Translate`, `Rotate`, `Scale`, `Universal`.
* **Campos de GizmoAxis (Enum):** `None`, `X`, `Y`, `Z`, `XY`, `XZ`, `YZ`, `XYZ`, `View`.
* **Campos de GizmoHit (Struct):**
  * `mode` (Tipo: `GizmoMode`) - Opcional.
  * `axis` (Tipo: `GizmoAxis`) - Opcional.
* **Identificadores:** N/A (Tipos de valor efêmero).
* **Regras de Validação:** `GizmoHit` possui `.is_valid()`.

---

## 7. LineInstance (Instância de Renderização de Linhas)
* **Objetivo:** Representar os dados enviados à GPU para renderizar uma linha de sobreposição em espaço de ecrã (*Screen Space*).
* **Campos:**
  * `start[3]` (Tipo: Array de `float`) - Posição inicial (X, Y, Z). Obrigatório.
  * `widthPixels` (Tipo: `float`) - Espessura da linha. Obrigatório.
  * `end[3]` (Tipo: Array de `float`) - Posição final (X, Y, Z). Obrigatório.
  * `padding` (Tipo: `float`) - Alinhamento de memória (Regra de empacotamento GLSL). Obrigatório.
  * `color[4]` (Tipo: Array de `float` RGBA) - Obrigatório.
* **Identificadores:** N/A (Dados temporários para a GPU).
* **Regras de Validação:** O `widthPixels` é limitado superiormente pela configuração do renderizador (`config_.maxWidthPixels`).

---

## 8. MeshToolTarget & OperationPreview
* **Objetivo:** Armazenar os dados temporários de uma operação de malha a decorrer (como extrudir vértices enquanto o utilizador arrasta o rato).
* **Campos de MeshToolTarget:**
  * `nodeId` (Tipo: `SceneNodeId`) - Obrigatório.
* **Campos de OperationPreview:** A DEFINIR (Contém as malhas sólidas e de arame virtuais temporárias para renderização através de *uploaders* gráficos).
* **Relacionamentos:** Pertencem e são geridos pela ferramenta ativa (`MeshDragOperationTool`).