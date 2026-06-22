#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace locus::kernel::geometry;

namespace {

    int g_failed = 0;
    int g_passed = 0;

    void check(bool condition, const std::string& message)
    {
        if (condition) {
            ++g_passed;
            std::cout << "[OK] " << message << '\n';
            return;
        }

        ++g_failed;
        std::cout << "[FAIL] " << message << '\n';
    }

    bool near(float a, float b, float epsilon = 0.0001f)
    {
        return std::abs(a - b) <= epsilon;
    }

    bool near_vec3(const glm::vec3& a, const glm::vec3& b, float epsilon = 0.0001f)
    {
        return near(a.x, b.x, epsilon)
            && near(a.y, b.y, epsilon)
            && near(a.z, b.z, epsilon);
    }

    void print_section(const std::string& title)
    {
        std::cout << "\n=== " << title << " ===\n";
    }

    struct QuadFixture {
        LEM mesh;
        LEMEditor editor;
        VertexHandle v0;
        VertexHandle v1;
        VertexHandle v2;
        VertexHandle v3;
        EdgeHandle e01;
        EdgeHandle e12;
        EdgeHandle e23;
        EdgeHandle e30;
        FaceHandle face;

        QuadFixture()
            : mesh()
            , editor(mesh)
        {
            v0 = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
            v1 = editor.add_vertex({ 1.0f, 0.0f, 0.0f });
            v2 = editor.add_vertex({ 1.0f, 1.0f, 0.0f });
            v3 = editor.add_vertex({ 0.0f, 1.0f, 0.0f });

            face = editor.add_face({ v0, v1, v2, v3 });

            e01 = mesh.find_edge(v0, v1);
            e12 = mesh.find_edge(v1, v2);
            e23 = mesh.find_edge(v2, v3);
            e30 = mesh.find_edge(v3, v0);
        }
    };

    void test_facade_accessors()
    {
        print_section("LEMEditor facade: accessors e diff");

        QuadFixture fixture;

        check(&fixture.editor.mesh() == &fixture.mesh, "mesh() retorna a malha editada");
        check(&fixture.editor.topology().mesh() == &fixture.mesh, "topology() aponta para a mesma malha");
        check(&fixture.editor.geometry().mesh() == &fixture.mesh, "geometry() aponta para a mesma malha");
        check(&fixture.editor.attributes().mesh() == &fixture.mesh, "attributes() aponta para a mesma malha");

        check(!fixture.editor.diff().empty(), "diff registra criacao inicial");

        const std::size_t beforeTake = fixture.editor.diff().size();
        LEMDiff diff = fixture.editor.take_diff();

        check(diff.size() == beforeTake, "take_diff retorna os eventos acumulados");
        check(fixture.editor.diff().empty(), "take_diff limpa o diff interno");

        fixture.editor.set_selected(fixture.v0, true);
        check(!fixture.editor.diff().empty(), "diff volta a registrar novos eventos depois de take_diff");

        fixture.editor.clear_diff();
        check(fixture.editor.diff().empty(), "clear_diff limpa eventos acumulados");
    }

