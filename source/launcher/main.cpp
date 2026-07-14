/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"
#include "editor/tools/core/ITool.h"
#include "editor/tools/core/ToolContext.h"
#include "editor/tools/core/ToolDescriptor.h"
#include "editor/tools/core/ToolEvent.h"
#include "editor/tools/core/ToolResult.h"
#include "editor/tools/core/ToolState.h"
#include "editor/tools/management/ToolManager.h"
#include "editor/tools/management/ToolRegistry.h"

#include <iostream>
#include <memory>
#include <string>
#include <utility>

namespace {

    using namespace locus::editor;

    void print_result(
        bool condition,
        const std::string& message) {

        std::cout
            << (condition ? "[OK] " : "[FAIL] ")
            << message
            << '\n';
    }

    const char* result_code_name(ToolResultCode code) {
        switch (code) {
        case ToolResultCode::Ignored:
            return "Ignored";

        case ToolResultCode::Consumed:
            return "Consumed";

        case ToolResultCode::Started:
            return "Started";

        case ToolResultCode::Updated:
            return "Updated";

        case ToolResultCode::Confirmed:
            return "Confirmed";

        case ToolResultCode::Cancelled:
            return "Cancelled";

        case ToolResultCode::Failed:
            return "Failed";
        }

        return "Unknown";
    }

    void print_tool_result(
        const std::string& label,
        const ToolResult& result) {

        std::cout << label << '\n';
        std::cout
            << "  code: "
            << result_code_name(result.code)
            << '\n';

        std::cout
            << "  consumed: "
            << (result.was_consumed() ? "true" : "false")
            << '\n';

        std::cout
            << "  failed: "
            << (result.failed() ? "true" : "false")
            << '\n';

        std::cout
            << "  message: "
            << result.message
            << '\n';
    }

    class DummyTool final : public ITool {
    public:
        explicit DummyTool(
            ToolDescriptor descriptor,
            bool activationAllowed = true,
            bool activationFails = false)
            : descriptor_(std::move(descriptor)),
            activationAllowed_(activationAllowed),
            activationFails_(activationFails) {
        }

        [[nodiscard]]
        const ToolDescriptor& descriptor() const override {
            return descriptor_;
        }

        [[nodiscard]]
        ToolState state() const override {
            return state_;
        }

        [[nodiscard]]
        bool can_activate(
            const ToolContext& context) const override {

            (void)context;
            return activationAllowed_;
        }

        ToolResult activate(
            ToolContext& context) override {

            (void)context;
            ++activationCount_;

            if (activationFails_) {
                state_ = ToolState::Inactive;

                return ToolResult::fail(
                    "Dummy activation failure.");
            }

            state_ = ToolState::Ready;

            return ToolResult::consumed(
                EditorDirtyFlags::None,
                "Dummy tool activated.");
        }

        ToolResult deactivate(
            ToolContext& context) override {

            (void)context;
            ++deactivationCount_;
            state_ = ToolState::Inactive;

            return ToolResult::consumed(
                EditorDirtyFlags::None,
                "Dummy tool deactivated.");
        }

        ToolResult handle_event(
            ToolContext& context,
            const ToolEvent& event) override {

            (void)context;
            ++eventCount_;

            if (event.type == ToolEventType::PointerPress &&
                event.uses_primary_button()) {

                state_ = ToolState::Interacting;

                return ToolResult::started(
                    EditorDirtyFlags::Selection,
                    "Dummy interaction started.");
            }

            if (event.type == ToolEventType::PointerMove &&
                state_ == ToolState::Interacting) {

                return ToolResult::updated(
                    EditorDirtyFlags::Render,
                    "Dummy interaction updated.");
            }

            return ToolResult::ignored();
        }

        ToolResult confirm(
            ToolContext& context) override {

            (void)context;
            ++confirmCount_;

            if (state_ != ToolState::Interacting) {
                return ToolResult::ignored();
            }

            state_ = ToolState::Ready;

            return ToolResult::confirmed(
                EditorDirtyFlags::Scene,
                "Dummy interaction confirmed.");
        }

        ToolResult cancel(
            ToolContext& context) override {

            (void)context;
            ++cancelCount_;

            if (state_ != ToolState::Interacting) {
                return ToolResult::ignored();
            }

            state_ = ToolState::Ready;

            return ToolResult::cancelled(
                EditorDirtyFlags::Render,
                "Dummy interaction cancelled.");
        }

