/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"
#include "editor/scene/MeshNode.h"
#include "editor/tools/core/ToolContext.h"
#include "editor/tools/core/ToolDescriptor.h"
#include "editor/tools/core/ToolEvent.h"
#include "editor/tools/core/ToolResult.h"
#include "editor/tools/core/ToolState.h"
#include "editor/tools/interaction/DragTool.h"
#include "editor/tools/mesh/core/MeshDragOperationTool.h"
#include "editor/tools/mesh/core/MeshToolTarget.h"
#include "kernel/common/Error.h"
#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/modeling/core/IOperation.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/core/OperationResult.h"

#include <glm/geometric.hpp>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace {

    using locus::editor::DragCompletionPolicy;
    using locus::editor::Editor;
    using locus::editor::EditorDirtyFlags;
    using locus::editor::EditorMode;
    using locus::editor::MeshDragOperationTool;
    using locus::editor::MeshNode;
    using locus::editor::MeshToolTarget;
    using locus::editor::SceneNodeId;
    using locus::editor::SelectionGranularity;
    using locus::editor::ToolCapabilities;
    using locus::editor::ToolCategory;
    using locus::editor::ToolContext;
    using locus::editor::ToolDescriptor;
    using locus::editor::ToolEvent;
    using locus::editor::ToolEventType;
    using locus::editor::ToolId;
    using locus::editor::ToolPointerButton;
    using locus::editor::ToolResult;
    using locus::editor::ToolResultCode;
    using locus::editor::ToolState;

    using locus::kernel::ErrorCode;

    using locus::kernel::geometry::FaceHandle;
    using locus::kernel::geometry::LEM;
    using locus::kernel::geometry::LEMChangeType;
    using locus::kernel::geometry::LEMDiff;
    using locus::kernel::geometry::VertexHandle;

    using locus::kernel::modeling::IOperation;
    using locus::kernel::modeling::OperationContext;
    using locus::kernel::modeling::OperationResult;

    struct QuadFixture {
        SceneNodeId nodeId{};

        VertexHandle vertex0{};
        VertexHandle vertex1{};
        VertexHandle vertex2{};
        VertexHandle vertex3{};

        FaceHandle face{};
    };

    bool expect(
        const bool condition,
        const std::string& message)
    {
        if (condition) {
            std::cout << "[OK] " << message << '\n';
            return true;
        }

        std::cout << "[FAIL] " << message << '\n';
        return false;
    }

    bool nearly_equal(
        const glm::vec3& lhs,
        const glm::vec3& rhs,
        const float epsilon = 0.0001f)
    {
        return glm::length(lhs - rhs) <= epsilon;
    }

    bool has_dirty_flag(
        const ToolResult& result,
        const EditorDirtyFlags flag)
    {
        return locus::editor::has_flag(
            result.dirtyFlags,
            flag);
    }

    QuadFixture create_quad(
        Editor& editor,
        const std::string& name = "Mesh Drag Fixture")
    {
        QuadFixture fixture{};

        fixture.nodeId =
            editor.scene().create_mesh(name);

        MeshNode* node =
            editor.scene().find_mesh(fixture.nodeId);

        if (!node) {
            return fixture;
        }

        LEM& mesh = node->mesh();

        fixture.vertex0 =
            mesh.add_vertex(glm::vec3{
                -1.0f,
                -1.0f,
                0.0f
                });

        fixture.vertex1 =
            mesh.add_vertex(glm::vec3{
                1.0f,
                -1.0f,
                0.0f
                });

        fixture.vertex2 =
            mesh.add_vertex(glm::vec3{
                1.0f,
                1.0f,
                0.0f
                });

        fixture.vertex3 =
            mesh.add_vertex(glm::vec3{
                -1.0f,
                1.0f,
                0.0f
                });

        fixture.face =
            mesh.add_face({
                fixture.vertex0,
                fixture.vertex1,
                fixture.vertex2,
                fixture.vertex3
                });

        return fixture;
    }

    void select_fixture_face(
        Editor& editor,
        const QuadFixture& fixture)
    {
        auto& selection =
            editor.selection().mesh();

        selection.set_active_mesh(fixture.nodeId);
        selection.set_face(fixture.face);
    }

    ToolEvent make_pointer_event(
        const ToolEventType type,
        const glm::vec2 position,
        const ToolPointerButton button =
        ToolPointerButton::None)
    {
        ToolEvent event{};

        event.type = type;
        event.button = button;
        event.pointer.viewportPosition = position;

        return event;
    }

    ToolEvent make_pointer_press(
        const glm::vec2 position)
    {
        return make_pointer_event(
            ToolEventType::PointerPress,
            position,
            ToolPointerButton::Primary);
    }

    ToolEvent make_pointer_move(
        const glm::vec2 position,
        const glm::vec2 delta = glm::vec2{ 0.0f })
    {
        ToolEvent event =
            make_pointer_event(
                ToolEventType::PointerMove,
                position);

        event.pointer.viewportDelta = delta;

        return event;
    }

    ToolEvent make_pointer_release(
        const glm::vec2 position)
    {
        return make_pointer_event(
            ToolEventType::PointerRelease,
            position,
            ToolPointerButton::Primary);
    }

    class MoveVertexOperation final : public IOperation {
    public:
        MoveVertexOperation(
            const VertexHandle vertex,
            const glm::vec3 delta)
            : vertex_(vertex),
            delta_(delta)
        {
        }

        [[nodiscard]]
        std::string_view name() const override
        {
            return "MoveVertexOperation";
        }

    private:
        [[nodiscard]]
        OperationResult execute_impl(
            OperationContext& context) override
        {
            LEM& mesh =
                context.editable_mesh();

            if (!mesh.is_valid(vertex_)) {
                return OperationResult::fail(
                    ErrorCode::InvalidArgument,
                    "MoveVertexOperation received an invalid vertex.");
            }

            mesh.vertex(vertex_).position += delta_;

            LEMDiff diff{};

            diff.record(
                LEMChangeType::VertexModified,
                vertex_);

            return OperationResult::success(
                std::move(diff));
        }

        VertexHandle vertex_{};
        glm::vec3 delta_{ 0.0f };
    };

    class TestMeshDragTool final :
        public MeshDragOperationTool {
    public:
        explicit TestMeshDragTool(
            const DragCompletionPolicy completionPolicy =
            DragCompletionPolicy::ConfirmOnRelease)
            : MeshDragOperationTool(
                make_descriptor(),
                SelectionGranularity::Face,
                completionPolicy)
        {
        }

        [[nodiscard]]
        float amount() const
        {
            return amount_;
        }

        [[nodiscard]]
        int begin_count() const
        {
            return beginCount_;
        }

        [[nodiscard]]
        int update_count() const
        {
            return updateCount_;
        }

        [[nodiscard]]
        int preview_build_count() const
        {
            return previewBuildCount_;
        }

        [[nodiscard]]
        int commit_count() const
        {
            return commitCount_;
        }

        [[nodiscard]]
        int clear_count() const
        {
            return clearCount_;
        }

        [[nodiscard]]
        bool concrete_state_active() const
        {
            return concreteStateActive_;
        }

        [[nodiscard]]
        SceneNodeId committed_node() const
        {
            return committedNode_;
        }

        [[nodiscard]]
        float committed_amount() const
        {
            return committedAmount_;
        }

        void set_fail_begin(
            const bool fail)
        {
            failBegin_ = fail;
        }

        void set_fail_commit(
            const bool fail)
        {
            failCommit_ = fail;
        }

        void set_null_preview_operation(
            const bool enabled)
        {
            nullPreviewOperation_ = enabled;
        }

        void set_ignore_updates(
            const bool enabled)
        {
            ignoreUpdates_ = enabled;
        }

    protected:
        ToolResult begin_mesh_operation(
            ToolContext& context,
            const ToolEvent& event,
            const MeshToolTarget& target) override
        {
            (void)context;

            ++beginCount_;

            if (failBegin_) {
                return ToolResult::fail(
                    "Intentional mesh operation begin failure.");
            }

            if (!target.targets_faces()) {
                return ToolResult::fail(
                    "Test mesh tool requires a face target.");
            }

            startPosition_ =
                event.pointer.viewportPosition;

            amount_ = 0.0f;
            concreteStateActive_ = true;

            return ToolResult::consumed(
                EditorDirtyFlags::None,
                "Test mesh operation initialized.");
        }

        ToolResult update_mesh_operation(
            ToolContext& context,
            const ToolEvent& event,
            const MeshToolTarget& target) override
        {
            (void)context;
            (void)target;

            ++updateCount_;

            if (ignoreUpdates_) {
                return ToolResult::ignored();
            }

            const glm::vec2 offset =
                event.pointer.viewportPosition -
                startPosition_;

            amount_ = offset.x * 0.1f;

            return ToolResult::updated(
                EditorDirtyFlags::None,
                "Test mesh parameters updated.");
        }

        [[nodiscard]]
        std::unique_ptr<IOperation>
            build_preview_operation(
                const ToolContext& context,
                const MeshToolTarget& target) const override
        {
            ++previewBuildCount_;

            if (nullPreviewOperation_) {
                return nullptr;
            }

            const MeshNode* node =
                context.scene().find_mesh(
                    target.nodeId);

            if (!node ||
                target.faces.empty()) {
                return nullptr;
            }

            const LEM& mesh =
                node->mesh();

            const FaceHandle face =
                target.faces.front();

            if (!mesh.is_valid(face)) {
                return nullptr;
            }

            const auto& faceElement =
                mesh.face(face);

            if (faceElement.loop.is_invalid()) {
                return nullptr;
            }

            const auto& loopElement =
                mesh.loop(faceElement.loop);

            if (loopElement.vertex.is_invalid()) {
                return nullptr;
            }

            return std::make_unique<MoveVertexOperation>(
                loopElement.vertex,
                glm::vec3{
                    0.0f,
                    0.0f,
                    amount_
                });
        }

        ToolResult commit_mesh_operation(
            ToolContext& context,
            const MeshToolTarget& target) override
        {
            (void)context;

            if (failCommit_) {
                return ToolResult::fail(
                    "Intentional mesh operation commit failure.");
            }

            ++commitCount_;

            committedNode_ = target.nodeId;
            committedAmount_ = amount_;

            return ToolResult::confirmed(
                EditorDirtyFlags::Mesh |
                EditorDirtyFlags::Render |
                EditorDirtyFlags::Picking,
                "Test mesh operation committed.");
        }

        void clear_mesh_operation() override
        {
            ++clearCount_;

            amount_ = 0.0f;
            startPosition_ = glm::vec2{ 0.0f };
            concreteStateActive_ = false;
        }

    private:
        [[nodiscard]]
        static ToolDescriptor make_descriptor()
        {
            return ToolDescriptor{
                ToolId{ "test.mesh_drag" },
                "Test Mesh Drag",
                "Tests the shared mesh drag operation lifecycle.",
                ToolCategory::Mesh,
                ToolCapabilities::MeshMode |
                    ToolCapabilities::RequiresSelection |
                    ToolCapabilities::UsesPointer |
                    ToolCapabilities::UsesPreview |
                    ToolCapabilities::Modal
            };
        }

        glm::vec2 startPosition_{ 0.0f };
        float amount_ = 0.0f;

        int beginCount_ = 0;
        int updateCount_ = 0;

        mutable int previewBuildCount_ = 0;

        int commitCount_ = 0;
        int clearCount_ = 0;

        bool concreteStateActive_ = false;
        bool failBegin_ = false;
        bool failCommit_ = false;
        bool nullPreviewOperation_ = false;
        bool ignoreUpdates_ = false;

        SceneNodeId committedNode_{};
        float committedAmount_ = 0.0f;
    };

    bool test_activation_contract()
    {
        std::cout << "\n=== Activation contract ===\n";

        bool ok = true;

        Editor editor{};
        ToolContext context{ editor };

        TestMeshDragTool tool{};

        ok &= expect(
            tool.state() == ToolState::Inactive,
            "tool comeca Inactive");

        ok &= expect(
            !tool.can_activate(context),
            "tool nao ativa em Object mode");

        const ToolResult invalidActivation =
            tool.activate(context);

        ok &= expect(
            invalidActivation.code ==
            ToolResultCode::Failed,
            "activate falha em Object mode");

        ok &= expect(
            tool.state() == ToolState::Inactive,
            "falha de ativacao mantem Inactive");

        editor.set_mode(EditorMode::Mesh);

        ok &= expect(
            tool.can_activate(context),
            "tool pode ativar em Mesh mode");

        const ToolResult activation =
            tool.activate(context);

        ok &= expect(
            activation.code ==
            ToolResultCode::Consumed,
            "activate retorna Consumed");

        ok &= expect(
            tool.state() == ToolState::Ready,
            "activate move tool para Ready");

        ok &= expect(
            tool.descriptor().is_valid(),
            "descriptor da tool e valido");

        ok &= expect(
            tool.target_granularity() ==
            SelectionGranularity::Face,
            "tool preserva granularidade Face");

        const ToolResult duplicateActivation =
            tool.activate(context);

        ok &= expect(
            !duplicateActivation.failed(),
            "activate repetido nao falha");

        ok &= expect(
            tool.state() == ToolState::Ready,
            "activate repetido preserva estado Ready");

        const ToolResult deactivation =
            tool.deactivate(context);

        ok &= expect(
            deactivation.code ==
            ToolResultCode::Consumed,
            "deactivate retorna Consumed");

        ok &= expect(
            tool.state() == ToolState::Inactive,
            "deactivate retorna tool para Inactive");

        return ok;
    }

    bool test_press_without_target()
    {
        std::cout << "\n=== Press without target ===\n";

        bool ok = true;

        Editor editor{};
        editor.set_mode(EditorMode::Mesh);

        ToolContext context{ editor };
        TestMeshDragTool tool{};

        tool.activate(context);

        const ToolResult result =
            tool.handle_event(
                context,
                make_pointer_press(
                    glm::vec2{
                        10.0f,
                        20.0f
                    }));

        ok &= expect(
            result.code == ToolResultCode::Ignored,
            "press sem selecao retorna Ignored");

        ok &= expect(
            tool.state() == ToolState::Ready,
            "press sem selecao mantem Ready");

        ok &= expect(
            !tool.capture().is_active(),
            "press sem selecao nao captura pointer");

        ok &= expect(
            !tool.mesh_session().is_active(),
            "press sem selecao nao inicia mesh session");

        ok &= expect(
            tool.begin_count() == 0,
            "press sem selecao nao chama begin concreto");

        return ok;
    }

    bool test_begin_and_preview()
    {
        std::cout << "\n=== Begin and preview ===\n";

        bool ok = true;

        Editor editor{};
        editor.set_mode(EditorMode::Mesh);

        const QuadFixture fixture =
            create_quad(editor);

        select_fixture_face(
            editor,
            fixture);

        ToolContext context{ editor };
        TestMeshDragTool tool{};

        tool.activate(context);

        const MeshNode* node =
            editor.scene().find_mesh(
                fixture.nodeId);

        if (!node) {
            return expect(
                false,
                "fixture possui MeshNode");
        }

        const glm::vec3 originalPosition =
            node->mesh()
            .vertex(fixture.vertex0)
            .position;

        const ToolResult pressResult =
            tool.handle_event(
                context,
                make_pointer_press(
                    glm::vec2{
                        100.0f,
                        50.0f
                    }));

        ok &= expect(
            pressResult.code ==
            ToolResultCode::Started,
            "pointer press retorna Started");

        ok &= expect(
            tool.state() ==
            ToolState::Interacting,
            "pointer press move tool para Interacting");

        ok &= expect(
            tool.capture().has_pointer(),
            "pointer press inicia captura");

        ok &= expect(
            tool.mesh_session().is_active(),
            "pointer press inicia mesh session");

        ok &= expect(
            tool.has_operation_preview(),
            "pointer press gera preview inicial");

        ok &= expect(
            tool.operation_preview().is_ready(),
            "preview inicial possui status Ready");

        ok &= expect(
            tool.begin_count() == 1,
            "begin concreto foi chamado uma vez");

        ok &= expect(
            tool.preview_build_count() == 1,
            "preview inicial foi construido uma vez");

        ok &= expect(
            tool.concrete_state_active(),
            "estado concreto fica ativo");

        const glm::vec3 positionAfterPress =
            node->mesh()
            .vertex(fixture.vertex0)
            .position;

        ok &= expect(
            nearly_equal(
                positionAfterPress,
                originalPosition),
            "preview inicial nao altera LEM autoritativa");

        const ToolResult moveResult =
            tool.handle_event(
                context,
                make_pointer_move(
                    glm::vec2{
                        125.0f,
                        50.0f
                    },
                    glm::vec2{
                        25.0f,
                        0.0f
                    }));

        ok &= expect(
            moveResult.code ==
            ToolResultCode::Updated,
            "pointer move retorna Updated");

        ok &= expect(
            has_dirty_flag(
                moveResult,
                EditorDirtyFlags::Render),
            "pointer move marca render dirty");

        ok &= expect(
            tool.update_count() == 1,
            "pointer move chama update concreto");

        ok &= expect(
            tool.preview_build_count() == 2,
            "pointer move reconstrui preview");

        ok &= expect(
            tool.has_operation_preview(),
            "preview continua pronto apos move");

        ok &= expect(
            tool.amount() == 2.5f,
            "pointer move atualiza parametro concreto");

        const glm::vec3 positionAfterMove =
            node->mesh()
            .vertex(fixture.vertex0)
            .position;

        ok &= expect(
            nearly_equal(
                positionAfterMove,
                originalPosition),
            "preview atualizado nao altera LEM autoritativa");

        return ok;
    }

    bool test_confirm_on_release()
    {
        std::cout << "\n=== Confirm on release ===\n";

        bool ok = true;

        Editor editor{};
        editor.set_mode(EditorMode::Mesh);

        const QuadFixture fixture =
            create_quad(editor);

        select_fixture_face(
            editor,
            fixture);

        ToolContext context{ editor };
        TestMeshDragTool tool{};

        tool.activate(context);

        tool.handle_event(
            context,
            make_pointer_press(
                glm::vec2{
                    10.0f,
                    10.0f
                }));

        tool.handle_event(
            context,
            make_pointer_move(
                glm::vec2{
                    40.0f,
                    10.0f
                }));

        const ToolResult releaseResult =
            tool.handle_event(
                context,
                make_pointer_release(
                    glm::vec2{
                        40.0f,
                        10.0f
                    }));

        ok &= expect(
            releaseResult.code ==
            ToolResultCode::Confirmed,
            "pointer release confirma interacao");

        ok &= expect(
            tool.state() == ToolState::Ready,
            "confirmacao retorna tool para Ready");

        ok &= expect(
            !tool.capture().is_active(),
            "confirmacao limpa captura");

        ok &= expect(
            !tool.mesh_session().is_active(),
            "confirmacao limpa mesh session");

        ok &= expect(
            tool.commit_count() == 1,
            "confirmacao chama commit uma vez");

        ok &= expect(
            tool.committed_node() ==
            fixture.nodeId,
            "commit recebe node capturado");

        ok &= expect(
            tool.committed_amount() == 3.0f,
            "commit preserva parametro final");

        ok &= expect(
            !tool.concrete_state_active(),
            "confirmacao limpa estado concreto");

        ok &= expect(
            has_dirty_flag(
                releaseResult,
                EditorDirtyFlags::Mesh) &&
            has_dirty_flag(
                releaseResult,
                EditorDirtyFlags::Render) &&
            has_dirty_flag(
                releaseResult,
                EditorDirtyFlags::Picking),
            "confirmacao preserva dirty flags do commit");

        return ok;
    }

    bool test_explicit_confirmation_policy()
    {
        std::cout
            << "\n=== Explicit confirmation policy ===\n";

        bool ok = true;

        Editor editor{};
        editor.set_mode(EditorMode::Mesh);

        const QuadFixture fixture =
            create_quad(editor);

        select_fixture_face(
            editor,
            fixture);

        ToolContext context{ editor };

        TestMeshDragTool tool{
            DragCompletionPolicy::
                WaitForExplicitConfirmation
        };

        tool.activate(context);

        tool.handle_event(
            context,
            make_pointer_press(
                glm::vec2{
                    0.0f,
                    0.0f
                }));

        tool.handle_event(
            context,
            make_pointer_move(
                glm::vec2{
                    20.0f,
                    0.0f
                }));

        const ToolResult releaseResult =
            tool.handle_event(
                context,
                make_pointer_release(
                    glm::vec2{
                        20.0f,
                        0.0f
                    }));

        ok &= expect(
            releaseResult.code ==
            ToolResultCode::Consumed,
            "release explicito nao confirma automaticamente");

        ok &= expect(
            tool.state() ==
            ToolState::Interacting,
            "release explicito mantem Interacting");

        ok &= expect(
            !tool.capture().is_active(),
            "release explicito encerra captura");

        ok &= expect(
            tool.mesh_session().is_active(),
            "release explicito preserva mesh session");

        ok &= expect(
            tool.commit_count() == 0,
            "release explicito ainda nao chama commit");

        const ToolResult confirmResult =
            tool.confirm(context);

        ok &= expect(
            confirmResult.code ==
            ToolResultCode::Confirmed,
            "confirm semantico conclui interacao");

        ok &= expect(
            tool.state() == ToolState::Ready,
            "confirm semantico retorna Ready");

        ok &= expect(
            tool.commit_count() == 1,
            "confirm semantico chama commit");

        ok &= expect(
            !tool.mesh_session().is_active(),
            "confirm semantico limpa mesh session");

        return ok;
    }

    bool test_cancel()
    {
        std::cout << "\n=== Cancellation ===\n";

        bool ok = true;

        Editor editor{};
        editor.set_mode(EditorMode::Mesh);

        const QuadFixture fixture =
            create_quad(editor);

        select_fixture_face(
            editor,
            fixture);

        ToolContext context{ editor };
        TestMeshDragTool tool{};

        tool.activate(context);

        tool.handle_event(
            context,
            make_pointer_press(
                glm::vec2{
                    5.0f,
                    5.0f
                }));

        tool.handle_event(
            context,
            make_pointer_move(
                glm::vec2{
                    15.0f,
                    5.0f
                }));

        const ToolResult cancelResult =
            tool.cancel(context);

        ok &= expect(
            cancelResult.code ==
            ToolResultCode::Cancelled,
            "cancel retorna Cancelled");

        ok &= expect(
            tool.state() == ToolState::Ready,
            "cancel retorna tool para Ready");

        ok &= expect(
            !tool.capture().is_active(),
            "cancel limpa captura");

        ok &= expect(
            !tool.mesh_session().is_active(),
            "cancel limpa mesh session");

        ok &= expect(
            tool.commit_count() == 0,
            "cancel nao executa commit");

        ok &= expect(
            !tool.concrete_state_active(),
            "cancel limpa estado concreto");

        ok &= expect(
            has_dirty_flag(
                cancelResult,
                EditorDirtyFlags::Render),
            "cancel marca render dirty");

        return ok;
    }

    bool test_deactivate_while_interacting()
    {
        std::cout
            << "\n=== Deactivate while interacting ===\n";

        bool ok = true;

        Editor editor{};
        editor.set_mode(EditorMode::Mesh);

        const QuadFixture fixture =
            create_quad(editor);

        select_fixture_face(
            editor,
            fixture);

        ToolContext context{ editor };
        TestMeshDragTool tool{};

        tool.activate(context);

        tool.handle_event(
            context,
            make_pointer_press(
                glm::vec2{
                    0.0f,
                    0.0f
                }));

        const ToolResult result =
            tool.deactivate(context);

        ok &= expect(
            result.code ==
            ToolResultCode::Consumed,
            "deactivate conclui com Consumed");

        ok &= expect(
            tool.state() == ToolState::Inactive,
            "deactivate retorna Inactive");

        ok &= expect(
            !tool.capture().is_active(),
            "deactivate limpa captura");

        ok &= expect(
            !tool.mesh_session().is_active(),
            "deactivate limpa mesh session");

        ok &= expect(
            tool.commit_count() == 0,
            "deactivate nao confirma operacao");

        ok &= expect(
            !tool.concrete_state_active(),
            "deactivate limpa estado concreto");

        return ok;
    }

    bool test_begin_failure()
    {
        std::cout << "\n=== Begin failure ===\n";

        bool ok = true;

        Editor editor{};
        editor.set_mode(EditorMode::Mesh);

        const QuadFixture fixture =
            create_quad(editor);

        select_fixture_face(
            editor,
            fixture);

        ToolContext context{ editor };
        TestMeshDragTool tool{};

        tool.set_fail_begin(true);
        tool.activate(context);

        const ToolResult result =
            tool.handle_event(
                context,
                make_pointer_press(
                    glm::vec2{
                        10.0f,
                        10.0f
                    }));

        ok &= expect(
            result.code == ToolResultCode::Failed,
            "falha no begin retorna Failed");

        ok &= expect(
            tool.state() == ToolState::Ready,
            "falha no begin mantem Ready");

        ok &= expect(
            !tool.capture().is_active(),
            "falha no begin nao mantem captura");

        ok &= expect(
            !tool.mesh_session().is_active(),
            "falha no begin limpa mesh session");

        ok &= expect(
            !tool.concrete_state_active(),
            "falha no begin limpa estado concreto");

        ok &= expect(
            tool.commit_count() == 0,
            "falha no begin nao executa commit");

        return ok;
    }

    bool test_preview_construction_failure()
    {
        std::cout
            << "\n=== Preview construction failure ===\n";

        bool ok = true;

        Editor editor{};
        editor.set_mode(EditorMode::Mesh);

        const QuadFixture fixture =
            create_quad(editor);

        select_fixture_face(
            editor,
            fixture);

        ToolContext context{ editor };
        TestMeshDragTool tool{};

        tool.set_null_preview_operation(true);
        tool.activate(context);

        const ToolResult result =
            tool.handle_event(
                context,
                make_pointer_press(
                    glm::vec2{
                        0.0f,
                        0.0f
                    }));

        ok &= expect(
            result.code == ToolResultCode::Failed,
            "preview nulo retorna Failed");

        ok &= expect(
            tool.state() == ToolState::Ready,
            "falha de preview mantem Ready");

        ok &= expect(
            !tool.capture().is_active(),
            "falha de preview nao mantem captura");

        ok &= expect(
            !tool.mesh_session().is_active(),
            "falha de preview limpa mesh session");

        ok &= expect(
            !tool.concrete_state_active(),
            "falha de preview limpa estado concreto");

        return ok;
    }

    bool test_ignored_update()
    {
        std::cout << "\n=== Ignored update ===\n";

        bool ok = true;

        Editor editor{};
        editor.set_mode(EditorMode::Mesh);

        const QuadFixture fixture =
            create_quad(editor);

        select_fixture_face(
            editor,
            fixture);

        ToolContext context{ editor };
        TestMeshDragTool tool{};

        tool.set_ignore_updates(true);
        tool.activate(context);

        tool.handle_event(
            context,
            make_pointer_press(
                glm::vec2{
                    0.0f,
                    0.0f
                }));

        const int previewCountBeforeMove =
            tool.preview_build_count();

        const ToolResult moveResult =
            tool.handle_event(
                context,
                make_pointer_move(
                    glm::vec2{
                        30.0f,
                        0.0f
                    }));

        ok &= expect(
            moveResult.code ==
            ToolResultCode::Ignored,
            "update concreto Ignored permanece Ignored");

        ok &= expect(
            tool.preview_build_count() ==
            previewCountBeforeMove,
            "update Ignored nao reconstrui preview");

        ok &= expect(
            tool.state() ==
            ToolState::Interacting,
            "update Ignored mantem interacao ativa");

        tool.cancel(context);

        return ok;
    }

    bool test_commit_failure()
    {
        std::cout << "\n=== Commit failure ===\n";

        bool ok = true;

        Editor editor{};
        editor.set_mode(EditorMode::Mesh);

        const QuadFixture fixture =
            create_quad(editor);

        select_fixture_face(
            editor,
            fixture);

        ToolContext context{ editor };
        TestMeshDragTool tool{};

        tool.set_fail_commit(true);
        tool.activate(context);

        tool.handle_event(
            context,
            make_pointer_press(
                glm::vec2{
                    0.0f,
                    0.0f
                }));

        tool.handle_event(
            context,
            make_pointer_move(
                glm::vec2{
                    20.0f,
                    0.0f
                }));

        const ToolResult releaseResult =
            tool.handle_event(
                context,
                make_pointer_release(
                    glm::vec2{
                        20.0f,
                        0.0f
                    }));

        ok &= expect(
            releaseResult.code ==
            ToolResultCode::Failed,
            "falha no commit retorna Failed");

        ok &= expect(
            tool.state() ==
            ToolState::Interacting,
            "falha no commit preserva interacao");

        ok &= expect(
            tool.mesh_session().is_active(),
            "falha no commit preserva mesh session");

        ok &= expect(
            tool.commit_count() == 0,
            "commit com falha nao incrementa sucesso");

        ok &= expect(
            tool.concrete_state_active(),
            "falha no commit preserva parametros para retry/cancel");

        const ToolResult cancelResult =
            tool.cancel(context);

        ok &= expect(
            cancelResult.code ==
            ToolResultCode::Cancelled,
            "interacao pode ser cancelada apos falha no commit");

        ok &= expect(
            tool.state() == ToolState::Ready,
            "cancel apos falha retorna Ready");

        return ok;
    }

    bool test_selection_is_captured()
    {
        std::cout << "\n=== Stable captured target ===\n";

        bool ok = true;

        Editor editor{};
        editor.set_mode(EditorMode::Mesh);

        const QuadFixture firstFixture =
            create_quad(
                editor,
                "First Mesh");

        const QuadFixture secondFixture =
            create_quad(
                editor,
                "Second Mesh");

        select_fixture_face(
            editor,
            firstFixture);

        ToolContext context{ editor };
        TestMeshDragTool tool{};

        tool.activate(context);

        tool.handle_event(
            context,
            make_pointer_press(
                glm::vec2{
                    0.0f,
                    0.0f
                }));

        /*
         * Change the editor selection while the operation is active. The
         * captured MeshToolTarget must remain bound to the first node.
         */
        select_fixture_face(
            editor,
            secondFixture);

        tool.handle_event(
            context,
            make_pointer_move(
                glm::vec2{
                    10.0f,
                    0.0f
                }));

        const ToolResult releaseResult =
            tool.handle_event(
                context,
                make_pointer_release(
                    glm::vec2{
                        10.0f,
                        0.0f
                    }));

        ok &= expect(
            releaseResult.code ==
            ToolResultCode::Confirmed,
            "operacao confirma apos mudanca de selecao");

        ok &= expect(
            tool.committed_node() ==
            firstFixture.nodeId,
            "commit usa node capturado no inicio");

        ok &= expect(
            tool.committed_node() !=
            secondFixture.nodeId,
            "mudanca de selecao nao troca alvo ativo");

        return ok;
    }

} // namespace

int main()
{
    std::cout
        << "=== Locus3D Editor MeshDragOperationTool "
        << "Smoke Test ===\n";

    bool ok = true;

    ok &= test_activation_contract();
    ok &= test_press_without_target();
    ok &= test_begin_and_preview();
    ok &= test_confirm_on_release();
    ok &= test_explicit_confirmation_policy();
    ok &= test_cancel();
    ok &= test_deactivate_while_interacting();
    ok &= test_begin_failure();
    ok &= test_preview_construction_failure();
    ok &= test_ignored_update();
    ok &= test_commit_failure();
    ok &= test_selection_is_captured();

    std::cout << "\n=== Resultado final ===\n";

    if (!ok) {
        std::cout
            << "[FAIL] Um ou mais testes do "
            << "MeshDragOperationTool falharam.\n";

        return EXIT_FAILURE;
    }

    std::cout
        << "[OK] Todos os testes do "
        << "MeshDragOperationTool passaram.\n";

    return EXIT_SUCCESS;
}