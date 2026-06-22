#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <glm/geometric.hpp>
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

    void print_section(const std::string& title)
    {
        std::cout << "\n=== " << title << " ===\n";
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

    std::size_t active_face_count(const LEM& mesh)
    {
        return TopologyTraversal::faces(mesh).size();
    }

    std::size_t active_vertex_count(const LEM& mesh)
    {
        return TopologyTraversal::vertices(mesh).size();
    }

    std::size_t active_edge_count(const LEM& mesh)
    {
        return TopologyTraversal::edges(mesh).size();
    }

    void test_merge_loose_vertices()
    {
        print_section("merge_vertices: vertices soltos sem edge");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle target = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle source = editor.add_vertex({ 1.0f, 0.0f, 0.0f });

        editor.clear_diff();

        check(mesh.is_valid(target), "target valido antes do merge");
        check(mesh.is_valid(source), "source valido antes do merge");
        check(active_vertex_count(mesh) == 2, "malha possui 2 vertices ativos antes do merge");

        check(editor.merge_vertices(source, target), "merge_vertices executa em vertices soltos");
        check(mesh.is_valid(target), "target continua valido apos merge");
        check(!mesh.is_valid(source), "source deixa de ser valido apos merge");
        check(active_vertex_count(mesh) == 1, "malha possui 1 vertice ativo apos merge");
        check(near_vec3(mesh.vertex(target).position, { 0.0f, 0.0f, 0.0f }), "merge_vertices preserva posicao do target");
        check(!editor.diff().empty(), "merge_vertices registra eventos no diff");
    }

    void test_merge_vertices_at_position()
    {
        print_section("merge_vertices_at_position: posicao final explicita");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle target = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle source = editor.add_vertex({ 2.0f, 0.0f, 0.0f });

        editor.clear_diff();

        check(
            editor.merge_vertices_at_position(source, target, { 1.0f, 2.0f, 3.0f }),
            "merge_vertices_at_position executa");

        check(mesh.is_valid(target), "target continua valido");
        check(!mesh.is_valid(source), "source foi removido");
        check(
            near_vec3(mesh.vertex(target).position, { 1.0f, 2.0f, 3.0f }),
            "target recebe posicao final explicita");
        check(active_vertex_count(mesh) == 1, "apenas 1 vertice ativo permanece");
    }

    void test_merge_connected_vertices()
    {
        print_section("merge_vertices: vertices conectados por edge");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle target = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle source = editor.add_vertex({ 1.0f, 0.0f, 0.0f });
        EdgeHandle edge = editor.find_or_create_edge(target, source);

        editor.clear_diff();

        check(mesh.is_valid(edge), "edge solta criada entre target e source");
        check(active_edge_count(mesh) == 1, "malha possui 1 edge antes do merge");

        check(editor.merge_vertices(source, target), "merge_vertices executa mesmo com edge entre vertices");
        check(mesh.is_valid(target), "target continua valido");
        check(!mesh.is_valid(source), "source deixa de ser valido");
        check(!mesh.is_valid(edge), "edge entre source e target foi removida");
        check(active_edge_count(mesh) == 0, "malha fica sem edges ativas apos merge");
    }

    void test_merge_degenerate_face()
    {
        print_section("merge_vertices: face degenerada removida");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle v0 = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle v1 = editor.add_vertex({ 1.0f, 0.0f, 0.0f });
        VertexHandle v2 = editor.add_vertex({ 0.0f, 1.0f, 0.0f });

        FaceHandle face = editor.add_face({ v0, v1, v2 });

        editor.clear_diff();

        check(mesh.is_valid(face), "triangulo valido antes do merge");
        check(active_face_count(mesh) == 1, "malha possui 1 face antes do merge");

        check(editor.merge_vertices(v1, v0), "merge_vertices executa em vertices da mesma face");
        check(mesh.is_valid(v0), "target continua valido");
        check(!mesh.is_valid(v1), "source deixa de ser valido");
        check(!mesh.is_valid(face), "face original degenerada deixa de ser valida");
        check(active_face_count(mesh) == 0, "face degenerada nao foi recriada");
    }

    void test_merge_rebuilds_non_degenerate_face()
    {
        print_section("merge_vertices: face afetada reconstruida");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle target = editor.add_vertex({ 0.0f, 0.0f, 0.0f });

        VertexHandle source = editor.add_vertex({ 3.0f, 0.0f, 0.0f });
        VertexHandle a = editor.add_vertex({ 4.0f, 0.0f, 0.0f });
        VertexHandle b = editor.add_vertex({ 3.5f, 1.0f, 0.0f });

        FaceHandle oldFace = editor.add_face({ source, a, b });

        editor.clear_diff();

        check(mesh.is_valid(oldFace), "face original valida antes do merge");
        check(active_face_count(mesh) == 1, "malha possui 1 face antes do merge");

        check(editor.merge_vertices(source, target), "merge_vertices executa com face nao degenerada");
        check(mesh.is_valid(target), "target continua valido");
        check(!mesh.is_valid(source), "source removido");
        check(!mesh.is_valid(oldFace), "face antiga foi removida");
        check(active_face_count(mesh) == 1, "face nao degenerada foi reconstruida");

        std::vector<FaceHandle> targetFaces = TopologyTraversal::vertex_faces(mesh, target);

        check(targetFaces.size() == 1, "target passa a pertencer a face reconstruida");

        if (!targetFaces.empty()) {
            std::vector<VertexHandle> vertices = TopologyTraversal::face_vertices(mesh, targetFaces.front());

            bool hasTarget = false;
            bool hasA = false;
            bool hasB = false;
            bool hasSource = false;

            for (VertexHandle vertex : vertices) {
                if (vertex == target) {
                    hasTarget = true;
                }
                if (vertex == a) {
                    hasA = true;
                }
                if (vertex == b) {
                    hasB = true;
                }
                if (vertex == source) {
                    hasSource = true;
                }
            }

            check(vertices.size() == 3, "face reconstruida continua triangular");
            check(hasTarget, "face reconstruida usa target");
            check(hasA, "face reconstruida preserva vertice A");
            check(hasB, "face reconstruida preserva vertice B");
            check(!hasSource, "face reconstruida nao usa source removido");
        }
    }

    void test_weld_vertices_restricted()
    {
        print_section("weld_vertices: conjunto restrito");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle a = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle b = editor.add_vertex({ 0.0005f, 0.0f, 0.0f });
        VertexHandle c = editor.add_vertex({ 10.0f, 0.0f, 0.0f });
        VertexHandle d = editor.add_vertex({ 0.0004f, 0.0f, 0.0f });

        editor.clear_diff();

        std::size_t merged = editor.weld_vertices({ a, b, c }, 0.001f);

        check(merged == 1, "weld_vertices funde apenas vertices dentro da lista");
        check(mesh.is_valid(a), "primeiro vertice da lista continua valido como target");
        check(!mesh.is_valid(b), "vertice proximo dentro da lista foi fundido");
        check(mesh.is_valid(c), "vertice distante dentro da lista continua valido");
        check(mesh.is_valid(d), "vertice proximo fora da lista nao foi afetado");
        check(active_vertex_count(mesh) == 3, "malha possui 3 vertices ativos apos weld restrito");
    }

    void test_merge_vertices_by_distance()
    {
        print_section("merge_vertices_by_distance: malha inteira");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle a0 = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle a1 = editor.add_vertex({ 0.05f, 0.0f, 0.0f });

        VertexHandle b0 = editor.add_vertex({ 10.0f, 0.0f, 0.0f });
        VertexHandle b1 = editor.add_vertex({ 10.05f, 0.0f, 0.0f });

        VertexHandle c0 = editor.add_vertex({ 20.0f, 0.0f, 0.0f });

        editor.clear_diff();

        std::size_t merged = editor.merge_vertices_by_distance(0.1f);

        check(merged == 2, "merge_vertices_by_distance funde dois pares proximos");
        check(mesh.is_valid(a0), "a0 continua valido");
        check(!mesh.is_valid(a1), "a1 foi fundido em a0");
        check(mesh.is_valid(b0), "b0 continua valido");
        check(!mesh.is_valid(b1), "b1 foi fundido em b0");
        check(mesh.is_valid(c0), "c0 distante continua valido");
        check(active_vertex_count(mesh) == 3, "malha possui 3 vertices ativos apos merge por distancia");
        check(near_vec3(mesh.vertex(a0).position, { 0.025f, 0.0f, 0.0f }), "a0 recebe media do primeiro par");
        check(near_vec3(mesh.vertex(b0).position, { 10.025f, 0.0f, 0.0f }), "b0 recebe media do segundo par");
    }

    void test_merge_invalid_inputs()
    {
        print_section("merge/weld: entradas invalidas");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle valid = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle invalid{};

        editor.clear_diff();

        check(!editor.merge_vertices(invalid, valid), "merge_vertices rejeita source invalido");
        check(!editor.merge_vertices(valid, invalid), "merge_vertices rejeita target invalido");
        check(!editor.merge_vertices(valid, valid), "merge_vertices rejeita source igual ao target");
        check(!editor.merge_vertices_at_position(valid, valid, { 1.0f, 0.0f, 0.0f }), "merge_vertices_at_position rejeita source igual ao target");
        check(editor.merge_vertices_by_distance(-1.0f) == 0, "merge_vertices_by_distance rejeita distancia negativa");
        check(editor.weld_vertices({ valid }, -1.0f) == 0, "weld_vertices rejeita distancia negativa");
        check(editor.weld_vertices({ valid }, 0.1f) == 0, "weld_vertices com apenas um vertice nao funde nada");
        check(mesh.is_valid(valid), "vertice valido permanece ativo apos entradas invalidas");
    }

    void test_merge_with_zero_distance()
    {
        print_section("merge_vertices_by_distance: distancia zero");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle a = editor.add_vertex({ 1.0f, 2.0f, 3.0f });
        VertexHandle b = editor.add_vertex({ 1.0f, 2.0f, 3.0f });
        VertexHandle c = editor.add_vertex({ 1.0f, 2.0f, 3.001f });

        editor.clear_diff();

        std::size_t merged = editor.merge_vertices_by_distance(0.0f);

        check(merged == 1, "distancia zero funde apenas vertices coincidentes");
        check(mesh.is_valid(a), "vertice coincidente target continua valido");
        check(!mesh.is_valid(b), "vertice coincidente source removido");
        check(mesh.is_valid(c), "vertice quase coincidente permanece valido");
        check(active_vertex_count(mesh) == 2, "malha possui 2 vertices ativos apos merge com distancia zero");
    }

}

int main()
{
    std::cout << "=== Locus3D LEM Merge/Weld Regression Test ===\n";

    test_merge_loose_vertices();
    test_merge_vertices_at_position();
    test_merge_connected_vertices();
    test_merge_degenerate_face();
    test_merge_rebuilds_non_degenerate_face();
    test_weld_vertices_restricted();
    test_merge_vertices_by_distance();
    test_merge_invalid_inputs();
    test_merge_with_zero_distance();

    std::cout << "\n=== Resultado ===\n";
    std::cout << "Passou: " << g_passed << '\n';
    std::cout << "Falhou: " << g_failed << '\n';

    if (g_failed != 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}