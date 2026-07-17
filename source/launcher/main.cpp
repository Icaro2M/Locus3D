/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"
#include "editor/actions/ActionExecutor.h"
#include "editor/actions/ActionRegistry.h"
#include "editor/actions/core/ActionContext.h"
#include "editor/actions/core/ActionDescriptor.h"
#include "editor/actions/core/ActionResult.h"
#include "editor/actions/core/IEditorAction.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/command/scene/RenameNodeCommand.h"
#include "editor/history/HistoryStack.h"
#include "editor/scene/SceneNode.h"

#include <iostream>
#include <memory>
#include <string>
#include <utility>

namespace {

    using namespace locus::editor;

    constexpr const char* ExecutableActionId =
        "test.action.executable";

    constexpr const char* UnavailableActionId =
        "test.action.unavailable";

    constexpr const char* CommandActionId =
        "test.action.rename";

    /**
     * @brief Prints one smoke-test assertion.
     *
     * @param condition Assertion result.
     * @param message Human-readable assertion description.
     */
    void print_result(
        bool condition,
        const std::string& message) {
        std::cout
            << (condition ? "[OK] " : "[FAIL] ")
            << message
            << '\n';
    }

    /**
     * @brief Converts an action result code to readable text.
     *
     * @param code Result code.
     * @return Static readable name.
     */
    const char* result_code_name(ActionResultCode code) {
        switch (code) {
        case ActionResultCode::Executed:
            return "Executed";

        case ActionResultCode::Unavailable:
            return "Unavailable";

        case ActionResultCode::Failed:
            return "Failed";
        }

        return "Unknown";
    }

    /**
     * @brief Prints an action result.
     *
     * @param label Result label.
     * @param result Result to print.
     */
    void print_action_result(
        const std::string& label,
        const ActionResult& result) {
        std::cout << label << '\n';
        std::cout
            << "  code: "
            << result_code_name(result.code)
            << '\n';
        std::cout
            << "  message: "
            << result.message
            << '\n';
        std::cout
            << "  succeeded: "
            << (result.succeeded() ? "true" : "false")
            << '\n';
        std::cout
            << "  unavailable: "
            << (result.is_unavailable() ? "true" : "false")
            << '\n';
        std::cout
            << "  failed: "
            << (result.failed() ? "true" : "false")
            << '\n';
    }

    /**
     * @brief Simple action with configurable availability and result.
     */
    class TestAction final : public IEditorAction {
    public:
        TestAction(
            ActionDescriptor descriptor,
            bool available,
            ActionResult result)
            : descriptor_(std::move(descriptor)),
            available_(available),
            result_(std::move(result)) {
        }

        [[nodiscard]] const ActionDescriptor&
            descriptor() const override {
            return descriptor_;
        }

        [[nodiscard]] bool can_execute(
            const ActionContext& context) const override {
            (void)context;
            return available_;
        }

        ActionResult execute(
            ActionContext& context) override {
            (void)context;
            ++executionCount_;
            return result_;
        }

        [[nodiscard]] std::size_t execution_count() const {
            return executionCount_;
        }

    private:
        ActionDescriptor descriptor_{};
        bool available_ = false;
        ActionResult result_{};
        std::size_t executionCount_ = 0u;
    };

    /**
     * @brief Action that renames one scene node through a command.
     */
    class RenameNodeTestAction final : public IEditorAction {
    public:
        RenameNodeTestAction(
            SceneNodeId nodeId,
            std::string newName)
            : nodeId_(nodeId),
            newName_(std::move(newName)) {
        }

        [[nodiscard]] const ActionDescriptor&
            descriptor() const override {
            static const ActionDescriptor descriptor{
                ActionId{ CommandActionId },
                "Rename Test Node",
                "Renames the smoke-test node through a command.",
                ActionCategory::Scene,
                {
                    "rename",
                    "node",
                    "test"
                }
            };

            return descriptor;
        }

        [[nodiscard]] bool can_execute(
            const ActionContext& context) const override {
            return nodeId_.is_valid()
                && context.scene().find_node(nodeId_) != nullptr
                && !newName_.empty();
        }

        ActionResult execute(
            ActionContext& context) override {
            if (!can_execute(context)) {
                return ActionResult::unavailable(
                    "The target node is not available.");
            }

            CommandResult result =
                context.execute_command(
                    std::make_unique<RenameNodeCommand>(
                        nodeId_,
                        newName_));

            return ActionResult::from_command(
                std::move(result));
        }

