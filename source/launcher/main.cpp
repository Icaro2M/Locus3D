/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/render/MeshNodeRenderAdapter.h"
#include "editor/scene/MeshNode.h"
#include "graphics/mesh/MeshUploadData.h"
#include "graphics/scene/RenderLayer.h"
#include "kernel/geometry/mesh/LEMEditor.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

    constexpr float Epsilon = 0.0001f;

    bool nearly_equal(float lhs, float rhs, float epsilon = Epsilon)
    {
        return std::abs(lhs - rhs) <= epsilon;
    }

    bool expect(bool condition, const std::string& message)
    {
        if (condition) {
            std::cout << "[OK] " << message << '\n';
            return true;
        }

        std::cout << "[FAIL] " << message << '\n';
        return false;
    }

    bool expect_size(
        std::size_t actual,
        std::size_t expected,
        const std::string& message)
    {
        if (actual == expected) {
            std::cout << "[OK] " << message << " = " << actual << '\n';
            return true;
        }

        std::cout
            << "[FAIL] " << message
            << " | actual=" << actual
            << " expected=" << expected
            << '\n';

        return false;
    }

    bool expect_u64(
        std::uint64_t actual,
        std::uint64_t expected,
        const std::string& message)
    {
        if (actual == expected) {
            std::cout << "[OK] " << message << " = " << actual << '\n';
            return true;
        }

        std::cout
            << "[FAIL] " << message
            << " | actual=" << actual
            << " expected=" << expected
            << '\n';

        return false;
    }

    bool expect_float(
        float actual,
        float expected,
        const std::string& message)
    {
        if (nearly_equal(actual, expected)) {
            std::cout
                << "[OK] " << message
                << " = " << std::fixed << std::setprecision(4) << actual
                << '\n';

            return true;
        }

        std::cout
            << "[FAIL] " << message
            << " | actual=" << std::fixed << std::setprecision(4) << actual
            << " expected=" << expected
            << '\n';

        return false;
    }

    void print_upload_summary(
        const locus::graphics::MeshUploadData& uploadData,
        const locus::editor::MeshNodeRenderResult& result)
    {
        std::cout
            << "MeshUploadData"
            << " | vertices: " << uploadData.vertices.size()
            << " | indices: " << uploadData.indices.size()
            << " | hasUploadData: " << (result.hasUploadData ? "true" : "false")
            << " | hasGpuMesh: " << (result.hasGpuMesh ? "true" : "false")
            << " | hasRenderObject: " << (result.hasRenderObject ? "true" : "false")
            << " | skipped: " << (result.skipped ? "true" : "false")
            << '\n';

        std::cout
            << "UploadResult"
            << " | vertices: " << result.uploadResult.vertexCount
            << " | triangles: " << result.uploadResult.triangleCount
            << " | lines: " << result.uploadResult.lineCount
            << " | indices: " << result.uploadResult.indexCount
            << '\n';

        if (!result.message.empty()) {
            std::cout << "Message: " << result.message << '\n';
        }
    }

    locus::kernel::geometry::FaceHandle make_triangle(
        locus::kernel::geometry::LEMEditor& editor)
    {
        const auto v0 = editor.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
        const auto v1 = editor.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f });
        const auto v2 = editor.add_vertex(glm::vec3{ 0.0f, 1.0f, 0.0f });

        return editor.add_face(std::vector{ v0, v1, v2 });
    }

    locus::kernel::geometry::FaceHandle make_quad(
        locus::kernel::geometry::LEMEditor& editor)
    {
        const auto v0 = editor.add_vertex(glm::vec3{ -1.0f, -1.0f, 0.0f });
        const auto v1 = editor.add_vertex(glm::vec3{ 1.0f, -1.0f, 0.0f });
        const auto v2 = editor.add_vertex(glm::vec3{ 1.0f,  1.0f, 0.0f });
        const auto v3 = editor.add_vertex(glm::vec3{ -1.0f,  1.0f, 0.0f });

        return editor.add_face(std::vector{ v0, v1, v2, v3 });
    }

    bool test_empty_mesh_node_upload()
    {
        using namespace locus;

        std::cout << "\n=== MeshNodeRenderAdapter: mesh node vazio ===\n";

        bool ok = true;

        editor::MeshNode node{ editor::SceneNodeId{ 10 }, "Empty mesh node" };

        editor::MeshNodeRenderResult result{};
        const graphics::MeshUploadData uploadData =
            editor::MeshNodeRenderAdapter::build_upload_data(node, {}, &result);

        print_upload_summary(uploadData, result);

        ok &= expect(uploadData.is_empty(), "upload data vazio");
        ok &= expect(!uploadData.has_indices(), "upload data sem indices");
        ok &= expect(result.nodeId == node.id(), "result.nodeId preservado");
        ok &= expect(!result.hasUploadData, "result.hasUploadData false");
        ok &= expect(!result.hasGpuMesh, "result.hasGpuMesh false");
        ok &= expect(!result.hasRenderObject, "result.hasRenderObject false");
        ok &= expect(result.skipped, "result.skipped true");
        ok &= expect_size(result.uploadResult.vertexCount, 0, "uploadResult.vertexCount");
        ok &= expect_size(result.uploadResult.triangleCount, 0, "uploadResult.triangleCount");
        ok &= expect_size(result.uploadResult.indexCount, 0, "uploadResult.indexCount");
        ok &= expect(!result.message.empty(), "mensagem diagnostica preenchida");

        return ok;
    }

    bool test_triangle_mesh_node_upload()
    {
        using namespace locus;

        std::cout << "\n=== MeshNodeRenderAdapter: mesh node triangulo ===\n";

        bool ok = true;

        editor::MeshNode node{ editor::SceneNodeId{ 20 }, "Triangle mesh node" };
        kernel::geometry::LEMEditor meshEditor{ node.mesh() };

        const auto face = make_triangle(meshEditor);
        ok &= expect(node.mesh().is_valid(face), "face triangular criada");

        editor::MeshNodeRenderOptions options{};
        options.uploadOptions.color = graphics::ColorRGBA{ 0.20f, 0.40f, 0.80f, 1.0f };
        options.uploadOptions.usage = graphics::BufferUsage::Dynamic;

        editor::MeshNodeRenderResult result{};
        const graphics::MeshUploadData uploadData =
            editor::MeshNodeRenderAdapter::build_upload_data(node, options, &result);

        print_upload_summary(uploadData, result);

        ok &= expect(!uploadData.is_empty(), "upload data nao vazio");
        ok &= expect(uploadData.has_indices(), "upload data com indices");
        ok &= expect(uploadData.topology == graphics::PrimitiveTopology::Triangles, "topologia Triangles");
        ok &= expect(uploadData.usage == graphics::BufferUsage::Dynamic, "usage Dynamic");
        ok &= expect(result.nodeId == node.id(), "result.nodeId preservado");
        ok &= expect(result.hasUploadData, "result.hasUploadData true");
        ok &= expect(!result.hasGpuMesh, "result.hasGpuMesh false");
        ok &= expect(!result.hasRenderObject, "result.hasRenderObject false");
        ok &= expect(!result.skipped, "result.skipped false");
        ok &= expect_size(uploadData.vertices.size(), 3, "upload vertices");
        ok &= expect_size(uploadData.indices.size(), 3, "upload indices");
        ok &= expect_size(result.uploadResult.vertexCount, 3, "uploadResult.vertexCount");
        ok &= expect_size(result.uploadResult.triangleCount, 1, "uploadResult.triangleCount");
        ok &= expect_size(result.uploadResult.indexCount, 3, "uploadResult.indexCount");

        ok &= expect_float(uploadData.vertices[0].color[0], 0.20f, "color.r");
        ok &= expect_float(uploadData.vertices[0].color[1], 0.40f, "color.g");
        ok &= expect_float(uploadData.vertices[0].color[2], 0.80f, "color.b");
        ok &= expect_float(uploadData.vertices[0].color[3], 1.00f, "color.a");

        return ok;
    }

    bool test_quad_mesh_node_upload()
    {
        using namespace locus;

        std::cout << "\n=== MeshNodeRenderAdapter: mesh node quad ===\n";

        bool ok = true;

        editor::MeshNode node{ editor::SceneNodeId{ 30 }, "Quad mesh node" };
        kernel::geometry::LEMEditor meshEditor{ node.mesh() };

        const auto face = make_quad(meshEditor);
        ok &= expect(node.mesh().is_valid(face), "face quad criada");

        editor::MeshNodeRenderResult result{};
        const graphics::MeshUploadData uploadData =
            editor::MeshNodeRenderAdapter::build_upload_data(node, {}, &result);

        print_upload_summary(uploadData, result);

        ok &= expect(!uploadData.is_empty(), "upload data nao vazio");
        ok &= expect(uploadData.has_indices(), "upload data com indices");
        ok &= expect_size(uploadData.vertices.size(), 4, "upload vertices");
        ok &= expect_size(uploadData.indices.size(), 6, "upload indices");
        ok &= expect_size(result.uploadResult.vertexCount, 4, "uploadResult.vertexCount");
        ok &= expect_size(result.uploadResult.triangleCount, 2, "uploadResult.triangleCount");
        ok &= expect_size(result.uploadResult.indexCount, 6, "uploadResult.indexCount");

        return ok;
    }

    bool test_build_render_object_metadata()
    {
        using namespace locus;

        std::cout << "\n=== MeshNodeRenderAdapter: RenderObject metadata/flags ===\n";

        bool ok = true;

        editor::MeshNode node{ editor::SceneNodeId{ 40 }, "Renderable mesh node" };

        node.transform().set_position(glm::vec3{ 3.0f, 4.0f, 5.0f });
        node.transform().set_scale(glm::vec3{ 2.0f, 3.0f, 4.0f });
        node.transform().set_rotation(glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f });

        node.metadata().visible = true;
        node.metadata().selectable = true;
        node.metadata().locked = false;

        editor::MeshNodeRenderOptions options{};
        options.layer = graphics::RenderLayer::Default;
        options.selected = true;
        options.hovered = true;
        options.wireframe = true;

        const graphics::RenderObject object =
            editor::MeshNodeRenderAdapter::build_render_object(node, nullptr, options);

        std::cout
            << "RenderObject"
            << " | id: " << object.id
            << " | name: " << object.name
            << " | visible: " << (object.visibility.visible ? "true" : "false")
            << " | selectable: " << (object.visibility.selectable ? "true" : "false")
            << " | selected: " << (object.selected ? "true" : "false")
            << " | hovered: " << (object.hovered ? "true" : "false")
            << " | wireframe: " << (object.wireframe ? "true" : "false")
            << '\n';

        ok &= expect_u64(object.id, 40, "object.id");
        ok &= expect(object.name == "Renderable mesh node", "object.name preservado");
        ok &= expect(object.mesh == nullptr, "object.mesh pode ser nullptr no teste sem GPU");
        ok &= expect(object.shader == nullptr, "object.shader nullptr");
        ok &= expect(object.material == nullptr, "object.material nullptr");
        ok &= expect(object.layer == graphics::RenderLayer::Default, "object.layer Default");
        ok &= expect(object.selected, "object.selected true");
        ok &= expect(object.hovered, "object.hovered true");
        ok &= expect(object.wireframe, "object.wireframe true");

        ok &= expect(object.visibility.visible, "visibility.visible true");
        ok &= expect(object.visibility.selectable, "visibility.selectable true");
        ok &= expect(!object.visibility.castsShadow, "visibility.castsShadow false");
        ok &= expect(!object.visibility.receivesShadow, "visibility.receivesShadow false");

        ok &= expect_float(object.transform.position.x, 3.0f, "transform.position.x");
        ok &= expect_float(object.transform.position.y, 4.0f, "transform.position.y");
        ok &= expect_float(object.transform.position.z, 5.0f, "transform.position.z");

        ok &= expect_float(object.transform.scale.x, 2.0f, "transform.scale.x");
        ok &= expect_float(object.transform.scale.y, 3.0f, "transform.scale.y");
        ok &= expect_float(object.transform.scale.z, 4.0f, "transform.scale.z");

        return ok;
    }

    bool test_build_render_object_visibility_rules()
    {
        using namespace locus;

        std::cout << "\n=== MeshNodeRenderAdapter: RenderObject visibility/selectability ===\n";

        bool ok = true;

        editor::MeshNode lockedNode{ editor::SceneNodeId{ 50 }, "Locked mesh node" };
        lockedNode.metadata().visible = true;
        lockedNode.metadata().selectable = true;
        lockedNode.metadata().locked = true;

        const graphics::RenderObject lockedObject =
            editor::MeshNodeRenderAdapter::build_render_object(lockedNode, nullptr, {});

        ok &= expect(lockedObject.visibility.visible, "lockedObject visible true");
        ok &= expect(!lockedObject.visibility.selectable, "lockedObject selectable false por locked");

        editor::MeshNode hiddenNode{ editor::SceneNodeId{ 51 }, "Hidden mesh node" };
        hiddenNode.metadata().visible = false;
        hiddenNode.metadata().selectable = true;
        hiddenNode.metadata().locked = false;

        const graphics::RenderObject hiddenObject =
            editor::MeshNodeRenderAdapter::build_render_object(hiddenNode, nullptr, {});

        ok &= expect(!hiddenObject.visibility.visible, "hiddenObject visible false");
        ok &= expect(!hiddenObject.visibility.selectable, "hiddenObject selectable false por invisivel");

        editor::MeshNode nonSelectableNode{ editor::SceneNodeId{ 52 }, "Non selectable mesh node" };
        nonSelectableNode.metadata().visible = true;
        nonSelectableNode.metadata().selectable = false;
        nonSelectableNode.metadata().locked = false;

        const graphics::RenderObject nonSelectableObject =
            editor::MeshNodeRenderAdapter::build_render_object(nonSelectableNode, nullptr, {});

        ok &= expect(nonSelectableObject.visibility.visible, "nonSelectableObject visible true");
        ok &= expect(!nonSelectableObject.visibility.selectable, "nonSelectableObject selectable false");

        return ok;
    }

    bool test_cache_key()
    {
        using namespace locus;

        std::cout << "\n=== MeshNodeRenderAdapter: cache key ===\n";

        bool ok = true;

        editor::MeshNode node{ editor::SceneNodeId{ 60 }, "Cache key mesh node" };

        const graphics::MeshRenderCacheKey key =
            editor::MeshNodeRenderAdapter::build_cache_key(node, 7);

        std::cout
            << "MeshRenderCacheKey"
            << " | ownerId: " << key.ownerId
            << " | revision: " << key.revision
            << " | valid: " << (key.is_valid() ? "true" : "false")
            << '\n';

        ok &= expect(key.is_valid(), "cache key valida");
        ok &= expect_u64(key.ownerId, 60, "cache key ownerId");
        ok &= expect_u64(key.revision, 7, "cache key revision");

        editor::MeshNode invalidNode{ editor::SceneNodeId{}, "Invalid id mesh node" };

        const graphics::MeshRenderCacheKey invalidKey =
            editor::MeshNodeRenderAdapter::build_cache_key(invalidNode, 1);

        std::cout
            << "Invalid MeshRenderCacheKey"
            << " | ownerId: " << invalidKey.ownerId
            << " | revision: " << invalidKey.revision
            << " | valid: " << (invalidKey.is_valid() ? "true" : "false")
            << '\n';

        /*
         * Este teste só documenta o comportamento atual do adapter:
         * SceneNodeId inválido tem valor max uint64, então a chave ainda passa em
         * MeshRenderCacheKey::is_valid(), que só exige ownerId != 0.
         *
         * A validação forte contra node inválido acontece em build_cached_render_object(),
         * antes de usar a chave no cache.
         */
        ok &= expect(invalidKey.is_valid(), "invalidKey documenta regra atual de MeshRenderCacheKey");
        ok &= expect_u64(invalidKey.revision, 1, "invalidKey revision preservada");

        return ok;
    }

} // namespace

int main()
{
    std::cout << "=== Locus3D Editor MeshNodeRenderAdapter Smoke Test ===\n";

    bool ok = true;

    ok &= test_empty_mesh_node_upload();
    ok &= test_triangle_mesh_node_upload();
    ok &= test_quad_mesh_node_upload();
    ok &= test_build_render_object_metadata();
    ok &= test_build_render_object_visibility_rules();
    ok &= test_cache_key();

    std::cout << "\n=== Resultado final ===\n";

    if (ok) {
        std::cout << "[OK] Todos os testes de MeshNodeRenderAdapter passaram.\n";
        return EXIT_SUCCESS;
    }

    std::cout << "[FAIL] Algum teste de MeshNodeRenderAdapter falhou.\n";
    return EXIT_FAILURE;
}