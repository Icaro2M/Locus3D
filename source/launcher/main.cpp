/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"
#include "editor/tools/core/ToolContext.h"
#include "editor/tools/core/ToolDescriptor.h"
#include "editor/tools/core/ToolEvent.h"
#include "editor/tools/core/ToolResult.h"
#include "editor/tools/core/ToolState.h"
#include "editor/tools/interaction/DragTool.h"
#include "editor/tools/interaction/ModalTool.h"
#include "editor/tools/interaction/ToolCancelReason.h"
#include "editor/tools/management/ToolManager.h"
#include "editor/tools/management/ToolRegistry.h"

#include <glm/vec2.hpp>

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

    const char* tool_state_name(ToolState state) {
        switch (state) {
        case ToolState::Inactive:
            return "Inactive";

        case ToolState::Ready:
            return "Ready";

        case ToolState::Interacting:
            return "Interacting";

        case ToolState::Suspended:
            return "Suspended";
        }

        return "Unknown";
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

    const char* cancel_reason_name(
        ToolCancelReason reason) {

        switch (reason) {
        case ToolCancelReason::UserRequest:
            return "UserRequest";

        case ToolCancelReason::FocusLost:
            return "FocusLost";

        case ToolCancelReason::ToolSwitch:
            return "ToolSwitch";

        case ToolCancelReason::ToolDeactivated:
            return "ToolDeactivated";

        case ToolCancelReason::InvalidState:
            return "InvalidState";
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
            << "  terminal: "
            << (result.is_terminal() ? "true" : "false")
            << '\n';

        std::cout
            << "  message: "
            << result.message
            << '\n';
    }

    ToolDescriptor make_drag_descriptor(
        const std::string& id,
        const std::string& name) {

        return ToolDescriptor{
            ToolId{ id },
            name,
            "Dummy drag tool used by the interaction smoke test.",
            ToolCategory::Transform,
            ToolCapabilities::ObjectMode |
                ToolCapabilities::UsesPointer |
                ToolCapabilities::Modal
        };
    }

    /**
     * @brief Drag tool used to exercise the generic interaction infrastructure.
     */
    class DummyDragTool final : public DragTool {
    public:
        DummyDragTool(
            ToolDescriptor descriptor,
            DragCompletionPolicy completionPolicy)
            : DragTool(
                std::move(descriptor),
                completionPolicy) {
        }

        [[nodiscard]] int activate_count() const {
            return activationCount_;
        }

        [[nodiscard]] int deactivate_count() const {
            return deactivationCount_;
        }

        [[nodiscard]] int begin_count() const {
            return beginCount_;
        }

        [[nodiscard]] int update_count() const {
            return updateCount_;
        }

        [[nodiscard]] int release_count() const {
            return releaseCount_;
        }

        [[nodiscard]] int confirm_count() const {
            return confirmCount_;
        }

        [[nodiscard]] int cancel_count() const {
            return cancelCount_;
        }

        [[nodiscard]] ToolCancelReason last_cancel_reason() const {
            return lastCancelReason_;
        }

        [[nodiscard]] const glm::vec2& last_total_delta() const {
            return lastTotalDelta_;
        }

    protected:
        ToolResult on_activate(
            ToolContext& context) override {

            (void)context;

            ++activationCount_;

            return ToolResult::consumed(
                EditorDirtyFlags::None,
                "Dummy drag tool activated.");
        }

        ToolResult on_deactivate(
            ToolContext& context) override {

            (void)context;

            ++deactivationCount_;

            return ToolResult::consumed(
                EditorDirtyFlags::None,
                "Dummy drag tool deactivated.");
        }

        ToolResult on_begin_drag(
            ToolContext& context,
            const ToolEvent& event) override {

            (void)context;

            ++beginCount_;
            beginPosition_ =
                event.pointer.viewportPosition;

            return ToolResult::started(
                EditorDirtyFlags::Selection,
                "Dummy drag started.");
        }

        ToolResult on_update_drag(
            ToolContext& context,
            const ToolEvent& event) override {

            (void)context;
            (void)event;

            ++updateCount_;
            lastTotalDelta_ = capture().total_delta();

            return ToolResult::updated(
                EditorDirtyFlags::Render,
                "Dummy drag updated.");
        }

        ToolResult on_release_drag(
            ToolContext& context,
            const ToolEvent& event) override {

            (void)context;

            ++releaseCount_;
            releasePosition_ =
                event.pointer.viewportPosition;

            return ToolResult::consumed(
                EditorDirtyFlags::Render,
                "Dummy pointer released.");
        }

        ToolResult on_confirm_drag(
            ToolContext& context) override {

            (void)context;

            ++confirmCount_;

            return ToolResult::confirmed(
                EditorDirtyFlags::Scene,
                "Dummy drag confirmed.");
        }

        ToolResult on_cancel_drag(
            ToolContext& context,
            ToolCancelReason reason) override {

            (void)context;

            ++cancelCount_;
            lastCancelReason_ = reason;

            return ToolResult::cancelled(
                EditorDirtyFlags::Render,
                "Dummy drag cancelled.");
        }

    private:
        int activationCount_ = 0;
        int deactivationCount_ = 0;
        int beginCount_ = 0;
        int updateCount_ = 0;
        int releaseCount_ = 0;
        int confirmCount_ = 0;
        int cancelCount_ = 0;

        ToolCancelReason lastCancelReason_ =
            ToolCancelReason::InvalidState;

        glm::vec2 beginPosition_{ 0.0f, 0.0f };
        glm::vec2 releasePosition_{ 0.0f, 0.0f };
        glm::vec2 lastTotalDelta_{ 0.0f, 0.0f };
    };

    ToolEvent make_pointer_event(
        ToolEventType type,
        ToolPointerButton button,
        float x,
        float y) {

        ToolEvent event{};
        event.type = type;
        event.button = button;
        event.pointer.viewportPosition =
            glm::vec2{ x, y };

        return event;
    }

    bool almost_equal(float lhs, float rhs) {
        constexpr float epsilon = 0.0001f;

        const float difference =
            lhs >= rhs
            ? lhs - rhs
            : rhs - lhs;

        return difference <= epsilon;
    }

    bool test_automatic_confirmation() {
        std::cout
            << "\n=== DragTool: confirmacao automatica ===\n";

        Editor editor{};
        editor.clear_dirty();

        ToolContext context{ editor };
        ToolRegistry registry{};

        const ToolId toolId{
            "editor.test.auto-drag"
        };

        registry.register_tool(
            make_drag_descriptor(
                toolId.value,
                "Automatic Drag"),
            [] {
                return std::make_unique<DummyDragTool>(
                    make_drag_descriptor(
                        "editor.test.auto-drag",
                        "Automatic Drag"),
                    DragCompletionPolicy::ConfirmOnRelease);
            });

        ToolManager manager{ registry };

        const ToolResult activation =
            manager.activate_tool(
                context,
                toolId);

        print_tool_result(
            "activate automatic drag",
            activation);

        DummyDragTool* tool =
            dynamic_cast<DummyDragTool*>(
                manager.active_tool());

        print_result(
            tool != nullptr,
            "manager possui DummyDragTool ativa");

        print_result(
            tool != nullptr &&
            tool->state() == ToolState::Ready &&
            tool->activate_count() == 1,
            "activate colocou tool em Ready");

        const ToolEvent secondaryPress =
            make_pointer_event(
                ToolEventType::PointerPress,
                ToolPointerButton::Secondary,
                5.0f,
                10.0f);

        const ToolResult secondaryResult =
            manager.handle_event(
                context,
                secondaryPress);

        print_tool_result(
            "secondary press",
            secondaryResult);

        print_result(
            secondaryResult.code ==
            ToolResultCode::Ignored &&
            tool != nullptr &&
            tool->state() == ToolState::Ready,
            "botao secundario nao iniciou drag");

        const ToolEvent primaryPress =
            make_pointer_event(
                ToolEventType::PointerPress,
                ToolPointerButton::Primary,
                10.0f,
                20.0f);

        const ToolResult started =
            manager.handle_event(
                context,
                primaryPress);

        print_tool_result(
            "primary press",
            started);

        print_result(
            started.code == ToolResultCode::Started &&
            tool != nullptr &&
            tool->state() == ToolState::Interacting &&
            tool->begin_count() == 1,
            "botao primario iniciou drag");

        print_result(
            tool != nullptr &&
            tool->capture().has_pointer() &&
            tool->capture().matches_button(
                ToolPointerButton::Primary),
            "drag capturou o ponteiro primario");

        print_result(
            has_flag(
                editor.dirty_flags(),
                EditorDirtyFlags::Selection),
            "inicio do drag marcou Selection dirty");

        editor.clear_dirty();

        const ToolEvent move =
            make_pointer_event(
                ToolEventType::PointerMove,
                ToolPointerButton::None,
                35.0f,
                60.0f);

        const ToolResult updated =
            manager.handle_event(
                context,
                move);

        print_tool_result(
            "pointer move",
            updated);

        const glm::vec2 expectedDelta{
            25.0f,
            40.0f
        };

        print_result(
            updated.code == ToolResultCode::Updated &&
            tool != nullptr &&
            tool->update_count() == 1,
            "movimento atualizou drag ativo");

        print_result(
            tool != nullptr &&
            almost_equal(
                tool->last_total_delta().x,
                expectedDelta.x) &&
            almost_equal(
                tool->last_total_delta().y,
                expectedDelta.y),
            "capture calculou delta total esperado");

        print_result(
            has_flag(
                editor.dirty_flags(),
                EditorDirtyFlags::Render),
            "update marcou Render dirty");

        editor.clear_dirty();

        const ToolEvent wrongRelease =
            make_pointer_event(
                ToolEventType::PointerRelease,
                ToolPointerButton::Secondary,
                35.0f,
                60.0f);

        const ToolResult wrongReleaseResult =
            manager.handle_event(
                context,
                wrongRelease);

        print_result(
            wrongReleaseResult.code ==
            ToolResultCode::Ignored &&
            tool != nullptr &&
            tool->state() == ToolState::Interacting &&
            tool->capture().has_pointer(),
            "release de outro botao nao encerrou captura");

        const ToolEvent primaryRelease =
            make_pointer_event(
                ToolEventType::PointerRelease,
                ToolPointerButton::Primary,
                40.0f,
                70.0f);

        const ToolResult confirmed =
            manager.handle_event(
                context,
                primaryRelease);

        print_tool_result(
            "primary release",
            confirmed);

        print_result(
            confirmed.code ==
            ToolResultCode::Confirmed &&
            tool != nullptr &&
            tool->state() == ToolState::Ready,
            "release primario confirmou drag automaticamente");

        print_result(
            tool != nullptr &&
            tool->release_count() == 1 &&
            tool->confirm_count() == 1,
            "release e confirm foram executados uma vez");

        print_result(
            tool != nullptr &&
            !tool->capture().is_active(),
            "captura foi limpa apos confirmacao");

        print_result(
            has_flag(
                editor.dirty_flags(),
                EditorDirtyFlags::Render) &&
            has_flag(
                editor.dirty_flags(),
                EditorDirtyFlags::Scene),
            "release combinou dirty flags de Render e Scene");

        return
            !activation.failed() &&
            tool != nullptr &&
            started.code == ToolResultCode::Started &&
            updated.code == ToolResultCode::Updated &&
            confirmed.code == ToolResultCode::Confirmed &&
            tool->state() == ToolState::Ready &&
            tool->confirm_count() == 1 &&
            !tool->capture().is_active();
    }

    bool test_explicit_confirmation() {
        std::cout
            << "\n=== DragTool: confirmacao explicita ===\n";

        Editor editor{};
        editor.clear_dirty();

        ToolContext context{ editor };
        ToolRegistry registry{};

        const ToolId toolId{
            "editor.test.explicit-drag"
        };

        registry.register_tool(
            make_drag_descriptor(
                toolId.value,
                "Explicit Drag"),
            [] {
                return std::make_unique<DummyDragTool>(
                    make_drag_descriptor(
                        "editor.test.explicit-drag",
                        "Explicit Drag"),
                    DragCompletionPolicy::
                    WaitForExplicitConfirmation);
            });

        ToolManager manager{ registry };

        manager.activate_tool(
            context,
            toolId);

        DummyDragTool* tool =
            dynamic_cast<DummyDragTool*>(
                manager.active_tool());

        const ToolEvent press =
            make_pointer_event(
                ToolEventType::PointerPress,
                ToolPointerButton::Primary,
                100.0f,
                80.0f);

        manager.handle_event(
            context,
            press);

        const ToolEvent release =
            make_pointer_event(
                ToolEventType::PointerRelease,
                ToolPointerButton::Primary,
                120.0f,
                90.0f);

        const ToolResult released =
            manager.handle_event(
                context,
                release);

        print_tool_result(
            "release awaiting confirmation",
            released);

        print_result(
            released.was_consumed() &&
            tool != nullptr &&
            tool->state() == ToolState::Interacting,
            "release manteve sessao interativa ativa");

        print_result(
            tool != nullptr &&
            !tool->capture().is_active(),
            "release encerrou apenas a captura do ponteiro");

        print_result(
            tool != nullptr &&
            tool->release_count() == 1 &&
            tool->confirm_count() == 0,
            "release nao confirmou a operacao");

        ToolEvent confirmEvent{};
        confirmEvent.type = ToolEventType::Confirm;

        const ToolResult confirmed =
            manager.handle_event(
                context,
                confirmEvent);

        print_tool_result(
            "explicit confirm",
            confirmed);

        print_result(
            confirmed.code ==
            ToolResultCode::Confirmed &&
            tool != nullptr &&
            tool->state() == ToolState::Ready,
            "evento Confirm finalizou sessao");

        print_result(
            tool != nullptr &&
            tool->confirm_count() == 1,
            "confirmacao explicita foi executada uma vez");

        return
            tool != nullptr &&
            released.was_consumed() &&
            confirmed.code == ToolResultCode::Confirmed &&
            tool->state() == ToolState::Ready &&
            tool->confirm_count() == 1;
    }

    bool test_user_cancellation() {
        std::cout
            << "\n=== DragTool: cancelamento do usuario ===\n";

        Editor editor{};
        editor.clear_dirty();

        ToolContext context{ editor };
        ToolRegistry registry{};

        const ToolId toolId{
            "editor.test.cancel-drag"
        };

        registry.register_tool(
            make_drag_descriptor(
                toolId.value,
                "Cancelable Drag"),
            [] {
                return std::make_unique<DummyDragTool>(
                    make_drag_descriptor(
                        "editor.test.cancel-drag",
                        "Cancelable Drag"),
                    DragCompletionPolicy::
                    WaitForExplicitConfirmation);
            });

        ToolManager manager{ registry };

        manager.activate_tool(
            context,
            toolId);

        DummyDragTool* tool =
            dynamic_cast<DummyDragTool*>(
                manager.active_tool());

        manager.handle_event(
            context,
            make_pointer_event(
                ToolEventType::PointerPress,
                ToolPointerButton::Primary,
                0.0f,
                0.0f));

        manager.handle_event(
            context,
            make_pointer_event(
                ToolEventType::PointerMove,
                ToolPointerButton::None,
                10.0f,
                15.0f));

        ToolEvent cancelEvent{};
        cancelEvent.type = ToolEventType::Cancel;

        const ToolResult cancelled =
            manager.handle_event(
                context,
                cancelEvent);

        print_tool_result(
            "user cancel",
            cancelled);

        print_result(
            cancelled.code ==
            ToolResultCode::Cancelled &&
            tool != nullptr &&
            tool->state() == ToolState::Ready,
            "Cancel encerrou sessao e retornou a Ready");

        print_result(
            tool != nullptr &&
            tool->cancel_count() == 1 &&
            tool->last_cancel_reason() ==
            ToolCancelReason::UserRequest,
            "cancelamento recebeu motivo UserRequest");

        print_result(
            tool != nullptr &&
            !tool->capture().is_active(),
            "cancelamento limpou captura");

        print_result(
            has_flag(
                editor.dirty_flags(),
                EditorDirtyFlags::Render),
            "cancelamento marcou Render dirty");

        std::cout
            << "  cancel reason: "
            << (
                tool != nullptr
                ? cancel_reason_name(
                    tool->last_cancel_reason())
                : "Unavailable")
            << '\n';

        return
            tool != nullptr &&
            cancelled.code ==
            ToolResultCode::Cancelled &&
            tool->state() == ToolState::Ready &&
            tool->cancel_count() == 1 &&
            tool->last_cancel_reason() ==
            ToolCancelReason::UserRequest &&
            !tool->capture().is_active();
    }

    bool test_deactivation_cancels_drag() {
        std::cout
            << "\n=== ModalTool: deactivate durante drag ===\n";

        Editor editor{};
        editor.clear_dirty();

        ToolContext context{ editor };
        ToolRegistry registry{};

        const ToolId toolId{
            "editor.test.deactivate-drag"
        };

        registry.register_tool(
            make_drag_descriptor(
                toolId.value,
                "Deactivate Drag"),
            [] {
                return std::make_unique<DummyDragTool>(
                    make_drag_descriptor(
                        "editor.test.deactivate-drag",
                        "Deactivate Drag"),
                    DragCompletionPolicy::
                    WaitForExplicitConfirmation);
            });

        ToolManager manager{ registry };

        manager.activate_tool(
            context,
            toolId);

        DummyDragTool* tool =
            dynamic_cast<DummyDragTool*>(
                manager.active_tool());

        manager.handle_event(
            context,
            make_pointer_event(
                ToolEventType::PointerPress,
                ToolPointerButton::Primary,
                15.0f,
                25.0f));

        print_result(
            tool != nullptr &&
            tool->state() == ToolState::Interacting,
            "drag estava ativo antes de deactivate");

        /*
         * Preserve test observations before the manager destroys the active tool
         * instance after successful deactivation.
         */
        int cancelCountBefore = 0;
        int deactivateCountBefore = 0;
        ToolCancelReason cancelReasonBefore =
            ToolCancelReason::InvalidState;

        const ToolResult deactivated =
            manager.deactivate_tool(context);

        print_tool_result(
            "deactivate active drag",
            deactivated);

        /*
         * The owned tool no longer exists after deactivate_tool(), so values that
         * belong to its callbacks are observed through a separate fixture below.
         */
        (void)cancelCountBefore;
        (void)deactivateCountBefore;
        (void)cancelReasonBefore;

        print_result(
            !deactivated.failed() &&
            !manager.has_active_tool(),
            "deactivate removeu tool ativa");

        return
            !deactivated.failed() &&
            !manager.has_active_tool();
    }

    struct SharedDragObservation {
        int cancelCount = 0;
        int deactivateCount = 0;

        ToolCancelReason lastCancelReason =
            ToolCancelReason::InvalidState;
    };

    /**
     * @brief Drag fixture that writes lifecycle observations outside its lifetime.
     */
    class ObservedDragTool final : public DragTool {
    public:
        explicit ObservedDragTool(
            SharedDragObservation& observation)
            : DragTool(
                make_drag_descriptor(
                    "editor.test.observed-drag",
                    "Observed Drag"),
                DragCompletionPolicy::
                WaitForExplicitConfirmation),
            observation_(&observation) {
        }

    protected:
        ToolResult on_deactivate(
            ToolContext& context) override {

            (void)context;

            ++observation_->deactivateCount;

            return ToolResult::consumed(
                EditorDirtyFlags::None,
                "Observed tool deactivated.");
        }

        ToolResult on_begin_drag(
            ToolContext& context,
            const ToolEvent& event) override {

            (void)context;
            (void)event;

            return ToolResult::started(
                EditorDirtyFlags::None,
                "Observed drag started.");
        }

        ToolResult on_update_drag(
            ToolContext& context,
            const ToolEvent& event) override {

            (void)context;
            (void)event;

            return ToolResult::updated(
                EditorDirtyFlags::None,
                "Observed drag updated.");
        }

        ToolResult on_confirm_drag(
            ToolContext& context) override {

            (void)context;

            return ToolResult::confirmed(
                EditorDirtyFlags::None,
                "Observed drag confirmed.");
        }

        ToolResult on_cancel_drag(
            ToolContext& context,
            ToolCancelReason reason) override {

            (void)context;

            ++observation_->cancelCount;
            observation_->lastCancelReason = reason;

            return ToolResult::cancelled(
                EditorDirtyFlags::Render,
                "Observed drag cancelled.");
        }

    private:
        SharedDragObservation* observation_ = nullptr;
    };

    bool test_deactivation_cancel_reason() {
        std::cout
            << "\n=== ModalTool: motivo ao desativar ===\n";

        Editor editor{};
        editor.clear_dirty();

        ToolContext context{ editor };
        ToolRegistry registry{};

        SharedDragObservation observation{};

        registry.register_tool(
            make_drag_descriptor(
                "editor.test.observed-drag",
                "Observed Drag"),
            [&observation] {
                return std::make_unique<ObservedDragTool>(
                    observation);
            });

        ToolManager manager{ registry };

        manager.activate_tool(
            context,
            ToolId{ "editor.test.observed-drag" });

        manager.handle_event(
            context,
            make_pointer_event(
                ToolEventType::PointerPress,
                ToolPointerButton::Primary,
                5.0f,
                5.0f));

        const ToolResult deactivated =
            manager.deactivate_tool(context);

        print_tool_result(
            "deactivate observed drag",
            deactivated);

        std::cout
            << "  cancel reason: "
            << cancel_reason_name(
                observation.lastCancelReason)
            << '\n';

        print_result(
            observation.cancelCount == 1,
            "deactivate cancelou drag uma vez");

        print_result(
            observation.lastCancelReason ==
            ToolCancelReason::ToolDeactivated,
            "deactivate informou ToolDeactivated");

        print_result(
            observation.deactivateCount == 1,
            "hook on_deactivate foi chamado uma vez");

        print_result(
            has_flag(
                editor.dirty_flags(),
                EditorDirtyFlags::Render),
            "dirty flags do cancel foram preservadas");

        print_result(
            !manager.has_active_tool(),
            "manager ficou sem tool ativa");

        return
            !deactivated.failed() &&
            observation.cancelCount == 1 &&
            observation.lastCancelReason ==
            ToolCancelReason::ToolDeactivated &&
            observation.deactivateCount == 1 &&
            !manager.has_active_tool();
    }

    bool test_events_while_inactive() {
        std::cout
            << "\n=== ModalTool: eventos sem ativacao ===\n";

        Editor editor{};
        ToolContext context{ editor };

        DummyDragTool tool{
            make_drag_descriptor(
                "editor.test.inactive-drag",
                "Inactive Drag"),
            DragCompletionPolicy::ConfirmOnRelease
        };

        const ToolResult eventResult =
            tool.handle_event(
                context,
                make_pointer_event(
                    ToolEventType::PointerPress,
                    ToolPointerButton::Primary,
                    0.0f,
                    0.0f));

        const ToolResult confirmResult =
            tool.confirm(context);

        const ToolResult cancelResult =
            tool.cancel(context);

        print_result(
            tool.state() == ToolState::Inactive,
            "tool comeca Inactive");

        print_result(
            eventResult.code ==
            ToolResultCode::Ignored,
            "evento em tool inativa foi ignorado");

        print_result(
            confirmResult.code ==
            ToolResultCode::Ignored,
            "confirm em tool inativa foi ignorado");

        print_result(
            cancelResult.code ==
            ToolResultCode::Ignored,
            "cancel em tool inativa foi ignorado");

        return
            tool.state() == ToolState::Inactive &&
            eventResult.code ==
            ToolResultCode::Ignored &&
            confirmResult.code ==
            ToolResultCode::Ignored &&
            cancelResult.code ==
            ToolResultCode::Ignored;
    }

} // namespace

int main() {
    std::cout
        << "=== Locus3D Editor Tool Interaction Smoke Test ===\n";

    bool ok = true;

    ok = test_automatic_confirmation() && ok;
    ok = test_explicit_confirmation() && ok;
    ok = test_user_cancellation() && ok;
    ok = test_deactivation_cancels_drag() && ok;
    ok = test_deactivation_cancel_reason() && ok;
    ok = test_events_while_inactive() && ok;

    std::cout
        << "\n=== Resultado final ===\n";

    print_result(
        ok,
        "ModalTool e DragTool smoke test");

    return ok ? 0 : 1;
}