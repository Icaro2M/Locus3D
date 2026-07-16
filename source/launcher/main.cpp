/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/render/PreviewRenderAdapter.h"
#include "editor/scene/MeshNode.h"
#include "graphics/mesh/GpuMesh.h"
#include "kernel/geometry/render/RenderMesh.h"
#include "kernel/modeling/preview/OperationPreview.h"
#include "kernel/modeling/preview/PreviewMesh.h"

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

    using locus::editor::MeshNode;
    using locus::editor::PreviewRenderAdapter;
    using locus::editor::PreviewRenderObjects;
    using locus::editor::PreviewRenderOptions;
    using locus::editor::PreviewRenderResult;
    using locus::editor::SceneNodeId;

    using locus::graphics::BufferUsage;
    using locus::graphics::GpuMesh;
    using locus::graphics::MeshUploadData;
    using locus::graphics::PrimitiveTopology;
    using locus::graphics::RenderLayer;

    using locus::kernel::geometry::RenderMesh;
    using locus::kernel::modeling::OperationPreview;
    using locus::kernel::modeling::OperationPreviewStatus;
    using locus::kernel::modeling::PreviewMesh;

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
        const float a,
        const float b,
        const float epsilon = 0.0001f)
    {
        return std::abs(a - b) <= epsilon;
    }

    bool nearly_equal(
        const glm::vec3& a,
        const glm::vec3& b,
        const float epsilon = 0.0001f)
    {
        return glm::length(a - b) <= epsilon;
    }

    bool nearly_equal(
        const glm::quat& a,
        const glm::quat& b,
        const float epsilon = 0.0001f)
    {
        const float directDistance =
            glm::length(glm::vec4{
                a.w - b.w,
                a.x - b.x,
                a.y - b.y,
                a.z - b.z
                });

        const float inverseDistance =
            glm::length(glm::vec4{
                a.w + b.w,
                a.x + b.x,
                a.y + b.y,
                a.z + b.z
                });

        /*
         * q and -q represent the same orientation, so either representation is
         * accepted.
         */
        return directDistance <= epsilon ||
            inverseDistance <= epsilon;
    }

    RenderMesh make_solid_quad()
    {
        RenderMesh mesh{};

        const glm::vec3 normal{ 0.0f, 0.0f, 1.0f };

        const auto vertex0 = mesh.add_vertex(
            glm::vec3{ -1.0f, -1.0f, 0.0f },
            normal);

        const auto vertex1 = mesh.add_vertex(
            glm::vec3{ 1.0f, -1.0f, 0.0f },
            normal);

        const auto vertex2 = mesh.add_vertex(
            glm::vec3{ 1.0f, 1.0f, 0.0f },
            normal);

        const auto vertex3 = mesh.add_vertex(
            glm::vec3{ -1.0f, 1.0f, 0.0f },
            normal);

        mesh.add_triangle(vertex0, vertex1, vertex2);
        mesh.add_triangle(vertex0, vertex2, vertex3);

        return mesh;
    }

    RenderMesh make_wire_quad()
    {
        RenderMesh mesh{};

        const auto vertex0 =
            mesh.add_vertex(glm::vec3{ -1.0f, -1.0f, 0.0f });

        const auto vertex1 =
            mesh.add_vertex(glm::vec3{ 1.0f, -1.0f, 0.0f });

        const auto vertex2 =
            mesh.add_vertex(glm::vec3{ 1.0f, 1.0f, 0.0f });

        const auto vertex3 =
            mesh.add_vertex(glm::vec3{ -1.0f, 1.0f, 0.0f });

        mesh.add_line(vertex0, vertex1);
        mesh.add_line(vertex1, vertex2);
        mesh.add_line(vertex2, vertex3);
        mesh.add_line(vertex3, vertex0);

        return mesh;
    }

    OperationPreview make_ready_preview()
    {
        PreviewMesh previewMesh{
            make_solid_quad(),
            make_wire_quad()
        };

        previewMesh.set_message("Quad preview fixture.");

        return OperationPreview::ready(
            std::move(previewMesh));
    }

    bool test_ready_preview_fixture()
    {
        std::cout << "\n=== Ready preview fixture ===\n";

        bool ok = true;

        const OperationPreview preview =
            make_ready_preview();

        ok &= expect(
            preview.status() == OperationPreviewStatus::Ready,
            "fixture produz preview Ready");

        ok &= expect(
            preview.is_ready(),
            "preview pronto pode ser exibido");

        ok &= expect(
            preview.mesh().valid(),
            "payload do preview e valido");

        ok &= expect(
            preview.mesh().solid_vertex_count() == 4,
            "preview solido possui quatro vertices");

        ok &= expect(
            preview.mesh().solid_triangle_count() == 2,
            "preview solido possui dois triangulos");

        ok &= expect(
            preview.mesh().wire_vertex_count() == 4,
            "preview wire possui quatro vertices");

        ok &= expect(
            preview.mesh().wire_line_count() == 4,
            "preview wire possui quatro linhas");

        return ok;
    }

    bool test_solid_upload_data()
    {
        std::cout << "\n=== Solid preview upload ===\n";

        bool ok = true;

        const OperationPreview preview =
            make_ready_preview();

        PreviewRenderOptions options{};

        options.solidUploadOptions.color = {
            0.25f,
            0.50f,
            0.75f,
            0.80f
        };

        options.solidUploadOptions.usage =
            BufferUsage::Dynamic;

        PreviewRenderResult result{};

        const MeshUploadData uploadData =
            PreviewRenderAdapter::build_solid_upload_data(
                preview,
                options,
                &result);

        ok &= expect(
            !uploadData.is_empty(),
            "adapter gera upload solido nao vazio");

        ok &= expect(
            uploadData.vertices.size() == 4,
            "upload solido possui quatro vertices");

        ok &= expect(
            uploadData.indices.size() == 6,
            "upload solido possui seis indices");

        ok &= expect(
            uploadData.topology == PrimitiveTopology::Triangles,
            "upload solido usa topologia Triangles");

        ok &= expect(
            uploadData.usage == BufferUsage::Dynamic,
            "upload solido preserva usage configurado");

        ok &= expect(
            result.previewReady,
            "resultado informa preview pronto");

        ok &= expect(
            result.hasSolidUploadData,
            "resultado informa upload solido disponivel");

        ok &= expect(
            result.solidUploadResult.vertexCount == 4,
            "resultado informa quatro vertices solidos");

        ok &= expect(
            result.solidUploadResult.triangleCount == 2,
            "resultado informa dois triangulos");

        ok &= expect(
            result.solidUploadResult.indexCount == 6,
            "resultado informa seis indices solidos");

        ok &= expect(
            result.solidUploadResult.has_triangles(),
            "resultado reconhece triangulos");

        ok &= expect(
            !result.message.empty(),
            "conversao solida produz diagnostico");

        if (!uploadData.vertices.empty()) {
            const auto& vertex = uploadData.vertices.front();

            ok &= expect(
                nearly_equal(vertex.color[0], 0.25f) &&
                nearly_equal(vertex.color[1], 0.50f) &&
                nearly_equal(vertex.color[2], 0.75f) &&
                nearly_equal(vertex.color[3], 0.80f),
                "upload solido preserva a cor configurada");
        }

        return ok;
    }

    bool test_wire_upload_data()
    {
        std::cout << "\n=== Wire preview upload ===\n";

        bool ok = true;

        const OperationPreview preview =
            make_ready_preview();

        PreviewRenderOptions options{};

        options.wireUploadOptions.color = {
            1.0f,
            0.6f,
            0.1f,
            1.0f
        };

        options.wireUploadOptions.usage =
            BufferUsage::Dynamic;

        PreviewRenderResult result{};

        const MeshUploadData uploadData =
            PreviewRenderAdapter::build_wire_upload_data(
                preview,
                options,
                &result);

        ok &= expect(
            !uploadData.is_empty(),
            "adapter gera upload wire nao vazio");

        ok &= expect(
            uploadData.vertices.size() == 4,
            "upload wire possui quatro vertices");

        ok &= expect(
            uploadData.indices.size() == 8,
            "upload wire possui oito indices");

        ok &= expect(
            uploadData.topology == PrimitiveTopology::Lines,
            "upload wire usa topologia Lines");

        ok &= expect(
            uploadData.usage == BufferUsage::Dynamic,
            "upload wire preserva usage configurado");

        ok &= expect(
            result.previewReady,
            "resultado wire informa preview pronto");

        ok &= expect(
            result.hasWireUploadData,
            "resultado informa upload wire disponivel");

        ok &= expect(
            result.wireUploadResult.vertexCount == 4,
            "resultado informa quatro vertices wire");

        ok &= expect(
            result.wireUploadResult.lineCount == 4,
            "resultado informa quatro linhas");

        ok &= expect(
            result.wireUploadResult.indexCount == 8,
            "resultado informa oito indices wire");

        ok &= expect(
            result.wireUploadResult.has_lines(),
            "resultado reconhece linhas");

        ok &= expect(
            !result.message.empty(),
            "conversao wire produz diagnostico");

        return ok;
    }

    bool test_disabled_parts()
    {
        std::cout << "\n=== Disabled preview parts ===\n";

        bool ok = true;

        const OperationPreview preview =
            make_ready_preview();

        PreviewRenderOptions solidOptions{};
        solidOptions.includeSolid = false;

        PreviewRenderResult solidResult{};

        const MeshUploadData solidUpload =
            PreviewRenderAdapter::build_solid_upload_data(
                preview,
                solidOptions,
                &solidResult);

        ok &= expect(
            solidUpload.is_empty(),
            "solido desabilitado nao gera upload");

        ok &= expect(
            solidResult.skipped,
            "resultado marca solido desabilitado como skipped");

        ok &= expect(
            !solidResult.hasSolidUploadData,
            "solido desabilitado nao informa upload disponivel");

        PreviewRenderOptions wireOptions{};
        wireOptions.includeWire = false;

        PreviewRenderResult wireResult{};

        const MeshUploadData wireUpload =
            PreviewRenderAdapter::build_wire_upload_data(
                preview,
                wireOptions,
                &wireResult);

        ok &= expect(
            wireUpload.is_empty(),
            "wire desabilitado nao gera upload");

        ok &= expect(
            wireResult.skipped,
            "resultado marca wire desabilitado como skipped");

        ok &= expect(
            !wireResult.hasWireUploadData,
            "wire desabilitado nao informa upload disponivel");

        return ok;
    }

    bool test_invalid_preview_states()
    {
        std::cout << "\n=== Invalid preview states ===\n";

        bool ok = true;

        {
            const OperationPreview preview =
                OperationPreview::empty(
                    "No preview geometry.");

            PreviewRenderResult result{};

            const MeshUploadData upload =
                PreviewRenderAdapter::build_solid_upload_data(
                    preview,
                    {},
                    &result);

            ok &= expect(
                upload.is_empty(),
                "preview Empty nao gera upload");

            ok &= expect(
                result.status == OperationPreviewStatus::Empty,
                "resultado preserva status Empty");

            ok &= expect(
                result.skipped,
                "preview Empty e marcado como skipped");

            ok &= expect(
                !result.previewReady,
                "preview Empty nao e marcado como pronto");

            ok &= expect(
                result.message == "No preview geometry.",
                "preview Empty preserva diagnostico");
        }

        {
            const OperationPreview preview =
                OperationPreview::invalidated(
                    "Preview became stale.");

            PreviewRenderResult result{};

            const MeshUploadData upload =
                PreviewRenderAdapter::build_wire_upload_data(
                    preview,
                    {},
                    &result);

            ok &= expect(
                upload.is_empty(),
                "preview Invalidated nao gera upload");

            ok &= expect(
                result.status ==
                OperationPreviewStatus::Invalidated,
                "resultado preserva status Invalidated");

            ok &= expect(
                !result.previewReady,
                "preview Invalidated nao e marcado como pronto");

            ok &= expect(
                result.message == "Preview became stale.",
                "preview Invalidated preserva diagnostico");
        }

        {
            const OperationPreview preview =
                OperationPreview::failed(
                    "Operation failed.");

            PreviewRenderResult result{};

            const MeshUploadData upload =
                PreviewRenderAdapter::build_solid_upload_data(
                    preview,
                    {},
                    &result);

            ok &= expect(
                upload.is_empty(),
                "preview Failed nao gera upload");

            ok &= expect(
                result.status == OperationPreviewStatus::Failed,
                "resultado preserva status Failed");

            ok &= expect(
                !result.previewReady,
                "preview Failed nao e marcado como pronto");

            ok &= expect(
                result.message == "Operation failed.",
                "preview Failed preserva diagnostico");
        }

        {
            PreviewMesh invalidMesh{
                make_solid_quad(),
                make_wire_quad()
            };

            invalidMesh.set_valid(false);
            invalidMesh.set_message(
                "Preview mesh payload is invalid.");

            const OperationPreview preview =
                OperationPreview::ready(
                    std::move(invalidMesh));

            PreviewRenderResult result{};

            const MeshUploadData upload =
                PreviewRenderAdapter::build_solid_upload_data(
                    preview,
                    {},
                    &result);

            ok &= expect(
                upload.is_empty(),
                "payload invalido nao gera upload");

            ok &= expect(
                result.status == OperationPreviewStatus::Ready,
                "payload invalido ainda preserva status Ready");

            ok &= expect(
                !result.previewReady ||
                !result.hasSolidUploadData,
                "payload invalido nao produz dados utilizaveis");

            ok &= expect(
                result.message ==
                "Preview mesh payload is invalid.",
                "payload invalido preserva diagnostico");
        }

        return ok;
    }

    bool test_render_objects()
    {
        std::cout << "\n=== Preview render objects ===\n";

        bool ok = true;

        MeshNode node{
            SceneNodeId{ 42 },
            "Extrude Face"
        };

        const glm::vec3 expectedPosition{
            3.0f,
            -2.0f,
            5.0f
        };

        const glm::quat expectedRotation =
            glm::angleAxis(
                glm::radians(35.0f),
                glm::normalize(
                    glm::vec3{
                        0.0f,
                        1.0f,
                        1.0f
                    }));

        const glm::vec3 expectedScale{
            2.0f,
            0.5f,
            1.5f
        };

        node.transform().set_position(expectedPosition);
        node.transform().set_rotation(expectedRotation);
        node.transform().set_scale(expectedScale);

        const OperationPreview preview =
            make_ready_preview();

        /*
         * The adapter only stores non-owning pointers. These GpuMesh objects do
         * not need uploaded OpenGL resources for this CPU-side smoke test.
         */
        GpuMesh solidMesh{};
        GpuMesh wireMesh{};

        PreviewRenderOptions options{};
        options.solidObjectId = 1001;
        options.wireObjectId = 1002;
        options.solidLayer = RenderLayer::Preview;
        options.wireLayer = RenderLayer::Preview;

        PreviewRenderResult result{};

        const PreviewRenderObjects objects =
            PreviewRenderAdapter::build_render_objects(
                node,
                preview,
                &solidMesh,
                &wireMesh,
                options,
                &result);

        ok &= expect(
            !objects.empty(),
            "adapter gera objetos de preview");

        ok &= expect(
            objects.hasSolid,
            "adapter gera objeto solido");

        ok &= expect(
            objects.hasWire,
            "adapter gera objeto wire");

        ok &= expect(
            result.hasSolidObject,
            "resultado informa objeto solido");

        ok &= expect(
            result.hasWireObject,
            "resultado informa objeto wire");

        ok &= expect(
            result.nodeId.value == node.id().value,
            "resultado preserva SceneNodeId");

        ok &= expect(
            objects.solid.id == 1001,
            "objeto solido preserva id configurado");

        ok &= expect(
            objects.wire.id == 1002,
            "objeto wire preserva id configurado");

        ok &= expect(
            objects.solid.name ==
            "Extrude Face Preview Solid",
            "objeto solido recebe nome semantico");

        ok &= expect(
            objects.wire.name ==
            "Extrude Face Preview Wire",
            "objeto wire recebe nome semantico");

        ok &= expect(
            objects.solid.mesh == &solidMesh,
            "objeto solido referencia GpuMesh fornecida");

        ok &= expect(
            objects.wire.mesh == &wireMesh,
            "objeto wire referencia GpuMesh fornecida");

        ok &= expect(
            objects.solid.layer == RenderLayer::Preview &&
            objects.wire.layer == RenderLayer::Preview,
            "objetos usam camada Preview");

        ok &= expect(
            nearly_equal(
                objects.solid.transform.position,
                expectedPosition) &&
            nearly_equal(
                objects.wire.transform.position,
                expectedPosition),
            "objetos preservam posicao do MeshNode");

        ok &= expect(
            nearly_equal(
                objects.solid.transform.rotation,
                expectedRotation) &&
            nearly_equal(
                objects.wire.transform.rotation,
                expectedRotation),
            "objetos preservam rotacao do MeshNode");

        ok &= expect(
            nearly_equal(
                objects.solid.transform.scale,
                expectedScale) &&
            nearly_equal(
                objects.wire.transform.scale,
                expectedScale),
            "objetos preservam escala do MeshNode");

        ok &= expect(
            objects.solid.visibility.visible &&
            objects.wire.visibility.visible,
            "objetos preservam visibilidade do MeshNode");

        ok &= expect(
            !objects.solid.visibility.selectable &&
            !objects.wire.visibility.selectable,
            "preview nao participa da selecao");

        ok &= expect(
            !objects.solid.visibility.castsShadow &&
            !objects.solid.visibility.receivesShadow &&
            !objects.wire.visibility.castsShadow &&
            !objects.wire.visibility.receivesShadow,
            "preview nao participa de sombras");

        ok &= expect(
            !objects.solid.pickingId.is_valid() &&
            !objects.wire.pickingId.is_valid(),
            "preview nao possui PickingId valido");

        ok &= expect(
            !objects.solid.selected &&
            !objects.solid.hovered &&
            !objects.wire.selected &&
            !objects.wire.hovered,
            "preview nao herda estado de selecao ou hover");

        ok &= expect(
            !objects.solid.wireframe &&
            !objects.wire.wireframe,
            "objetos nao ativam rasterizacao wireframe");

        ok &= expect(
            !result.message.empty(),
            "construcao de objetos produz diagnostico");

        return ok;
    }

    bool test_partial_render_objects()
    {
        std::cout << "\n=== Partial preview render objects ===\n";

        bool ok = true;

        MeshNode node{
            SceneNodeId{ 7 },
            "Partial Preview"
        };

        const OperationPreview preview =
            make_ready_preview();

        GpuMesh solidMesh{};
        GpuMesh wireMesh{};

        {
            PreviewRenderOptions options{};

            PreviewRenderResult result{};

            const PreviewRenderObjects objects =
                PreviewRenderAdapter::build_render_objects(
                    node,
                    preview,
                    &solidMesh,
                    nullptr,
                    options,
                    &result);

            ok &= expect(
                objects.hasSolid && !objects.hasWire,
                "somente GpuMesh solida gera apenas objeto solido");

            ok &= expect(
                result.hasSolidObject &&
                !result.hasWireObject,
                "resultado parcial informa apenas objeto solido");
        }

        {
            PreviewRenderOptions options{};

            PreviewRenderResult result{};

            const PreviewRenderObjects objects =
                PreviewRenderAdapter::build_render_objects(
                    node,
                    preview,
                    nullptr,
                    &wireMesh,
                    options,
                    &result);

            ok &= expect(
                !objects.hasSolid && objects.hasWire,
                "somente GpuMesh wire gera apenas objeto wire");

            ok &= expect(
                !result.hasSolidObject &&
                result.hasWireObject,
                "resultado parcial informa apenas objeto wire");
        }

        {
            PreviewRenderOptions options{};

            PreviewRenderResult result{};

            const PreviewRenderObjects objects =
                PreviewRenderAdapter::build_render_objects(
                    node,
                    preview,
                    nullptr,
                    nullptr,
                    options,
                    &result);

            ok &= expect(
                objects.empty(),
                "ausencia de GpuMesh nao gera objetos");

            ok &= expect(
                result.skipped,
                "ausencia de GpuMesh e marcada como skipped");

            ok &= expect(
                !result.message.empty(),
                "ausencia de GpuMesh produz diagnostico");
        }

        return ok;
    }

    bool test_hidden_source_node()
    {
        std::cout << "\n=== Hidden source node ===\n";

        bool ok = true;

        MeshNode node{
            SceneNodeId{ 88 },
            "Hidden Preview"
        };

        node.metadata().visible = false;

        const OperationPreview preview =
            make_ready_preview();

        GpuMesh solidMesh{};
        GpuMesh wireMesh{};

        const PreviewRenderObjects objects =
            PreviewRenderAdapter::build_render_objects(
                node,
                preview,
                &solidMesh,
                &wireMesh);

        ok &= expect(
            objects.hasSolid && objects.hasWire,
            "node oculto ainda gera descritores de render");

        ok &= expect(
            !objects.solid.visibility.visible &&
            !objects.wire.visibility.visible,
            "objetos de preview preservam node oculto");

        return ok;
    }

    bool test_invalid_node()
    {
        std::cout << "\n=== Invalid source node ===\n";

        bool ok = true;

        MeshNode node{
            SceneNodeId{},
            "Invalid Node"
        };

        const OperationPreview preview =
            make_ready_preview();

        GpuMesh solidMesh{};
        GpuMesh wireMesh{};

        PreviewRenderResult result{};

        const PreviewRenderObjects objects =
            PreviewRenderAdapter::build_render_objects(
                node,
                preview,
                &solidMesh,
                &wireMesh,
                {},
                &result);

        ok &= expect(
            objects.empty(),
            "node com id invalido nao gera objetos");

        ok &= expect(
            !result.hasSolidObject &&
            !result.hasWireObject,
            "resultado de node invalido nao informa objetos");

        ok &= expect(
            !result.message.empty(),
            "node invalido produz diagnostico");

        return ok;
    }

} // namespace

int main()
{
    std::cout
        << "=== Locus3D Editor PreviewRenderAdapter Smoke Test ===\n";

    bool ok = true;

    ok &= test_ready_preview_fixture();
    ok &= test_solid_upload_data();
    ok &= test_wire_upload_data();
    ok &= test_disabled_parts();
    ok &= test_invalid_preview_states();
    ok &= test_render_objects();
    ok &= test_partial_render_objects();
    ok &= test_hidden_source_node();
    ok &= test_invalid_node();

    std::cout << "\n=== Resultado final ===\n";

    if (!ok) {
        std::cout
            << "[FAIL] Um ou mais testes do "
            << "PreviewRenderAdapter falharam.\n";

        return EXIT_FAILURE;
    }

    std::cout
        << "[OK] Todos os testes do "
        << "PreviewRenderAdapter passaram.\n";

    return EXIT_SUCCESS;
}