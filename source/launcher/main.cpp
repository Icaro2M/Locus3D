/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"
#include "editor/scene/MeshNode.h"
#include "editor/tools/core/ToolContext.h"
#include "editor/tools/core/ToolResult.h"
#include "editor/tools/mesh/core/MeshOperationSession.h"
#include "editor/tools/mesh/core/MeshToolTarget.h"
#include "kernel/common/Error.h"
#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/modeling/core/IOperation.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/core/OperationResult.h"

#include <glm/geometric.hpp>

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

    using locus::editor::Editor;
    using locus::editor::EditorDirtyFlags;
    using locus::editor::MeshNode;
    using locus::editor::MeshOperationSession;
    using locus::editor::MeshOperationSessionState;
    using locus::editor::MeshToolTarget;
    using locus::editor::SceneNodeId;
    using locus::editor::SelectionGranularity;
    using locus::editor::ToolContext;
    using locus::editor::ToolResult;
    using locus::editor::ToolResultCode;

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
        const glm::vec3& a,
        const glm::vec3& b,
        const float epsilon = 0.0001f)
    {
        return glm::length(a - b) <= epsilon;
    }

    QuadFixture create_quad(
        Editor& editor,
        const std::string& name = "Mesh Operation Fixture")
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

    MeshToolTarget make_face_target(
        Editor& editor,
        const QuadFixture& fixture)
    {
        auto& selection =
            editor.selection().mesh();

        selection.set_active_mesh(fixture.nodeId);
        selection.set_face(fixture.face);

        return MeshToolTarget::capture(
            selection,
            SelectionGranularity::Face);
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
            LEM& mesh = context.editable_mesh();

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

    class NoChangeOperation final : public IOperation {
    public:
        [[nodiscard]]
        std::string_view name() const override
        {
            return "NoChangeOperation";
        }

    private:
        [[nodiscard]]
        OperationResult execute_impl(
            OperationContext&) override
        {
            return OperationResult::no_change(
                "Operation intentionally produced no changes.");
        }
    };

    class CancelledOperation final : public IOperation {
    public:
        [[nodiscard]]
        std::string_view name() const override
        {
            return "CancelledOperation";
        }

    private:
        [[nodiscard]]
        OperationResult execute_impl(
            OperationContext&) override
        {
            return OperationResult::cancelled(
                "Operation preview was cancelled.");
        }
    };

    class FailedOperation final : public IOperation {
    public:
        [[nodiscard]]
        std::string_view name() const override
        {
            return "FailedOperation";
        }

    private:
        [[nodiscard]]
        OperationResult execute_impl(
            OperationContext&) override
        {
            return OperationResult::fail(
                ErrorCode::InvalidState,
                "Intentional preview failure.");
        }
    };

    bool test_mesh_tool_target()
    {
        std::cout << "\n=== MeshToolTarget ===\n";

        bool ok = true;

        Editor editor{};

        const QuadFixture fixture =
            create_quad(editor);

        const MeshToolTarget target =
            make_face_target(editor, fixture);

        ok &= expect(
            target.has_node(),
            "target captura SceneNodeId valido");

        ok &= expect(
            target.nodeId == fixture.nodeId,
            "target preserva node id");

        ok &= expect(
            target.granularity ==
            SelectionGranularity::Face,
            "target preserva granularidade Face");

        ok &= expect(
            target.targets_faces(),
            "target reconhece alvo de faces");

        ok &= expect(
            !target.targets_vertices() &&
            !target.targets_edges() &&
            !target.targets_loops(),
            "target nao confunde tipos de componentes");

        ok &= expect(
            target.component_count() == 1,
            "target captura uma face");

        ok &= expect(
            target.faces.size() == 1 &&
            target.faces.front() == fixture.face,
            "target preserva handle da face");

        ok &= expect(
            target.vertices.empty() &&
            target.edges.empty() &&
            target.loops.empty(),
            "capture guarda apenas componentes da granularidade pedida");

        ok &= expect(
            target.is_valid(),
            "target capturado e estruturalmente valido");

        MeshToolTarget emptyTarget =
            MeshToolTarget::none();

        ok &= expect(
            !emptyTarget.has_node(),
            "none retorna target sem node");

        ok &= expect(
            emptyTarget.empty(),
            "none retorna target vazio");

        ok &= expect(
            !emptyTarget.is_valid(),
            "none retorna target invalido");

        MeshToolTarget objectTarget{};

        objectTarget.nodeId = fixture.nodeId;
        objectTarget.granularity =
            SelectionGranularity::Object;

        ok &= expect(
            !objectTarget.has_component_granularity(),
            "granularidade Object nao e componente de malha");

        ok &= expect(
            !objectTarget.is_valid(),
            "target Object nao e valido para ferramenta de malha");

        MeshToolTarget clearTarget = target;
        clearTarget.clear();

        ok &= expect(
            !clearTarget.has_node() &&
            clearTarget.empty() &&
            !clearTarget.is_valid(),
            "clear remove todos os dados do target");

        return ok;
    }

    bool test_session_begin()
    {
        std::cout << "\n=== MeshOperationSession begin ===\n";

        bool ok = true;

        Editor editor{};
        ToolContext context{ editor };

        const QuadFixture fixture =
            create_quad(editor);

        MeshOperationSession session{};

        ok &= expect(
            session.state() ==
            MeshOperationSessionState::Inactive,
            "sessao comeca Inactive");

        ok &= expect(
            !session.is_active(),
            "sessao inicialmente nao esta ativa");

        ok &= expect(
            !session.has_ready_preview(),
            "sessao inicialmente nao possui preview");

        const ToolResult result =
            session.begin(
                context,
                make_face_target(editor, fixture));

        ok &= expect(
            result.code == ToolResultCode::Started,
            "begin retorna Started");

        ok &= expect(
            !result.failed(),
            "begin valido nao falha");

        ok &= expect(
            session.state() ==
            MeshOperationSessionState::Active,
            "begin move sessao para Active");

        ok &= expect(
            session.is_active(),
            "sessao fica ativa depois de begin");

        ok &= expect(
            !session.has_ready_preview(),
            "begin ainda nao gera preview pronto");

        ok &= expect(
            session.preview().is_invalidated(),
            "preview inicial fica Invalidated");

        ok &= expect(
            session.target().nodeId == fixture.nodeId,
            "sessao preserva node alvo");

        ok &= expect(
            session.target().targets_faces(),
            "sessao preserva alvo de faces");

        const ToolResult duplicateBegin =
            session.begin(
                context,
                make_face_target(editor, fixture));

        ok &= expect(
            duplicateBegin.code == ToolResultCode::Failed,
            "segundo begin falha enquanto sessao esta ativa");

        return ok;
    }

    bool test_ready_preview()
    {
        std::cout << "\n=== Ready non-destructive preview ===\n";

        bool ok = true;

        Editor editor{};
        ToolContext context{ editor };

        const QuadFixture fixture =
            create_quad(editor);

        MeshNode* node =
            editor.scene().find_mesh(fixture.nodeId);

        if (!node) {
            return expect(
                false,
                "fixture possui MeshNode");
        }

        const glm::vec3 originalPosition =
            static_cast<const MeshNode&>(*node)
            .mesh()
            .vertex(fixture.vertex0)
            .position;

        MeshOperationSession session{};

        const ToolResult beginResult =
            session.begin(
                context,
                make_face_target(editor, fixture));

        ok &= expect(
            beginResult.code == ToolResultCode::Started,
            "sessao inicia antes do preview");

        MoveVertexOperation operation{
            fixture.vertex0,
            glm::vec3{
                0.0f,
                0.0f,
                2.0f
            }
        };

        const ToolResult previewResult =
            session.rebuild_preview(
                context,
                operation);

        ok &= expect(
            previewResult.code == ToolResultCode::Updated,
            "rebuild_preview retorna Updated");

        ok &= expect(
            previewResult.dirtyFlags ==
            EditorDirtyFlags::Render,
            "preview marca render como dirty");

        ok &= expect(
            session.state() ==
            MeshOperationSessionState::PreviewReady,
            "preview valido move sessao para PreviewReady");

        ok &= expect(
            session.has_ready_preview(),
            "sessao informa preview pronto");

        ok &= expect(
            session.preview().is_ready(),
            "OperationPreview possui status Ready");

        ok &= expect(
            session.preview().mesh().valid(),
            "preview contem payload valido");

        ok &= expect(
            !session.preview().mesh().solid_mesh().empty(),
            "preview contem geometria solida");

        ok &= expect(
            !session.preview().mesh().wire_mesh().empty(),
            "preview contem geometria wire");

        ok &= expect(
            session.preview().mesh().diff().size() == 1,
            "preview preserva diff da operacao");

        const glm::vec3 authoritativePosition =
            static_cast<const MeshNode&>(*node)
            .mesh()
            .vertex(fixture.vertex0)
            .position;

        ok &= expect(
            nearly_equal(
                authoritativePosition,
                originalPosition),
            "preview nao altera a LEM autoritativa");

        return ok;
    }

    bool test_no_change_and_cancelled_preview()
    {
        std::cout
            << "\n=== Empty and invalidated previews ===\n";

        bool ok = true;

        Editor editor{};
        ToolContext context{ editor };

        const QuadFixture fixture =
            create_quad(editor);

        MeshOperationSession session{};

        session.begin(
            context,
            make_face_target(editor, fixture));

        NoChangeOperation noChangeOperation{};

        const ToolResult noChangeResult =
            session.rebuild_preview(
                context,
                noChangeOperation);

        ok &= expect(
            noChangeResult.code == ToolResultCode::Updated,
            "operacao NoChange retorna Updated");

        ok &= expect(
            session.state() ==
            MeshOperationSessionState::Active,
            "NoChange mantem sessao Active");

        ok &= expect(
            session.preview().is_empty(),
            "NoChange produz OperationPreview Empty");

        ok &= expect(
            !session.has_ready_preview(),
            "NoChange nao produz preview pronto");

        CancelledOperation cancelledOperation{};

        const ToolResult cancelledResult =
            session.rebuild_preview(
                context,
                cancelledOperation);

        ok &= expect(
            cancelledResult.code == ToolResultCode::Updated,
            "preview cancelado retorna Updated");

        ok &= expect(
            session.state() ==
            MeshOperationSessionState::Active,
            "preview cancelado mantem sessao Active");

        ok &= expect(
            session.preview().is_invalidated(),
            "operacao Cancelled produz preview Invalidated");

        return ok;
    }

    bool test_failed_preview()
    {
        std::cout << "\n=== Failed preview ===\n";

        bool ok = true;

        Editor editor{};
        ToolContext context{ editor };

        const QuadFixture fixture =
            create_quad(editor);

        MeshOperationSession session{};

        session.begin(
            context,
            make_face_target(editor, fixture));

        FailedOperation operation{};

        const ToolResult result =
            session.rebuild_preview(
                context,
                operation);

        ok &= expect(
            result.code == ToolResultCode::Failed,
            "falha da operacao retorna ToolResult Failed");

        ok &= expect(
            result.dirtyFlags ==
            EditorDirtyFlags::Render,
            "falha limpa ou atualiza preview renderizado");

        ok &= expect(
            session.state() ==
            MeshOperationSessionState::Failed,
            "falha move sessao para Failed");

        ok &= expect(
            session.failed(),
            "failed reconhece estado de falha");

        ok &= expect(
            session.preview().is_failure(),
            "sessao preserva OperationPreview Failed");

        ok &= expect(
            result.message ==
            "Intentional preview failure.",
            "falha preserva diagnostico do kernel");

        return ok;
    }

    bool test_preview_options_and_invalidation()
    {
        std::cout
            << "\n=== Preview options and invalidation ===\n";

        bool ok = true;

        Editor editor{};
        ToolContext context{ editor };

        const QuadFixture fixture =
            create_quad(editor);

        MeshOperationSession session{};

        session.begin(
            context,
            make_face_target(editor, fixture));

        MoveVertexOperation operation{
            fixture.vertex0,
            glm::vec3{
                0.0f,
                0.0f,
                1.0f
            }
        };

        session.rebuild_preview(
            context,
            operation);

        auto options =
            session.preview_options();

        options.buildWireframe = false;
        options.validateAfterPreview = true;

        session.set_preview_options(options);

        ok &= expect(
            session.state() ==
            MeshOperationSessionState::Active,
            "alterar opcoes invalida preview pronto");

        ok &= expect(
            session.preview().is_invalidated(),
            "set_preview_options produz preview Invalidated");

        ok &= expect(
            !session.preview_options().buildWireframe,
            "sessao preserva buildWireframe false");

        ok &= expect(
            session.preview_options().validateAfterPreview,
            "sessao preserva validateAfterPreview true");

        const ToolResult rebuildResult =
            session.rebuild_preview(
                context,
                operation);

        ok &= expect(
            rebuildResult.code == ToolResultCode::Updated,
            "preview pode ser reconstruido apos mudar opcoes");

        ok &= expect(
            session.has_ready_preview(),
            "rebuild apos opcoes produz preview pronto");

        ok &= expect(
            session.preview().mesh().wire_mesh().empty(),
            "buildWireframe false omite geometria wire");

        const ToolResult invalidateResult =
            session.invalidate_preview(
                "Parameters changed.");

        ok &= expect(
            invalidateResult.code == ToolResultCode::Updated,
            "invalidate_preview retorna Updated");

        ok &= expect(
            invalidateResult.dirtyFlags ==
            EditorDirtyFlags::Render,
            "invalidate_preview marca render dirty");

        ok &= expect(
            session.state() ==
            MeshOperationSessionState::Active,
            "invalidate_preview retorna sessao para Active");

        ok &= expect(
            session.preview().is_invalidated(),
            "invalidate_preview altera status do preview");

        ok &= expect(
            session.preview().message() ==
            "Parameters changed.",
            "invalidate_preview preserva mensagem");

        return ok;
    }

    bool test_invalid_targets()
    {
        std::cout << "\n=== Invalid targets ===\n";

        bool ok = true;

        {
            Editor editor{};
            ToolContext context{ editor };

            MeshOperationSession session{};

            const ToolResult result =
                session.begin(
                    context,
                    MeshToolTarget::none());

            ok &= expect(
                result.code == ToolResultCode::Failed,
                "begin rejeita target vazio");

            ok &= expect(
                !session.is_active(),
                "target vazio nao ativa sessao");
        }

        {
            Editor editor{};
            ToolContext context{ editor };

            const QuadFixture fixture =
                create_quad(editor);

            MeshToolTarget target =
                make_face_target(editor, fixture);

            editor.scene().remove_node(
                fixture.nodeId);

            MeshOperationSession session{};

            const ToolResult result =
                session.begin(
                    context,
                    std::move(target));

            ok &= expect(
                result.code == ToolResultCode::Failed,
                "begin rejeita node removido");

            ok &= expect(
                !session.is_active(),
                "node removido nao deixa sessao ativa");
        }

        {
            Editor editor{};
            ToolContext context{ editor };

            const QuadFixture fixture =
                create_quad(editor);

            MeshToolTarget target =
                make_face_target(editor, fixture);

            MeshNode* node =
                editor.scene().find_mesh(
                    fixture.nodeId);

            if (node) {
                node->mesh().clear();
            }

            MeshOperationSession session{};

            const ToolResult result =
                session.begin(
                    context,
                    std::move(target));

            ok &= expect(
                result.code == ToolResultCode::Failed,
                "begin rejeita handle que nao existe mais na LEM");

            ok &= expect(
                !session.is_active(),
                "handle invalido nao deixa sessao ativa");
        }

        return ok;
    }

    bool test_target_invalidation_during_session()
    {
        std::cout
            << "\n=== Target invalidation during session ===\n";

        bool ok = true;

        Editor editor{};
        ToolContext context{ editor };

        const QuadFixture fixture =
            create_quad(editor);

        MeshOperationSession session{};

        session.begin(
            context,
            make_face_target(editor, fixture));

        editor.scene().remove_node(
            fixture.nodeId);

        MoveVertexOperation operation{
            fixture.vertex0,
            glm::vec3{
                0.0f,
                0.0f,
                1.0f
            }
        };

        const ToolResult result =
            session.rebuild_preview(
                context,
                operation);

        ok &= expect(
            result.code == ToolResultCode::Failed,
            "rebuild falha quando node e removido");

        ok &= expect(
            session.state() ==
            MeshOperationSessionState::Failed,
            "node removido move sessao para Failed");

        ok &= expect(
            session.preview().is_failure(),
            "node removido produz preview Failed");

        return ok;
    }

    bool test_cancel_and_clear()
    {
        std::cout << "\n=== Cancel and clear ===\n";

        bool ok = true;

        Editor editor{};
        ToolContext context{ editor };

        const QuadFixture fixture =
            create_quad(editor);

        MeshOperationSession session{};

        session.begin(
            context,
            make_face_target(editor, fixture));

        MoveVertexOperation operation{
            fixture.vertex0,
            glm::vec3{
                0.0f,
                0.0f,
                1.0f
            }
        };

        session.rebuild_preview(
            context,
            operation);

        const ToolResult cancelResult =
            session.cancel(
                "User cancelled mesh operation.");

        ok &= expect(
            cancelResult.code ==
            ToolResultCode::Cancelled,
            "cancel retorna Cancelled");

        ok &= expect(
            cancelResult.dirtyFlags ==
            EditorDirtyFlags::Render,
            "cancel marca render dirty");

        ok &= expect(
            cancelResult.message ==
            "User cancelled mesh operation.",
            "cancel preserva diagnostico");

        ok &= expect(
            session.state() ==
            MeshOperationSessionState::Inactive,
            "cancel retorna sessao para Inactive");

        ok &= expect(
            !session.is_active(),
            "cancel desativa sessao");

        ok &= expect(
            !session.target().has_node(),
            "cancel limpa target");

        ok &= expect(
            !session.has_ready_preview(),
            "cancel descarta preview pronto");

        const ToolResult secondCancel =
            session.cancel();

        ok &= expect(
            secondCancel.code ==
            ToolResultCode::Ignored,
            "cancel inativo retorna Ignored");

        session.begin(
            context,
            make_face_target(editor, fixture));

        session.clear();

        ok &= expect(
            session.state() ==
            MeshOperationSessionState::Inactive &&
            !session.target().has_node(),
            "clear limpa sessao sem ToolResult");

        return ok;
    }

    bool test_rebuild_without_session()
    {
        std::cout
            << "\n=== Rebuild without active session ===\n";

        bool ok = true;

        Editor editor{};
        ToolContext context{ editor };

        const QuadFixture fixture =
            create_quad(editor);

        MeshOperationSession session{};

        MoveVertexOperation operation{
            fixture.vertex0,
            glm::vec3{
                0.0f,
                0.0f,
                1.0f
            }
        };

        const ToolResult result =
            session.rebuild_preview(
                context,
                operation);

        ok &= expect(
            result.code == ToolResultCode::Failed,
            "rebuild sem begin retorna Failed");

        ok &= expect(
            session.state() ==
            MeshOperationSessionState::Inactive,
            "rebuild sem begin mantem sessao Inactive");

        const ToolResult invalidateResult =
            session.invalidate_preview();

        ok &= expect(
            invalidateResult.code ==
            ToolResultCode::Ignored,
            "invalidate sem sessao retorna Ignored");

        return ok;
    }

} // namespace

int main()
{
    std::cout
        << "=== Locus3D Editor Mesh Operation Session "
        << "Smoke Test ===\n";

    bool ok = true;

    ok &= test_mesh_tool_target();
    ok &= test_session_begin();
    ok &= test_ready_preview();
    ok &= test_no_change_and_cancelled_preview();
    ok &= test_failed_preview();
    ok &= test_preview_options_and_invalidation();
    ok &= test_invalid_targets();
    ok &= test_target_invalidation_during_session();
    ok &= test_cancel_and_clear();
    ok &= test_rebuild_without_session();

    std::cout << "\n=== Resultado final ===\n";

    if (!ok) {
        std::cout
            << "[FAIL] Um ou mais testes de "
            << "MeshToolTarget/MeshOperationSession falharam.\n";

        return EXIT_FAILURE;
    }

    std::cout
        << "[OK] Todos os testes de "
        << "MeshToolTarget/MeshOperationSession passaram.\n";

    return EXIT_SUCCESS;
}