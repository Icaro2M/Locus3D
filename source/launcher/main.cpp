#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <glm/glm.hpp>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

using namespace locus::kernel;
using namespace locus::kernel::geometry;

namespace {

    struct TestState {
        int passed = 0;
        int failed = 0;
    };

    bool nearly_equal(float a, float b, float epsilon = 1.0e-5f)
    {
        return std::abs(a - b) <= epsilon;
    }

    bool nearly_equal(const glm::vec3& a, const glm::vec3& b, float epsilon = 1.0e-5f)
    {
        return nearly_equal(a.x, b.x, epsilon)
            && nearly_equal(a.y, b.y, epsilon)
            && nearly_equal(a.z, b.z, epsilon);
    }

    void check(TestState& state, bool condition, const std::string& message)
    {
        if (condition) {
            ++state.passed;
            std::cout << "[OK] " << message << '\n';
            return;
        }

        ++state.failed;
        std::cout << "[FAIL] " << message << '\n';
    }

    void print_section(const std::string& title)
    {
        std::cout << "\n=== " << title << " ===\n";
    }

    std::size_t count_changes(const LEMDiff& diff, LEMChangeType type)
    {
        std::size_t count = 0;

        for (const LEMChange& change : diff.changes()) {
            if (change.type == type) {
                ++count;
            }
        }

        return count;
    }

    std::size_t count_changes(const LEMDiff& diff, LEMChangeType type, LEMElementType elementType)
    {
        std::size_t count = 0;

        for (const LEMChange& change : diff.changes()) {
            if (change.type == type && change.elementType == elementType) {
                ++count;
            }
        }

        return count;
    }

    void test_editor_facade_creation(TestState& state)
    {
        print_section("LEMEditor facade: criacao basica");

        LEM mesh;
        LEMEditor editor(mesh);

        check(state, mesh.empty(), "malha comeca vazia");
        check(state, editor.diff().empty(), "diff comeca vazio");

        VertexHandle v0 = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle v1 = editor.add_vertex({ 1.0f, 0.0f, 0.0f });
        VertexHandle v2 = editor.add_vertex({ 1.0f, 1.0f, 0.0f });
        VertexHandle v3 = editor.add_vertex({ 0.0f, 1.0f, 0.0f });

        check(state, mesh.is_valid(v0), "v0 valido");
        check(state, mesh.is_valid(v1), "v1 valido");
        check(state, mesh.is_valid(v2), "v2 valido");
        check(state, mesh.is_valid(v3), "v3 valido");
        check(state, mesh.vertex_count() == 4, "4 vertices criados");
        check(state, count_changes(editor.diff(), LEMChangeType::VertexAdded, LEMElementType::Vertex) == 4, "diff registrou 4 vertices adicionados");

        FaceHandle face = editor.add_face({ v0, v1, v2, v3 });

        check(state, mesh.is_valid(face), "face quad criada pela fachada");
        check(state, mesh.vertex_count() == 4, "quad manteve 4 vertices");
        check(state, mesh.edge_count() == 4, "quad possui 4 edges");
        check(state, mesh.loop_count() == 4, "quad possui 4 loops");
        check(state, mesh.face_count() == 1, "quad possui 1 face");
        check(state, mesh.face_loops(face).size() == 4, "face possui ciclo com 4 loops");
        check(state, count_changes(editor.diff(), LEMChangeType::EdgeAdded, LEMElementType::Edge) == 4, "diff registrou 4 edges adicionadas");
        check(state, count_changes(editor.diff(), LEMChangeType::LoopAdded, LEMElementType::Loop) == 4, "diff registrou 4 loops adicionados");
        check(state, count_changes(editor.diff(), LEMChangeType::FaceAdded, LEMElementType::Face) == 1, "diff registrou 1 face adicionada");

        EdgeHandle e01 = editor.find_or_create_edge(v0, v1);
        EdgeHandle e10 = editor.find_or_create_edge(v1, v0);

        check(state, mesh.is_valid(e01), "find_or_create_edge retornou edge valida");
        check(state, e01 == e10, "edge nao-direcional retorna o mesmo handle");
        check(state, mesh.edge_count() == 4, "find_or_create_edge nao duplicou edge existente");
    }