        [[nodiscard]] int activation_count() const {
            return activationCount_;
        }

        [[nodiscard]] int deactivation_count() const {
            return deactivationCount_;
        }

        [[nodiscard]] int event_count() const {
            return eventCount_;
        }

        [[nodiscard]] int confirm_count() const {
            return confirmCount_;
        }

        [[nodiscard]] int cancel_count() const {
            return cancelCount_;
        }

    private:
        ToolDescriptor descriptor_{};
        ToolState state_ = ToolState::Inactive;

        bool activationAllowed_ = true;
        bool activationFails_ = false;

        int activationCount_ = 0;
        int deactivationCount_ = 0;
        int eventCount_ = 0;
        int confirmCount_ = 0;
        int cancelCount_ = 0;
    };

    ToolDescriptor make_select_descriptor() {
        return ToolDescriptor{
            ToolId{ "editor.select" },
            "Select",
            "Dummy selection tool.",
            ToolCategory::Selection,
            ToolCapabilities::ObjectMode |
                ToolCapabilities::MeshMode |
                ToolCapabilities::UsesPointer
        };
    }

    ToolDescriptor make_transform_descriptor() {
        return ToolDescriptor{
            ToolId{ "editor.transform" },
            "Transform",
            "Dummy transform tool.",
            ToolCategory::Transform,
            ToolCapabilities::ObjectMode |
                ToolCapabilities::UsesPointer |
                ToolCapabilities::UsesGizmo |
                ToolCapabilities::Modal
        };
    }

    ToolDescriptor make_blocked_descriptor() {
        return ToolDescriptor{
            ToolId{ "editor.blocked" },
            "Blocked",
            "Dummy tool that cannot activate.",
            ToolCategory::Utility,
            ToolCapabilities::None
        };
    }

    ToolDescriptor make_failing_descriptor() {
        return ToolDescriptor{
            ToolId{ "editor.failing" },
            "Failing",
            "Dummy tool whose activation fails.",
            ToolCategory::Utility,
            ToolCapabilities::None
        };
    }

    bool test_registry() {
        std::cout << "\n=== ToolRegistry ===\n";

        ToolRegistry registry{};

        const bool selectRegistered = registry.register_tool(
            make_select_descriptor(),
            [] {
                return std::make_unique<DummyTool>(
                    make_select_descriptor());
            });

        const bool duplicateRejected = !registry.register_tool(
            make_select_descriptor(),
            [] {
                return std::make_unique<DummyTool>(
                    make_select_descriptor());
            });

        const bool transformRegistered = registry.register_tool(
            make_transform_descriptor(),
            [] {
                return std::make_unique<DummyTool>(
                    make_transform_descriptor());
            });

        const bool invalidRejected = !registry.register_tool(
            ToolDescriptor{},
            [] {
                return std::make_unique<DummyTool>(
                    ToolDescriptor{});
            });

        const ToolDescriptor* selectDescriptor =
            registry.descriptor(ToolId{ "editor.select" });

        std::unique_ptr<ITool> select =
            registry.create(ToolId{ "editor.select" });

        const auto transformDescriptors =
            registry.descriptors_by_category(
                ToolCategory::Transform);

        print_result(
            selectRegistered,
            "registro da SelectTool foi aceito");

        print_result(
            duplicateRejected,
            "id duplicado foi rejeitado");

        print_result(
            transformRegistered,
            "registro da TransformTool foi aceito");

        print_result(
            invalidRejected,
            "descriptor invalido foi rejeitado");

        print_result(
            registry.size() == 2u,
            "registry informa dois registros");

        print_result(
            registry.contains(ToolId{ "editor.select" }),
            "registry encontra editor.select");

        print_result(
            !registry.contains(ToolId{ "editor.unknown" }),
            "registry rejeita id desconhecido");

        print_result(
            selectDescriptor != nullptr &&
            selectDescriptor->name == "Select",
            "descriptor registrado pode ser consultado");

        print_result(
            select != nullptr &&
            select->descriptor().id ==
            ToolId{ "editor.select" },
            "factory cria tool com id correto");

        print_result(
            transformDescriptors.size() == 1u &&
            transformDescriptors.front()->id ==
            ToolId{ "editor.transform" },
            "consulta por categoria retorna TransformTool");

        const bool mismatchRegistered = registry.register_tool(
            ToolDescriptor{
                ToolId{ "editor.mismatch" },
                "Mismatch",
                "Factory returns another identifier.",
                ToolCategory::Utility,
                ToolCapabilities::None
            },
            [] {
                return std::make_unique<DummyTool>(
                    make_select_descriptor());
            });

        std::unique_ptr<ITool> mismatch =
            registry.create(ToolId{ "editor.mismatch" });

        print_result(
            mismatchRegistered,
            "registro com factory adiada foi aceito");

        print_result(
            mismatch == nullptr,
            "factory com descriptor divergente foi rejeitada na criacao");

        const bool removed =
            registry.unregister_tool(
                ToolId{ "editor.transform" });

        print_result(
            removed &&
            !registry.contains(
                ToolId{ "editor.transform" }),
            "unregister remove registro existente");

        return
            selectRegistered &&
            duplicateRejected &&
            transformRegistered &&
            invalidRejected &&
            registry.size() == 2u &&
            selectDescriptor != nullptr &&
            select != nullptr &&
            transformDescriptors.size() == 1u &&
            mismatchRegistered &&
            mismatch == nullptr &&
            removed;
    }

