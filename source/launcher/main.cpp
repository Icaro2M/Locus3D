#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/mesh/editing/TopologyEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <glm/vec3.hpp>

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

using namespace locus::kernel::geometry;

namespace {

    int g_passed = 0;
    int g_failed = 0;

    void check(bool condition, const std::string& message)
    {
        if (condition) {
            std::cout << "[OK] " << message << '\n';
            ++g_passed;
        }
        else {
            std::cout << "[FAIL] " << message << '\n';
            ++g_failed;
        }
    }

    void section(const std::string& title)
    {
        std::cout << "\n=== " << title << " ===\n";
    }

    void print_counts(const LEM& mesh)
    {
        std::cout
            << "vertices: " << mesh.vertex_count()
            << " | edges: " << mesh.edge_count()
            << " | loops: " << mesh.loop_count()
            << " | faces: " << mesh.face_count()
            << '\n';
    }

    std::vector<VertexHandle> make_quad_vertices(TopologyEditor& editor)
    {
        std::vector<VertexHandle> vertices;

        vertices.push_back(editor.add_vertex(glm::vec3(0.0f, 0.0f, 0.0f)));
        vertices.push_back(editor.add_vertex(glm::vec3(1.0f, 0.0f, 0.0f)));
        vertices.push_back(editor.add_vertex(glm::vec3(1.0f, 1.0f, 0.0f)));
        vertices.push_back(editor.add_vertex(glm::vec3(0.0f, 1.0f, 0.0f)));

        return vertices;
    }

    std::vector<VertexHandle> make_triangle_vertices(TopologyEditor& editor)
    {
        std::vector<VertexHandle> vertices;

        vertices.push_back(editor.add_vertex(glm::vec3(0.0f, 0.0f, 0.0f)));
        vertices.push_back(editor.add_vertex(glm::vec3(1.0f, 0.0f, 0.0f)));
        vertices.push_back(editor.add_vertex(glm::vec3(0.0f, 1.0f, 0.0f)));

        return vertices;
    }

    EdgeHandle find_required_edge(const LEM& mesh, VertexHandle a, VertexHandle b)
    {
        return mesh.find_edge(a, b);
    }

    void test_creation()
    {
        section("Criação");

        LEM mesh;
        LEMDiff diff;
        TopologyEditor editor(mesh, diff);

        check(mesh.empty(), "malha começa vazia");

        VertexHandle v0 = editor.add_vertex(glm::vec3(0.0f, 0.0f, 0.0f));
        VertexHandle v1 = editor.add_vertex(glm::vec3(1.0f, 0.0f, 0.0f));
        VertexHandle v2 = editor.add_vertex(glm::vec3(1.0f, 1.0f, 0.0f));
        VertexHandle v3 = editor.add_vertex(glm::vec3(0.0f, 1.0f, 0.0f));

        check(mesh.is_valid(v0), "v0 válido");
        check(mesh.is_valid(v1), "v1 válido");
        check(mesh.is_valid(v2), "v2 válido");
        check(mesh.is_valid(v3), "v3 válido");
        check(mesh.vertex_count() == 4, "4 vértices criados");

        EdgeHandle e01 = editor.find_or_create_edge(v0, v1);
        EdgeHandle e01Again = editor.find_or_create_edge(v0, v1);

        check(mesh.is_valid(e01), "edge v0-v1 criada");
        check(e01 == e01Again, "find_or_create_edge reutiliza edge existente");

        FaceHandle face = editor.add_face({ v0, v1, v2, v3 });

        check(mesh.is_valid(face), "face quad criada");
        check(mesh.face_count() == 1, "1 face armazenada");
        check(mesh.loop_count() == 4, "4 loops criados para quad");
        check(mesh.face_loops(face).size() == 4, "face possui ciclo com 4 loops");
        check(TopologyTraversal::face_vertices(mesh, face).size() == 4, "face_vertices retorna 4 vértices");
        check(diff.size() > 0, "diff registrou alterações");

        print_counts(mesh);
    }