    void test_internal_editors_creation(TestState& state)
    {
        print_section("editores internos: topology/geometry/attributes");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle v0 = editor.topology().add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle v1 = editor.topology().add_vertex({ 2.0f, 0.0f, 0.0f });
        VertexHandle v2 = editor.topology().add_vertex({ 2.0f, 2.0f, 0.0f });
        VertexHandle v3 = editor.topology().add_vertex({ 0.0f, 2.0f, 0.0f });

        FaceHandle face = editor.topology().add_face({ v0, v1, v2, v3 });

        check(state, mesh.is_valid(face), "topology().add_face criou face valida");
        check(state, editor.topology().mesh().face_count() == 1, "topology().mesh() aponta para a mesma malha");
        check(state, &editor.topology().mesh() == &mesh, "topology editor referencia a malha original");
        check(state, &editor.geometry().mesh() == &mesh, "geometry editor referencia a malha original");
        check(state, &editor.attributes().mesh() == &mesh, "attribute editor referencia a malha original");

        bool moved = editor.geometry().set_vertex_position(v0, { -1.0f, 0.0f, 0.0f });

        check(state, moved, "geometry().set_vertex_position aceitou vertice valido");
        check(state, nearly_equal(mesh.vertex(v0).position, { -1.0f, 0.0f, 0.0f }), "geometry editor alterou posicao do vertice");

        bool selected = editor.attributes().set_selected(face, true);
        bool hidden = editor.attributes().set_hidden(v1, true);

        check(state, selected, "attributes().set_selected aceitou face valida");
        check(state, hidden, "attributes().set_hidden aceitou vertice valido");
        check(state, mesh.face(face).selected, "face ficou selecionada");
        check(state, mesh.vertex(v1).hidden, "vertice ficou oculto");

        check(state, count_changes(editor.diff(), LEMChangeType::VertexModified, LEMElementType::Vertex) >= 1, "diff registrou modificacao de vertice");
        check(state, count_changes(editor.diff(), LEMChangeType::SelectionChanged, LEMElementType::Face) >= 1, "diff registrou mudanca de selecao em face");
        check(state, count_changes(editor.diff(), LEMChangeType::VisibilityChanged, LEMElementType::Vertex) >= 1, "diff registrou mudanca de visibilidade em vertice");
    }

    void test_geometry_editing(TestState& state)
    {
        print_section("edicao geometrica");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle v0 = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle v1 = editor.add_vertex({ 1.0f, 0.0f, 0.0f });
        VertexHandle v2 = editor.add_vertex({ 1.0f, 1.0f, 0.0f });
        VertexHandle v3 = editor.add_vertex({ 0.0f, 1.0f, 0.0f });
        FaceHandle face = editor.add_face({ v0, v1, v2, v3 });

        editor.clear_diff();

        bool setPositionResult = editor.set_vertex_position(v0, { 0.0f, 0.0f, 1.0f });

        check(state, setPositionResult, "set_vertex_position retornou true para vertice valido");
        check(state, nearly_equal(mesh.vertex(v0).position, { 0.0f, 0.0f, 1.0f }), "set_vertex_position atualizou posicao");
        check(state, count_changes(editor.diff(), LEMChangeType::VertexModified, LEMElementType::Vertex) == 1, "set_vertex_position registrou VertexModified");
        check(state, count_changes(editor.diff(), LEMChangeType::NormalsChanged, LEMElementType::Face) >= 1, "set_vertex_position atualizou normais adjacentes");

        glm::vec3 normalAfterSet = mesh.face(face).normal;

        check(state, glm::length(normalAfterSet) > 0.0f, "normal da face continua valida apos mover vertice");

        editor.clear_diff();

        bool translateResult = editor.translate_vertex(v1, { 0.0f, 0.0f, 2.0f });

        check(state, translateResult, "translate_vertex retornou true para vertice valido");
        check(state, nearly_equal(mesh.vertex(v1).position, { 1.0f, 0.0f, 2.0f }), "translate_vertex moveu vertice");
        check(state, count_changes(editor.diff(), LEMChangeType::VertexModified, LEMElementType::Vertex) == 1, "translate_vertex registrou VertexModified");

        editor.clear_diff();

        std::size_t translated = editor.translate_vertices({ v0, v1, v2, v3 }, { 1.0f, 0.0f, 0.0f });

        check(state, translated == 4, "translate_vertices moveu 4 vertices");
        check(state, nearly_equal(mesh.vertex(v0).position, { 1.0f, 0.0f, 1.0f }), "translate_vertices atualizou v0");
        check(state, nearly_equal(mesh.vertex(v1).position, { 2.0f, 0.0f, 2.0f }), "translate_vertices atualizou v1");
        check(state, nearly_equal(mesh.vertex(v2).position, { 2.0f, 1.0f, 0.0f }), "translate_vertices atualizou v2");
        check(state, nearly_equal(mesh.vertex(v3).position, { 1.0f, 1.0f, 0.0f }), "translate_vertices atualizou v3");
        check(state, count_changes(editor.diff(), LEMChangeType::VertexModified, LEMElementType::Vertex) == 4, "translate_vertices registrou 4 VertexModified");

        VertexHandle invalidVertex;
        bool invalidMove = editor.translate_vertex(invalidVertex, { 1.0f, 0.0f, 0.0f });

        check(state, !invalidMove, "translate_vertex rejeitou handle invalido");
    }