    private:
        SceneNodeId nodeId_{};
        std::string newName_{};
    };

    /**
     * @brief Creates a valid descriptor for one test action.
     *
     * @param id Stable textual identifier.
     * @param name Display name.
     * @param category Action category.
     * @return Complete action descriptor.
     */
    ActionDescriptor make_descriptor(
        std::string id,
        std::string name,
        ActionCategory category) {
        return ActionDescriptor{
            ActionId{ std::move(id) },
            std::move(name),
            "Smoke-test action.",
            category,
            {
                "smoke",
                "test"
            }
        };
    }

    bool test_registration() {
        std::cout << "\n=== ActionRegistry: registration ===\n";

        ActionRegistry registry{};

        print_result(
            registry.empty(),
            "registry inicia vazio");

        print_result(
            registry.size() == 0u,
            "registry inicia com tamanho zero");

        const bool nullRegistered =
            registry.register_action(nullptr);

        print_result(
            !nullRegistered,
            "registry rejeita action nula");

        auto invalidAction =
            std::make_unique<TestAction>(
                ActionDescriptor{},
                true,
                ActionResult::executed());

        const bool invalidRegistered =
            registry.register_action(
                std::move(invalidAction));

        print_result(
            !invalidRegistered,
            "registry rejeita descritor invalido");

        auto action =
            std::make_unique<TestAction>(
                make_descriptor(
                    ExecutableActionId,
                    "Executable Action",
                    ActionCategory::Utility),
                true,
                ActionResult::executed(
                    EditorDirtyFlags::Selection,
                    "Executable action completed."));

        TestAction* actionPointer = action.get();

        const bool registered =
            registry.register_action(std::move(action));

        print_result(
            registered,
            "registry aceita action valida");

        print_result(
            registry.size() == 1u,
            "registry atualiza tamanho");

        print_result(
            !registry.empty(),
            "registry deixa de estar vazio");

        print_result(
            registry.contains(
                ActionId{ ExecutableActionId }),
            "contains encontra action registrada");

        print_result(
            registry.find(
                ActionId{ ExecutableActionId })
            == actionPointer,
            "find retorna a instancia registrada");

        print_result(
            registry.find(
                ActionId{ "missing.action" })
            == nullptr,
            "find retorna null para action ausente");

        const ActionDescriptor* descriptor =
            registry.descriptor(
                ActionId{ ExecutableActionId });

        print_result(
            descriptor != nullptr,
            "descriptor encontra metadados registrados");

        print_result(
            descriptor
            && descriptor->name == "Executable Action",
            "descriptor preserva o nome da action");

        auto duplicate =
            std::make_unique<TestAction>(
                make_descriptor(
                    ExecutableActionId,
                    "Duplicate Action",
                    ActionCategory::Utility),
                true,
                ActionResult::executed());

        const bool duplicateRegistered =
            registry.register_action(
                std::move(duplicate));

        print_result(
            !duplicateRegistered,
            "registry rejeita identificador duplicado");

        print_result(
            registry.size() == 1u,
            "registro duplicado nao altera tamanho");

        return registry.size() == 1u
            && registry.contains(
                ActionId{ ExecutableActionId })
            && registry.find(
                ActionId{ ExecutableActionId })
            == actionPointer
            && !duplicateRegistered;
    }