    bool test_manager_activation_and_events() {
        std::cout << "\n=== ToolManager: ativacao e eventos ===\n";

        Editor editor{};
        editor.clear_dirty();

        ToolContext context{ editor };
        ToolRegistry registry{};

        registry.register_tool(
            make_select_descriptor(),
            [] {
                return std::make_unique<DummyTool>(
                    make_select_descriptor());
            });

        registry.register_tool(
            make_transform_descriptor(),
            [] {
                return std::make_unique<DummyTool>(
                    make_transform_descriptor());
            });

        ToolManager manager{ registry };

        const ToolResult activated =
            manager.activate_tool(
                context,
                ToolId{ "editor.select" });

        print_tool_result("activate select", activated);

        print_result(
            !activated.failed() &&
            manager.has_active_tool() &&
            manager.is_active(
                ToolId{ "editor.select" }),
            "SelectTool ficou ativa");

        DummyTool* active =
            dynamic_cast<DummyTool*>(
                manager.active_tool());

        print_result(
            active != nullptr &&
            active->state() == ToolState::Ready &&
            active->activation_count() == 1,
            "instancia ativa recebeu activate");

        ToolEvent press{};
        press.type = ToolEventType::PointerPress;
        press.button = ToolPointerButton::Primary;

        const ToolResult started =
            manager.handle_event(
                context,
                press);

        print_tool_result("primary press", started);

        print_result(
            started.code == ToolResultCode::Started &&
            active != nullptr &&
            active->state() ==
            ToolState::Interacting,
            "pointer press iniciou interacao");

        print_result(
            has_flag(
                editor.dirty_flags(),
                EditorDirtyFlags::Selection),
            "dirty flag de selecao foi aplicada pelo manager");

        editor.clear_dirty();

        ToolEvent move{};
        move.type = ToolEventType::PointerMove;
        move.pointer.viewportPosition =
            glm::vec2{ 100.0f, 50.0f };

        const ToolResult updated =
            manager.handle_event(
                context,
                move);

        print_tool_result("pointer move", updated);

        print_result(
            updated.code == ToolResultCode::Updated &&
            has_flag(
                editor.dirty_flags(),
                EditorDirtyFlags::Render),
            "pointer move atualizou interacao e marcou render");

        editor.clear_dirty();

        ToolEvent confirm{};
        confirm.type = ToolEventType::Confirm;

        const ToolResult confirmed =
            manager.handle_event(
                context,
                confirm);

        print_tool_result("confirm", confirmed);

        print_result(
            confirmed.code ==
            ToolResultCode::Confirmed &&
            active != nullptr &&
            active->state() == ToolState::Ready &&
            active->confirm_count() == 1,
            "evento Confirm foi roteado para confirm");

        print_result(
            has_flag(
                editor.dirty_flags(),
                EditorDirtyFlags::Scene),
            "confirmacao marcou scene dirty");

        return
            !activated.failed() &&
            started.code == ToolResultCode::Started &&
            updated.code == ToolResultCode::Updated &&
            confirmed.code == ToolResultCode::Confirmed &&
            manager.is_active(
                ToolId{ "editor.select" });
    }