    void test_remove_face_and_loose_cleanup()
    {
        section("Remoção segura: remove_face, remove_edge_if_loose, remove_vertex_if_loose");

        LEM mesh;
        LEMDiff diff;
        TopologyEditor editor(mesh, diff);

        std::vector<VertexHandle> vertices = make_quad_vertices(editor);
        FaceHandle face = editor.add_face(vertices);

        std::vector<EdgeHandle> edges = TopologyTraversal::face_edges(mesh, face);

        check(mesh.is_valid(face), "quad criado");
        check(edges.size() == 4, "quad possui 4 edges antes da remoção");

        bool removedFace = editor.remove_face(face);

        check(removedFace, "remove_face retornou true");
        check(!mesh.is_valid(face), "face removida ficou inválida");

        int removedEdges = 0;

        for (EdgeHandle edge : edges) {
            if (editor.remove_edge_if_loose(edge)) {
                ++removedEdges;
            }
        }

        check(removedEdges == 4, "4 edges loose removidas");

        int removedVertices = 0;

        for (VertexHandle vertex : vertices) {
            if (editor.remove_vertex_if_loose(vertex)) {
                ++removedVertices;
            }
        }

        check(removedVertices == 4, "4 vértices loose removidos");

        print_counts(mesh);
    }

    void test_kill_loop()
    {
        section("Remoção bruta: kill_loop");

        LEM mesh;
        LEMDiff diff;
        TopologyEditor editor(mesh, diff);

        std::vector<VertexHandle> vertices = make_quad_vertices(editor);
        FaceHandle face = editor.add_face(vertices);

        std::vector<LoopHandle> loops = mesh.face_loops(face);

        check(loops.size() == 4, "quad possui 4 loops antes do kill_loop");

        bool killed = editor.kill_loop(loops[0]);

        check(killed, "kill_loop retornou true");
        check(!mesh.is_valid(loops[0]), "loop morto ficou inválido");
        check(mesh.face_loops(face).size() == 3, "face ficou com 3 loops restantes");

        print_counts(mesh);
    }

    void test_kill_face_only()
    {
        section("Remoção bruta: kill_face_only");

        LEM mesh;
        LEMDiff diff;
        TopologyEditor editor(mesh, diff);

        std::vector<VertexHandle> vertices = make_quad_vertices(editor);
        FaceHandle face = editor.add_face(vertices);

        check(mesh.is_valid(face), "face válida antes do kill_face_only");

        bool killed = editor.kill_face_only(face);

        check(killed, "kill_face_only retornou true");
        check(!mesh.is_valid(face), "face morta ficou inválida");

        print_counts(mesh);
    }

    void test_kill_edge_only()
    {
        section("Remoção bruta: kill_edge_only");

        LEM mesh;
        LEMDiff diff;
        TopologyEditor editor(mesh, diff);

        VertexHandle a = editor.add_vertex(glm::vec3(0.0f, 0.0f, 0.0f));
        VertexHandle b = editor.add_vertex(glm::vec3(1.0f, 0.0f, 0.0f));
        EdgeHandle edge = editor.find_or_create_edge(a, b);

        check(mesh.is_valid(edge), "edge válida antes do kill_edge_only");

        bool killed = editor.kill_edge_only(edge);

        check(killed, "kill_edge_only retornou true");
        check(!mesh.is_valid(edge), "edge morta ficou inválida");
        check(mesh.is_valid(a), "vértice A continua válido");
        check(mesh.is_valid(b), "vértice B continua válido");

        print_counts(mesh);
    }