    bool test_queries_and_replacement() {
        std::cout
            << "\n=== ActionRegistry: queries and replacement ===\n";

        ActionRegistry registry{};

        registry.register_action(
            std::make_unique<TestAction>(
                make_descriptor(
                    "mesh.action.first",
                    "First Mesh Action",
                    ActionCategory::Mesh),
                true,
                ActionResult::executed()));

        registry.register_action(
            std::make_unique<TestAction>(
                make_descriptor(
                    "mesh.action.second",
                    "Second Mesh Action",
                    ActionCategory::Mesh),
                true,
                ActionResult::executed()));

        registry.register_action(
            std::make_unique<TestAction>(
                make_descriptor(
                    "scene.action.first",
                    "Scene Action",
                    ActionCategory::Scene),
                true,
                ActionResult::executed()));

        const std::vector<ActionId> ids =
            registry.action_ids();

        print_result(
            ids.size() == 3u,
            "action_ids retorna todos os identificadores");

        const std::vector<const ActionDescriptor*>
            meshDescriptors =
            registry.descriptors_by_category(
                ActionCategory::Mesh);

        print_result(
            meshDescriptors.size() == 2u,
            "consulta por categoria encontra actions de mesh");

        const std::vector<const ActionDescriptor*>
            transformDescriptors =
            registry.descriptors_by_category(
                ActionCategory::Transform);

        print_result(
            transformDescriptors.empty(),
            "categoria sem actions retorna lista vazia");

        auto replacement =
            std::make_unique<TestAction>(
                make_descriptor(
                    "mesh.action.first",
                    "Replaced Mesh Action",
                    ActionCategory::Mesh),
                false,
                ActionResult::unavailable(
                    "Replacement is unavailable."));

        TestAction* replacementPointer =
            replacement.get();

        const bool replaced =
            registry.replace_action(
                std::move(replacement));

        print_result(
            replaced,
            "replace_action substitui action existente");

        print_result(
            registry.size() == 3u,
            "substituicao preserva tamanho");

        print_result(
            registry.find(
                ActionId{ "mesh.action.first" })
            == replacementPointer,
            "find retorna a nova instancia");

        const ActionDescriptor* replacedDescriptor =
            registry.descriptor(
                ActionId{ "mesh.action.first" });

        print_result(
            replacedDescriptor
            && replacedDescriptor->name
            == "Replaced Mesh Action",
            "substituicao atualiza descritor");

        const bool invalidReplacement =
            registry.replace_action(nullptr);

        print_result(
            !invalidReplacement,
            "replace_action rejeita action nula");

        const bool removed =
            registry.unregister_action(
                ActionId{ "scene.action.first" });

        print_result(
            removed,
            "unregister_action remove action existente");

        print_result(
            !registry.contains(
                ActionId{ "scene.action.first" }),
            "action removida deixa de ser encontrada");

        print_result(
            registry.size() == 2u,
            "remocao atualiza tamanho");

        const bool removedAgain =
            registry.unregister_action(
                ActionId{ "scene.action.first" });

        print_result(
            !removedAgain,
            "remocao de action ausente retorna false");

        const bool removedInvalid =
            registry.unregister_action(ActionId{});

        print_result(
            !removedInvalid,
            "remocao rejeita identificador invalido");

        registry.clear();

        print_result(
            registry.empty(),
            "clear remove todas as actions");

        return replaced
            && registry.empty()
            && !invalidReplacement
            && !removedAgain
            && !removedInvalid;
    }

    bool test_executor_basic() {
        std::cout
            << "\n=== ActionExecutor: basic execution ===\n";

        Editor editor{};
        editor.clear_dirty();

        CommandDispatcher dispatcher{ editor };
        HistoryStack history{};

        ActionContext context{
            editor,
            dispatcher,
            history
        };

        ActionRegistry registry{};

        auto executableAction =
            std::make_unique<TestAction>(
                make_descriptor(
                    ExecutableActionId,
                    "Executable Action",
                    ActionCategory::Utility),
                true,
                ActionResult::executed(
                    EditorDirtyFlags::Selection,
                    "Executable action completed."));

        TestAction* executablePointer =
            executableAction.get();

        registry.register_action(
            std::move(executableAction));

        auto unavailableAction =
            std::make_unique<TestAction>(
                make_descriptor(
                    UnavailableActionId,
                    "Unavailable Action",
                    ActionCategory::Utility),
                false,
                ActionResult::executed(
                    EditorDirtyFlags::Scene,
                    "This result must not be reached."));

        TestAction* unavailablePointer =
            unavailableAction.get();

        registry.register_action(
            std::move(unavailableAction));

        ActionExecutor executor{ registry };

        print_result(
            &executor.registry() == &registry,
            "executor preserva registry original");

        const ActionExecutor& constExecutor = executor;

        print_result(
            &constExecutor.registry() == &registry,
            "executor const retorna registry original");

        print_result(
            executor.can_execute(
                context,
                ActionId{ ExecutableActionId }),
            "can_execute aceita action disponivel");

        print_result(
            !executor.can_execute(
                context,
                ActionId{ UnavailableActionId }),
            "can_execute rejeita action indisponivel");

        print_result(
            !executor.can_execute(
                context,
                ActionId{ "missing.action" }),
            "can_execute rejeita action ausente");

        print_result(
            !executor.can_execute(
                context,
                ActionId{}),
            "can_execute rejeita identificador invalido");

        const ActionResult executed =
            executor.execute(
                context,
                ActionId{ ExecutableActionId });

        print_action_result(
            "executable action result",
            executed);

        print_result(
            executed.succeeded(),
            "executor executa action disponivel");

        print_result(
            executablePointer->execution_count() == 1u,
            "action disponivel foi executada uma vez");

        print_result(
            has_flag(
                editor.dirty_flags(),
                EditorDirtyFlags::Selection),
            "executor propaga dirty flags");

        editor.clear_dirty();

        const ActionResult unavailable =
            executor.execute(
                context,
                ActionId{ UnavailableActionId });

        print_action_result(
            "unavailable action result",
            unavailable);

        print_result(
            unavailable.is_unavailable(),
            "executor retorna indisponivel");

        print_result(
            unavailablePointer->execution_count() == 0u,
            "action indisponivel nao e executada");

        print_result(
            editor.dirty_flags()
            == EditorDirtyFlags::None,
            "action indisponivel nao marca dirty flags");

        const ActionResult missing =
            executor.execute(
                context,
                ActionId{ "missing.action" });

        print_action_result(
            "missing action result",
            missing);

        print_result(
            missing.failed(),
            "executor falha para action nao registrada");

        const ActionResult invalid =
            executor.execute(
                context,
                ActionId{});

        print_action_result(
            "invalid id result",
            invalid);

        print_result(
            invalid.failed(),
            "executor falha para identificador invalido");

        return executed.succeeded()
            && unavailable.is_unavailable()
            && missing.failed()
            && invalid.failed()
            && executablePointer->execution_count() == 1u
            && unavailablePointer->execution_count() == 0u;
    }