    void test_topology_passthrough()
    {
        print_section("LEMEditor facade: topology passthrough");

        QuadFixture fixture;

        check(fixture.mesh.vertex_count() == 4, "quad possui 4 vertices");
        check(fixture.mesh.edge_count() == 4, "quad possui 4 edges");
        check(fixture.mesh.loop_count() == 4, "quad possui 4 loops");
        check(fixture.mesh.face_count() == 1, "quad possui 1 face");
        check(fixture.mesh.is_valid(fixture.face), "face criada pela fachada e valida");
        check(fixture.mesh.is_valid(fixture.e01), "edge recuperada apos add_face e valida");

        std::optional<VertexHandle> splitVertex = fixture.editor.split_edge_at_param(fixture.e01, 0.25f);

        check(splitVertex.has_value(), "split_edge_at_param exposto pela fachada cria vertice");
        check(splitVertex.has_value() && fixture.mesh.is_valid(*splitVertex), "vertice criado no split e valido");

        if (splitVertex.has_value()) {
            check(
                near_vec3(fixture.mesh.vertex(*splitVertex).position, { 0.25f, 0.0f, 0.0f }),
                "split_edge_at_param posiciona vertice no parametro esperado");
        }

        QuadFixture splitFaceFixture;
        std::optional<EdgeHandle> diagonal = splitFaceFixture.editor.split_face(
            splitFaceFixture.face,
            splitFaceFixture.v0,
            splitFaceFixture.v2);

        check(diagonal.has_value(), "split_face exposto pela fachada cria diagonal");
        check(diagonal.has_value() && splitFaceFixture.mesh.is_valid(*diagonal), "diagonal criada e valida");

        QuadFixture flipFixture;
        const glm::vec3 normalBefore = flipFixture.mesh.face(flipFixture.face).normal;
        check(flipFixture.editor.flip_face(flipFixture.face), "flip_face exposto pela fachada executa");
        const glm::vec3 normalAfter = flipFixture.mesh.face(flipFixture.face).normal;
        check(glm::dot(normalBefore, normalAfter) < 0.0f, "flip_face inverte normal da face");

        check(flipFixture.editor.flip_all_faces() == 1, "flip_all_faces exposto pela fachada retorna quantidade correta");

        QuadFixture dissolveFixture;
        check(dissolveFixture.editor.dissolve_face(dissolveFixture.face), "dissolve_face exposto pela fachada executa");
        check(!dissolveFixture.mesh.is_valid(dissolveFixture.face), "face dissolvida deixa de ser valida");

        LEM looseMesh;
        LEMEditor looseEditor(looseMesh);
        VertexHandle a = looseEditor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle b = looseEditor.add_vertex({ 1.0f, 0.0f, 0.0f });
        EdgeHandle looseEdge = looseEditor.find_or_create_edge(a, b);

        check(looseMesh.is_valid(looseEdge), "find_or_create_edge exposto pela fachada cria edge solta");
        check(looseEditor.remove_edge_if_loose(looseEdge), "remove_edge_if_loose remove edge sem loops");
        check(!looseMesh.is_valid(looseEdge), "edge solta removida deixa de ser valida");
        check(looseEditor.remove_vertex_if_loose(a), "remove_vertex_if_loose remove vertice sem edges e loops");
        check(!looseMesh.is_valid(a), "vertice solto removido deixa de ser valido");
    }

    void test_geometry_passthrough()
    {
        print_section("LEMEditor facade: geometry passthrough");

        QuadFixture fixture;

        check(
            fixture.editor.set_vertex_position(fixture.v0, { 0.0f, 0.0f, 1.0f }),
            "set_vertex_position exposto pela fachada executa");

        check(
            near_vec3(fixture.mesh.vertex(fixture.v0).position, { 0.0f, 0.0f, 1.0f }),
            "set_vertex_position altera posicao");

        check(
            fixture.editor.translate_vertex(fixture.v0, { 1.0f, 0.0f, 0.0f }),
            "translate_vertex exposto pela fachada executa");

        check(
            near_vec3(fixture.mesh.vertex(fixture.v0).position, { 1.0f, 0.0f, 1.0f }),
            "translate_vertex altera posicao");

        check(
            fixture.editor.set_vertex_position_lerp(fixture.v1, { 3.0f, 0.0f, 0.0f }, 0.5f),
            "set_vertex_position_lerp exposto pela fachada executa");

        check(
            near_vec3(fixture.mesh.vertex(fixture.v1).position, { 2.0f, 0.0f, 0.0f }),
            "set_vertex_position_lerp aplica interpolacao esperada");

        std::size_t translated = fixture.editor.translate_vertices(
            { fixture.v1, fixture.v2, fixture.v3 },
            { 0.0f, 2.0f, 0.0f });

        check(translated == 3, "translate_vertices retorna quantidade de vertices alterados");
        check(
            near_vec3(fixture.mesh.vertex(fixture.v2).position, { 1.0f, 3.0f, 0.0f }),
            "translate_vertices altera posicao de vertice em lote");

        glm::mat4 transform{ 1.0f };
        transform = glm::translate(transform, glm::vec3{ 0.0f, 0.0f, 2.0f });

        std::size_t transformed = fixture.editor.transform_vertices(
            { fixture.v1, fixture.v2 },
            transform);

        check(transformed == 2, "transform_vertices retorna quantidade de vertices alterados");
        check(
            near_vec3(fixture.mesh.vertex(fixture.v2).position, { 1.0f, 3.0f, 2.0f }),
            "transform_vertices aplica matriz");

        QuadFixture normalFixture;
        normalFixture.editor.rebuild_face_normals();

        const glm::vec3 originalNormal = normalFixture.mesh.face(normalFixture.face).normal;
        check(
            near_vec3(originalNormal, { 0.0f, 0.0f, 1.0f }),
            "rebuild_face_normals gera normal esperada para quad XY");

        check(
            normalFixture.editor.offset_vertex_along_normal(normalFixture.v0, 0.5f),
            "offset_vertex_along_normal exposto pela fachada executa");

        check(
            near_vec3(normalFixture.mesh.vertex(normalFixture.v0).position, { 0.0f, 0.0f, 0.5f }),
            "offset_vertex_along_normal move vertice pela normal media");

        check(
            normalFixture.editor.rebuild_normals_around_face(normalFixture.face),
            "rebuild_normals_around_face exposto pela fachada executa");

        normalFixture.editor.rebuild_normals_around_vertex(normalFixture.v0);
        check(!normalFixture.editor.diff().empty(), "rebuild_normals_around_vertex registra eventos no diff");
    }