    void test_split_edge_loose()
    {
        section("Split: split_edge em edge loose");

        LEM mesh;
        LEMDiff diff;
        TopologyEditor editor(mesh, diff);

        VertexHandle a = editor.add_vertex(glm::vec3(0.0f, 0.0f, 0.0f));
        VertexHandle b = editor.add_vertex(glm::vec3(2.0f, 0.0f, 0.0f));
        EdgeHandle edge = editor.find_or_create_edge(a, b);

        std::optional<VertexHandle> mid = editor.split_edge(edge);

        check(mid.has_value(), "split_edge retornou novo vértice");
        check(mid.has_value() && mesh.is_valid(mid.value()), "novo vértice válido");
        check(!mesh.is_valid(edge), "edge original removida");
        check(mesh.is_valid(mesh.find_edge(a, mid.value())), "edge A-mid criada");
        check(mesh.is_valid(mesh.find_edge(mid.value(), b)), "edge mid-B criada");

        if (mid.has_value()) {
            const glm::vec3 position = mesh.vertex(mid.value()).position;
            check(position.x == 1.0f && position.y == 0.0f && position.z == 0.0f, "split_edge criou ponto médio correto");
        }

        print_counts(mesh);
    }

    void test_split_edge_at_param_on_face()
    {
        section("Split: split_edge_at_param em face");

        LEM mesh;
        LEMDiff diff;
        TopologyEditor editor(mesh, diff);

        std::vector<VertexHandle> vertices = make_quad_vertices(editor);
        FaceHandle face = editor.add_face(vertices);

        EdgeHandle edge = find_required_edge(mesh, vertices[0], vertices[1]);

        std::optional<VertexHandle> splitVertex = editor.split_edge_at_param(edge, 0.25f);

        check(splitVertex.has_value(), "split_edge_at_param retornou novo vértice");
        check(splitVertex.has_value() && mesh.is_valid(splitVertex.value()), "novo vértice válido");
        check(!mesh.is_valid(face), "face original foi reconstruída/removida");

        if (splitVertex.has_value()) {
            check(mesh.is_valid(mesh.find_edge(vertices[0], splitVertex.value())), "edge v0-split criada");
            check(mesh.is_valid(mesh.find_edge(splitVertex.value(), vertices[1])), "edge split-v1 criada");

            const glm::vec3 position = mesh.vertex(splitVertex.value()).position;
            check(position.x == 0.25f && position.y == 0.0f && position.z == 0.0f, "posição paramétrica correta");
        }

        bool foundFiveVertexFace = false;

        for (FaceHandle activeFace : TopologyTraversal::faces(mesh)) {
            if (TopologyTraversal::face_vertices(mesh, activeFace).size() == 5) {
                foundFiveVertexFace = true;
                break;
            }
        }

        check(foundFiveVertexFace, "face reconstruída possui 5 vértices");

        print_counts(mesh);
    }

    void test_split_face()
    {
        section("Split: split_face");

        LEM mesh;
        LEMDiff diff;
        TopologyEditor editor(mesh, diff);

        std::vector<VertexHandle> vertices = make_quad_vertices(editor);
        FaceHandle face = editor.add_face(vertices);

        std::optional<EdgeHandle> diagonal = editor.split_face(face, vertices[0], vertices[2]);

        check(diagonal.has_value(), "split_face retornou diagonal");
        check(diagonal.has_value() && mesh.is_valid(diagonal.value()), "diagonal válida");
        check(!mesh.is_valid(face), "face original removida");
        check(mesh.is_valid(mesh.find_edge(vertices[0], vertices[2])), "edge diagonal existe");

        int triangleCount = 0;

        for (FaceHandle activeFace : TopologyTraversal::faces(mesh)) {
            if (TopologyTraversal::face_vertices(mesh, activeFace).size() == 3) {
                ++triangleCount;
            }
        }

        check(triangleCount == 2, "split_face gerou 2 triângulos");

        print_counts(mesh);
    }

    void test_flip_face_and_all_faces()
    {
        section("Flip: flip_face e flip_all_faces");

        LEM mesh;
        LEMDiff diff;
        TopologyEditor editor(mesh, diff);

        std::vector<VertexHandle> vertices = make_quad_vertices(editor);
        FaceHandle face = editor.add_face(vertices);

        std::vector<VertexHandle> before = TopologyTraversal::face_vertices(mesh, face);

        bool flipped = editor.flip_face(face);

        check(flipped, "flip_face retornou true");

        std::vector<VertexHandle> after = TopologyTraversal::face_vertices(mesh, face);

        check(before.size() == after.size(), "flip_face preservou quantidade de vértices");
        check(before.size() == 4, "face continua quad");

        std::size_t flippedCount = editor.flip_all_faces();

        check(flippedCount >= 1, "flip_all_faces flipou pelo menos uma face");

        print_counts(mesh);
    }