    void test_attribute_editing(TestState& state)
    {
        print_section("edicao de atributos");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle v0 = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle v1 = editor.add_vertex({ 1.0f, 0.0f, 0.0f });
        VertexHandle v2 = editor.add_vertex({ 1.0f, 1.0f, 0.0f });
        VertexHandle v3 = editor.add_vertex({ 0.0f, 1.0f, 0.0f });
        FaceHandle face = editor.add_face({ v0, v1, v2, v3 });
        EdgeHandle edge = editor.find_or_create_edge(v0, v1);

        editor.clear_diff();

        check(state, editor.set_selected(v0, true), "set_selected vertex retornou true");
        check(state, editor.set_selected(edge, true), "set_selected edge retornou true");
        check(state, editor.set_selected(face, true), "set_selected face retornou true");
        check(state, mesh.vertex(v0).selected, "vertex selecionado");
        check(state, mesh.edge(edge).selected, "edge selecionada");
        check(state, mesh.face(face).selected, "face selecionada");
        check(state, count_changes(editor.diff(), LEMChangeType::SelectionChanged) == 3, "diff registrou 3 SelectionChanged");

        editor.clear_selection();

        check(state, !mesh.vertex(v0).selected, "clear_selection limpou vertex");
        check(state, !mesh.edge(edge).selected, "clear_selection limpou edge");
        check(state, !mesh.face(face).selected, "clear_selection limpou face");

        editor.clear_diff();

        check(state, editor.set_hidden(v0, true), "set_hidden vertex retornou true");
        check(state, editor.set_hidden(edge, true), "set_hidden edge retornou true");
        check(state, editor.set_hidden(face, true), "set_hidden face retornou true");
        check(state, mesh.vertex(v0).hidden, "vertex oculto");
        check(state, mesh.edge(edge).hidden, "edge oculta");
        check(state, mesh.face(face).hidden, "face oculta");
        check(state, count_changes(editor.diff(), LEMChangeType::VisibilityChanged) == 3, "diff registrou 3 VisibilityChanged");

        editor.clear_visibility();

        check(state, !mesh.vertex(v0).hidden, "clear_visibility limpou vertex");
        check(state, !mesh.edge(edge).hidden, "clear_visibility limpou edge");
        check(state, !mesh.face(face).hidden, "clear_visibility limpou face");

        VertexHandle invalidVertex;
        EdgeHandle invalidEdge;
        FaceHandle invalidFace;

        check(state, !editor.set_selected(invalidVertex, true), "set_selected rejeitou vertex invalido");
        check(state, !editor.set_selected(invalidEdge, true), "set_selected rejeitou edge invalida");
        check(state, !editor.set_selected(invalidFace, true), "set_selected rejeitou face invalida");
        check(state, !editor.set_hidden(invalidVertex, true), "set_hidden rejeitou vertex invalido");
        check(state, !editor.set_hidden(invalidEdge, true), "set_hidden rejeitou edge invalida");
        check(state, !editor.set_hidden(invalidFace, true), "set_hidden rejeitou face invalida");
    }