    bool test_manager_cancel_on_focus_loss() {
        std::cout << "\n=== ToolManager: cancel em focus lost ===\n";

        Editor editor{};
        ToolContext context{ editor };
        ToolRegistry registry{};

        registry.register_tool(
            make_select_descriptor(),
            [] {
                return std::make_unique<DummyTool>(
                    make_select_descriptor());
            });

        ToolManager manager{ registry };

        manager.activate_tool(
            context,
            ToolId{ "editor.select" });

        ToolEvent press{};
        press.type = ToolEventType::PointerPress;
        press.button = ToolPointerButton::Primary;

        manager.handle_event(context, press);

        DummyTool* active =
            dynamic_cast<DummyTool*>(
                manager.active_tool());

        ToolEvent focusLost{};
        focusLost.type = ToolEventType::FocusLost;

        const ToolResult cancelled =
            manager.handle_event(
                context,
                focusLost);

        print_tool_result("focus lost", cancelled);

        const bool ok =
            cancelled.code ==
            ToolResultCode::Cancelled &&
            active != nullptr &&
            active->state() == ToolState::Ready &&
            active->cancel_count() == 1;

        print_result(
            ok,
            "focus lost cancelou interacao ativa");

        return ok;
    }

    bool test_manager_switch_and_restore() {
        std::cout << "\n=== ToolManager: troca e restauracao ===\n";

        Editor editor{};
        ToolContext context{ editor };
        ToolRegistry registry{};

        registry.register_tool(
            make_select_descriptor(),
            [] {
                return std::make_unique<DummyTool>(
                    make_select_descriptor());
            });

        registry.register_tool(
            make_transform_descriptor(),
            [] {
                return std::make_unique<DummyTool>(
                    make_transform_descriptor());
            });

        registry.register_tool(
            make_blocked_descriptor(),
            [] {
                return std::make_unique<DummyTool>(
                    make_blocked_descriptor(),
                    false,
                    false);
            });

        registry.register_tool(
            make_failing_descriptor(),
            [] {
                return std::make_unique<DummyTool>(
                    make_failing_descriptor(),
                    true,
                    true);
            });

        ToolManager manager{ registry };

        manager.activate_tool(
            context,
            ToolId{ "editor.select" });

        const ToolResult blocked =
            manager.activate_tool(
                context,
                ToolId{ "editor.blocked" });

        print_tool_result("activate blocked", blocked);

        print_result(
            blocked.failed() &&
            manager.is_active(
                ToolId{ "editor.select" }),
            "can_activate falso preservou tool anterior");

        const ToolResult failing =
            manager.activate_tool(
                context,
                ToolId{ "editor.failing" });

        print_tool_result("activate failing", failing);

        print_result(
            failing.failed() &&
            manager.is_active(
                ToolId{ "editor.select" }),
            "falha de activate restaurou tool anterior");

        const ToolResult switched =
            manager.activate_tool(
                context,
                ToolId{ "editor.transform" });

        print_tool_result("switch to transform", switched);

        print_result(
            !switched.failed() &&
            manager.is_active(
                ToolId{ "editor.transform" }),
            "troca valida ativou TransformTool");

        const ToolResult sameTool =
            manager.activate_tool(
                context,
                ToolId{ "editor.transform" });

        print_tool_result(
            "activate already active",
            sameTool);

        print_result(
            sameTool.code ==
            ToolResultCode::Consumed &&
            manager.is_active(
                ToolId{ "editor.transform" }),
            "reativar mesmo id nao recriou instancia");

        const ToolResult deactivated =
            manager.deactivate_tool(context);

        print_tool_result("deactivate", deactivated);

        print_result(
            !deactivated.failed() &&
            !manager.has_active_tool(),
            "deactivate removeu tool ativa");

        const ToolResult noActive =
            manager.handle_event(
                context,
                ToolEvent{});

        print_result(
            noActive.code ==
            ToolResultCode::Ignored,
            "evento sem tool ativa foi ignorado");

        return
            blocked.failed() &&
            failing.failed() &&
            !switched.failed() &&
            sameTool.code ==
            ToolResultCode::Consumed &&
            !manager.has_active_tool() &&
            noActive.code ==
            ToolResultCode::Ignored;
    }

} // namespace

int main() {
    std::cout
        << "=== Locus3D Editor Tool Management Smoke Test ===\n";

    bool ok = true;

    ok = test_registry() && ok;
    ok = test_manager_activation_and_events() && ok;
    ok = test_manager_cancel_on_focus_loss() && ok;
    ok = test_manager_switch_and_restore() && ok;

    std::cout << "\n=== Resultado final ===\n";

    print_result(
        ok,
        "ToolRegistry e ToolManager smoke test");

    return ok ? 0 : 1;
}