    void test_flip_edge()
    {
        section("Flip: flip_edge");

        LEM mesh;
        LEMDiff diff;
        TopologyEditor editor(mesh, diff);

        VertexHandle v0 = editor.add_vertex(glm::vec3(0.0f, 0.0f, 0.0f));
        VertexHandle v1 = editor.add_vertex(glm::vec3(1.0f, 0.0f, 0.0f));
        VertexHandle v2 = editor.add_vertex(glm::vec3(1.0f, 1.0f, 0.0f));
        VertexHandle v3 = editor.add_vertex(glm::vec3(0.0f, 1.0f, 0.0f));

        FaceHandle f0 = editor.add_face({ v0, v1, v2 });
        FaceHandle f1 = editor.add_face({ v0, v2, v3 });

        EdgeHandle diagonal = mesh.find_edge(v0, v2);

        check(mesh.is_valid(f0), "triângulo 0 criado");
        check(mesh.is_valid(f1), "triângulo 1 criado");
        check(mesh.is_valid(diagonal), "diagonal inicial v0-v2 existe");

        bool flipped = editor.flip_edge(diagonal);

        check(flipped, "flip_edge retornou true");
        check(!mesh.is_valid(diagonal), "edge diagonal antiga foi removida");
        check(mesh.is_valid(mesh.find_edge(v1, v3)), "nova diagonal v1-v3 existe");

        int triangleCount = 0;

        for (FaceHandle face : TopologyTraversal::faces(mesh)) {
            if (TopologyTraversal::face_vertices(mesh, face).size() == 3) {
                ++triangleCount;
            }
        }

        check(triangleCount == 2, "flip_edge manteve 2 triângulos ativos");

        print_counts(mesh);
    }

    void test_collapse_edge()
    {
        section("Collapse: collapse_edge");

        LEM mesh;
        LEMDiff diff;
        TopologyEditor editor(mesh, diff);

        VertexHandle a = editor.add_vertex(glm::vec3(0.0f, 0.0f, 0.0f));
        VertexHandle b = editor.add_vertex(glm::vec3(2.0f, 0.0f, 0.0f));
        EdgeHandle edge = editor.find_or_create_edge(a, b);

        bool collapsed = editor.collapse_edge(edge);

        check(collapsed, "collapse_edge retornou true");
        check(mesh.is_valid(a), "vértice A permanece válido");
        check(!mesh.is_valid(b), "vértice B foi removido");
        check(!mesh.is_valid(edge), "edge colapsada foi removida");

        const glm::vec3 position = mesh.vertex(a).position;
        check(position.x == 1.0f && position.y == 0.0f && position.z == 0.0f, "posição colapsada no ponto médio");

        print_counts(mesh);
    }

    void test_dissolve_edge()
    {
        section("Dissolve: dissolve_edge");

        LEM mesh;
        LEMDiff diff;
        TopologyEditor editor(mesh, diff);

        VertexHandle v0 = editor.add_vertex(glm::vec3(0.0f, 0.0f, 0.0f));
        VertexHandle v1 = editor.add_vertex(glm::vec3(1.0f, 0.0f, 0.0f));
        VertexHandle v2 = editor.add_vertex(glm::vec3(1.0f, 1.0f, 0.0f));
        VertexHandle v3 = editor.add_vertex(glm::vec3(0.0f, 1.0f, 0.0f));

        editor.add_face({ v0, v1, v2 });
        editor.add_face({ v0, v2, v3 });

        EdgeHandle diagonal = mesh.find_edge(v0, v2);

        check(mesh.is_valid(diagonal), "diagonal compartilhada existe");

        bool dissolved = editor.dissolve_edge(diagonal);

        check(dissolved, "dissolve_edge retornou true");
        check(!mesh.is_valid(diagonal), "edge dissolvida ficou inválida");

        bool foundMergedFace = false;

        for (FaceHandle face : TopologyTraversal::faces(mesh)) {
            if (TopologyTraversal::face_vertices(mesh, face).size() >= 3) {
                foundMergedFace = true;
                break;
            }
        }

        check(foundMergedFace, "dissolve_edge gerou uma face de região");

        print_counts(mesh);
    }