    void test_topology_traversal_regression(TestState& state)
    {
        print_section("regressao de traversal/topologia");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle v0 = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle v1 = editor.add_vertex({ 1.0f, 0.0f, 0.0f });
        VertexHandle v2 = editor.add_vertex({ 1.0f, 1.0f, 0.0f });
        VertexHandle v3 = editor.add_vertex({ 0.0f, 1.0f, 0.0f });
        VertexHandle v4 = editor.add_vertex({ 2.0f, 0.0f, 0.0f });
        VertexHandle v5 = editor.add_vertex({ 2.0f, 1.0f, 0.0f });

        FaceHandle f0 = editor.add_face({ v0, v1, v2, v3 });
        FaceHandle f1 = editor.add_face({ v1, v4, v5, v2 });

        check(state, mesh.is_valid(f0), "primeira face valida");
        check(state, mesh.is_valid(f1), "segunda face valida");
        check(state, mesh.vertex_count() == 6, "malha com duas faces possui 6 vertices");
        check(state, mesh.edge_count() == 7, "malha com duas faces compartilhando edge possui 7 edges");
        check(state, mesh.loop_count() == 8, "malha com duas faces possui 8 loops");
        check(state, mesh.face_count() == 2, "malha com duas faces possui 2 faces");

        EdgeHandle shared = mesh.find_edge(v1, v2);

        check(state, mesh.is_valid(shared), "edge compartilhada encontrada");
        check(state, TopologyTraversal::vertices(mesh).size() == 6, "TopologyTraversal::vertices retornou 6 vertices");
        check(state, TopologyTraversal::edges(mesh).size() == 7, "TopologyTraversal::edges retornou 7 edges");
        check(state, TopologyTraversal::loops(mesh).size() == 8, "TopologyTraversal::loops retornou 8 loops");
        check(state, TopologyTraversal::faces(mesh).size() == 2, "TopologyTraversal::faces retornou 2 faces");
        check(state, TopologyTraversal::face_vertices(mesh, f0).size() == 4, "face_vertices retornou 4 vertices para f0");
        check(state, TopologyTraversal::face_edges(mesh, f0).size() == 4, "face_edges retornou 4 edges para f0");
        check(state, TopologyTraversal::edge_loops(mesh, shared).size() == 2, "edge compartilhada possui 2 loops radiais");
        check(state, TopologyTraversal::edge_faces(mesh, shared).size() == 2, "edge compartilhada possui 2 faces adjacentes");
        check(state, TopologyTraversal::vertex_edges(mesh, v1).size() >= 3, "vertex_edges encontrou edges incidentes em v1");
        check(state, TopologyTraversal::vertex_faces(mesh, v1).size() == 2, "vertex_faces encontrou 2 faces adjacentes em v1");
        check(state, TopologyTraversal::adjacent_vertices(mesh, v1).size() >= 3, "adjacent_vertices encontrou vizinhos de v1");
        check(state, !TopologyTraversal::is_boundary_edge(mesh, shared), "edge compartilhada nao e boundary");
        check(state, TopologyTraversal::is_manifold_edge(mesh, shared), "edge compartilhada e manifold");
    }

    void test_normals_and_clear(TestState& state)
    {
        print_section("normais, diff e clear");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle v0 = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle v1 = editor.add_vertex({ 1.0f, 0.0f, 0.0f });
        VertexHandle v2 = editor.add_vertex({ 1.0f, 1.0f, 0.0f });
        VertexHandle v3 = editor.add_vertex({ 0.0f, 1.0f, 0.0f });
        FaceHandle face = editor.add_face({ v0, v1, v2, v3 });

        editor.clear_diff();
        editor.rebuild_face_normals();

        glm::vec3 normal = mesh.face(face).normal;

        check(state, glm::length(normal) > 0.0f, "rebuild_face_normals gerou normal nao nula");
        check(state, count_changes(editor.diff(), LEMChangeType::NormalsChanged, LEMElementType::Face) == 1, "rebuild_face_normals registrou NormalsChanged");

        LEMDiff diff = editor.take_diff();

        check(state, !diff.empty(), "take_diff retornou diff preenchido");
        check(state, editor.diff().empty(), "take_diff limpou diff interno");

        editor.clear_diff();

        check(state, editor.diff().empty(), "clear_diff manteve diff vazio");

        editor.clear();

        check(state, mesh.empty(), "clear limpou a malha");
        check(state, mesh.vertex_count() == 0, "clear zerou vertices");
        check(state, mesh.edge_count() == 0, "clear zerou edges");
        check(state, mesh.loop_count() == 0, "clear zerou loops");
        check(state, mesh.face_count() == 0, "clear zerou faces");
        check(state, count_changes(editor.diff(), LEMChangeType::MeshCleared) == 1, "clear registrou MeshCleared");
    }

}

int main()
{
    std::cout << "=== Locus3D LEMEditor Regression Test ===\n";

    TestState state;

    test_editor_facade_creation(state);
    test_internal_editors_creation(state);
    test_geometry_editing(state);
    test_attribute_editing(state);
    test_topology_traversal_regression(state);
    test_normals_and_clear(state);

    std::cout << "\n=== Resultado final ===\n";
    std::cout << "Passou: " << state.passed << '\n';
    std::cout << "Falhou: " << state.failed << '\n';

    if (state.failed == 0) {
        std::cout << "\nTodos os testes passaram.\n";
        return 0;
    }

    std::cout << "\nAlguns testes falharam.\n";
    return 1;
}