    void test_attribute_passthrough()
    {
        print_section("LEMEditor facade: attributes passthrough");

        QuadFixture fixture;

        check(fixture.editor.set_selected(fixture.v0, true), "set_selected(vertex) exposto pela fachada executa");
        check(fixture.editor.set_selected(fixture.e01, true), "set_selected(edge) exposto pela fachada executa");
        check(fixture.editor.set_selected(fixture.face, true), "set_selected(face) exposto pela fachada executa");

        check(fixture.mesh.vertex(fixture.v0).selected, "vertex selected alterado");
        check(fixture.mesh.edge(fixture.e01).selected, "edge selected alterado");
        check(fixture.mesh.face(fixture.face).selected, "face selected alterado");

        fixture.editor.clear_selection();

        check(!fixture.mesh.vertex(fixture.v0).selected, "clear_selection limpa vertex selected");
        check(!fixture.mesh.edge(fixture.e01).selected, "clear_selection limpa edge selected");
        check(!fixture.mesh.face(fixture.face).selected, "clear_selection limpa face selected");

        check(fixture.editor.set_hidden(fixture.v1, true), "set_hidden(vertex) exposto pela fachada executa");
        check(fixture.editor.set_hidden(fixture.e12, true), "set_hidden(edge) exposto pela fachada executa");
        check(fixture.editor.set_hidden(fixture.face, true), "set_hidden(face) exposto pela fachada executa");

        check(fixture.mesh.vertex(fixture.v1).hidden, "vertex hidden alterado");
        check(fixture.mesh.edge(fixture.e12).hidden, "edge hidden alterado");
        check(fixture.mesh.face(fixture.face).hidden, "face hidden alterado");

        fixture.editor.clear_visibility();

        check(!fixture.mesh.vertex(fixture.v1).hidden, "clear_visibility limpa vertex hidden");
        check(!fixture.mesh.edge(fixture.e12).hidden, "clear_visibility limpa edge hidden");
        check(!fixture.mesh.face(fixture.face).hidden, "clear_visibility limpa face hidden");

        check(fixture.editor.set_smooth(fixture.e23, true), "set_smooth exposto pela fachada executa");
        check(fixture.mesh.edge(fixture.e23).smooth, "set_smooth altera edge.smooth");

        check(fixture.editor.set_crease(fixture.e23, 1.5f), "set_crease exposto pela fachada executa");
        check(near(fixture.mesh.edge(fixture.e23).crease, 1.0f), "set_crease clampa valor acima de 1");

        check(fixture.editor.set_crease(fixture.e23, -2.0f), "set_crease aceita valor abaixo de 0");
        check(near(fixture.mesh.edge(fixture.e23).crease, 0.0f), "set_crease clampa valor abaixo de 0");

        check(fixture.editor.set_tag(fixture.v2, 11), "set_tag(vertex) exposto pela fachada executa");
        check(fixture.editor.set_tag(fixture.e30, 22), "set_tag(edge) exposto pela fachada executa");
        check(fixture.editor.set_tag(fixture.face, 33), "set_tag(face) exposto pela fachada executa");

        check(fixture.mesh.vertex(fixture.v2).tag == 11, "vertex tag alterada");
        check(fixture.mesh.edge(fixture.e30).tag == 22, "edge tag alterada");
        check(fixture.mesh.face(fixture.face).tag == 33, "face tag alterada");

        fixture.editor.clear_tags();

        check(fixture.mesh.vertex(fixture.v2).tag == 0, "clear_tags limpa vertex tag");
        check(fixture.mesh.edge(fixture.e30).tag == 0, "clear_tags limpa edge tag");
        check(fixture.mesh.face(fixture.face).tag == 0, "clear_tags limpa face tag");
    }

    void test_clear()
    {
        print_section("LEMEditor facade: clear");

        QuadFixture fixture;

        fixture.editor.clear();

        check(fixture.mesh.empty(), "clear limpa a malha");
        check(!fixture.editor.diff().empty(), "clear registra evento no diff");
    }

}

int main()
{
    std::cout << "=== Locus3D LEMEditor Facade Regression Test ===\n";

    test_facade_accessors();
    test_topology_passthrough();
    test_geometry_passthrough();
    test_attribute_passthrough();
    test_clear();

    std::cout << "\n=== Resultado ===\n";
    std::cout << "Passou: " << g_passed << '\n';
    std::cout << "Falhou: " << g_failed << '\n';

    if (g_failed != 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}