    bool test_command_action() {
        std::cout
            << "\n=== ActionExecutor: command and history ===\n";

        Editor editor{};
        editor.clear_dirty();

        CommandDispatcher dispatcher{ editor };
        HistoryStack history{};

        ActionContext context{
            editor,
            dispatcher,
            history
        };

        const SceneNodeId nodeId =
            editor.scene().create_empty("Original Name");

        SceneNode* node =
            editor.scene().find_node(nodeId);

        print_result(
            node != nullptr,
            "node de teste foi criado");

        if (!node) {
            return false;
        }

        ActionRegistry registry{};

        const bool registered =
            registry.register_action(
                std::make_unique<RenameNodeTestAction>(
                    nodeId,
                    "Renamed By Action"));

        print_result(
            registered,
            "action baseada em command foi registrada");

        ActionExecutor executor{ registry };

        print_result(
            executor.can_execute(
                context,
                ActionId{ CommandActionId }),
            "action baseada em command esta disponivel");

        const ActionResult result =
            executor.execute(
                context,
                ActionId{ CommandActionId });

        print_action_result(
            "rename action result",
            result);

        print_result(
            result.succeeded(),
            "action baseada em command foi executada");

        print_result(
            node->metadata().name
            == "Renamed By Action",
            "action alterou o nome pelo command");

        print_result(
            history.undo_size() == 1u,
            "command da action entrou no historico");

        print_result(
            history.can_undo(),
            "undo ficou disponivel");

        const CommandResult undoResult =
            history.undo(dispatcher);

        print_result(
            undoResult.success,
            "undo da action funcionou");

        print_result(
            node->metadata().name == "Original Name",
            "undo restaurou o nome original");

        print_result(
            history.can_redo(),
            "redo ficou disponivel");

        const CommandResult redoResult =
            history.redo(dispatcher);

        print_result(
            redoResult.success,
            "redo da action funcionou");

        print_result(
            node->metadata().name
            == "Renamed By Action",
            "redo reaplicou a action");

        return registered
            && result.succeeded()
            && undoResult.success
            && redoResult.success
            && node->metadata().name
            == "Renamed By Action"
            && history.undo_size() == 1u;
    }

} // namespace

int main() {
    std::cout
        << "=== Locus3D Editor Action Registry and "
        "Executor Smoke Test ===\n";

    bool passed = true;

    passed = test_registration() && passed;
    passed = test_queries_and_replacement() && passed;
    passed = test_executor_basic() && passed;
    passed = test_command_action() && passed;

    std::cout << '\n';

    if (passed) {
        std::cout
            << "=== All action registry and executor "
            "smoke tests passed ===\n";
        return 0;
    }

    std::cout
        << "=== Action registry and executor "
        "smoke test failed ===\n";
    return 1;
}