    void test_dissolve_vertex()
    {
        section("Dissolve: dissolve_vertex");

        LEM mesh;
        LEMDiff diff;
        TopologyEditor editor(mesh, diff);

        VertexHandle a = editor.add_vertex(glm::vec3(0.0f, 0.0f, 0.0f));
        VertexHandle b = editor.add_vertex(glm::vec3(1.0f, 0.0f, 0.0f));
        VertexHandle c = editor.add_vertex(glm::vec3(2.0f, 0.0f, 0.0f));

        EdgeHandle ab = editor.find_or_create_edge(a, b);
        EdgeHandle bc = editor.find_or_create_edge(b, c);

        check(mesh.is_valid(ab), "edge A-B criada");
        check(mesh.is_valid(bc), "edge B-C criada");

        bool dissolved = editor.dissolve_vertex(b);

        check(dissolved, "dissolve_vertex retornou true");
        check(!mesh.is_valid(b), "vértice B removido");
        check(!mesh.is_valid(ab), "edge A-B removida");
        check(!mesh.is_valid(bc), "edge B-C removida");
        check(mesh.is_valid(mesh.find_edge(a, c)), "edge A-C criada");

        print_counts(mesh);
    }

    void test_dissolve_face()
    {
        section("Dissolve: dissolve_face");

        LEM mesh;
        LEMDiff diff;
        TopologyEditor editor(mesh, diff);

        std::vector<VertexHandle> vertices = make_triangle_vertices(editor);
        FaceHandle face = editor.add_face(vertices);

        check(mesh.is_valid(face), "triângulo criado");

        bool dissolved = editor.dissolve_face(face);

        check(dissolved, "dissolve_face retornou true");
        check(!mesh.is_valid(face), "face dissolvida ficou inválida");

        for (VertexHandle vertex : vertices) {
            check(!mesh.is_valid(vertex), "vértice loose removido pelo dissolve_face");
        }

        print_counts(mesh);
    }

    void test_rebuild_normals_and_clear()
    {
        section("Utilitários: rebuild_face_normals e clear");

        LEM mesh;
        LEMDiff diff;
        TopologyEditor editor(mesh, diff);

        std::vector<VertexHandle> vertices = make_quad_vertices(editor);
        FaceHandle face = editor.add_face(vertices);

        check(mesh.is_valid(face), "face criada antes de rebuild_face_normals");

        const std::size_t diffBeforeNormals = diff.size();

        editor.rebuild_face_normals();

        check(diff.size() > diffBeforeNormals, "rebuild_face_normals registrou alteração no diff");

        editor.clear();

        check(mesh.empty(), "clear deixou a malha vazia");
        check(diff.size() > 0, "clear registrou alteração no diff");

        print_counts(mesh);
    }

}

int main()
{
    std::cout << "=== Locus3D TopologyEditor Test ===\n";

    test_creation();
    test_remove_face_and_loose_cleanup();
    test_kill_loop();
    test_kill_face_only();
    test_kill_edge_only();
    test_split_edge_loose();
    test_split_edge_at_param_on_face();
    test_split_face();
    test_flip_face_and_all_faces();
    test_flip_edge();
    test_collapse_edge();
    test_dissolve_edge();
    test_dissolve_vertex();
    test_dissolve_face();
    test_rebuild_normals_and_clear();

    std::cout << "\n=== Resultado ===\n";
    std::cout << "Passou: " << g_passed << '\n';
    std::cout << "Falhou: " << g_failed << '\n';

    if (g_failed == 0) {
        std::cout << "\nTodos os testes de TopologyEditor passaram.\n";
        return EXIT_SUCCESS;
    }

    std::cout << "\nAlguns testes de TopologyEditor falharam.\n";
    return EXIT_FAILURE;
}