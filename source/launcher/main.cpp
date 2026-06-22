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

    std::vector<VertexHandle> make_quad_vertices(LEMEditor& editor)
    {
        return {
            editor.add_vertex({ 0.0f, 0.0f, 0.0f }),
            editor.add_vertex({ 1.0f, 0.0f, 0.0f }),
            editor.add_vertex({ 1.0f, 1.0f, 0.0f }),
            editor.add_vertex({ 0.0f, 1.0f, 0.0f })
        };
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

    void test_transform_vertices(TestState& state)
    {
        print_section("transform_vertices");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle v0 = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle v1 = editor.add_vertex({ 1.0f, 0.0f, 0.0f });
        VertexHandle v2 = editor.add_vertex({ 1.0f, 1.0f, 0.0f });
        VertexHandle v3 = editor.add_vertex({ 0.0f, 1.0f, 0.0f });
        FaceHandle face = editor.add_face({ v0, v1, v2, v3 });

        editor.clear_diff();

        glm::mat4 transform(1.0f);
        transform[3] = glm::vec4(2.0f, 3.0f, 4.0f, 1.0f);

        std::size_t transformed = editor.transform_vertices({ v0, v1, v2, v3 }, transform);

        check(state, transformed == 4, "transform_vertices transformou 4 vertices");
        check(state, nearly_equal(mesh.vertex(v0).position, { 2.0f, 3.0f, 4.0f }), "transform_vertices atualizou v0");
        check(state, nearly_equal(mesh.vertex(v1).position, { 3.0f, 3.0f, 4.0f }), "transform_vertices atualizou v1");
        check(state, nearly_equal(mesh.vertex(v2).position, { 3.0f, 4.0f, 4.0f }), "transform_vertices atualizou v2");
        check(state, nearly_equal(mesh.vertex(v3).position, { 2.0f, 4.0f, 4.0f }), "transform_vertices atualizou v3");
        check(state, count_changes(editor.diff(), LEMChangeType::VertexModified, LEMElementType::Vertex) == 4, "transform_vertices registrou 4 VertexModified");
        check(state, count_changes(editor.diff(), LEMChangeType::NormalsChanged, LEMElementType::Face) >= 1, "transform_vertices atualizou normal da face");

        glm::vec3 normal = mesh.face(face).normal;
        check(state, glm::length(normal) > 0.0f, "normal continua valida apos transform_vertices");
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

    void test_flip_face(TestState& state)
    {
        print_section("flip_face");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle v0 = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle v1 = editor.add_vertex({ 1.0f, 0.0f, 0.0f });
        VertexHandle v2 = editor.add_vertex({ 1.0f, 1.0f, 0.0f });
        VertexHandle v3 = editor.add_vertex({ 0.0f, 1.0f, 0.0f });

        FaceHandle face = editor.add_face({ v0, v1, v2, v3 });

        std::vector<VertexHandle> before = TopologyTraversal::face_vertices(mesh, face);
        glm::vec3 normalBefore = mesh.face(face).normal;

        editor.clear_diff();

        bool flipped = editor.flip_face(face);

        std::vector<VertexHandle> after = TopologyTraversal::face_vertices(mesh, face);
        glm::vec3 normalAfter = mesh.face(face).normal;

        check(state, flipped, "flip_face retornou true");
        check(state, mesh.is_valid(face), "face continua valida apos flip_face");
        check(state, after.size() == 4, "face continua com 4 vertices apos flip_face");
        check(state, after[0] == before[0], "flip_face preservou loop de entrada da face");
        check(state, after[1] == before[3], "flip_face inverteu ordem do segundo vertice");
        check(state, after[2] == before[2], "flip_face inverteu ordem do terceiro vertice");
        check(state, after[3] == before[1], "flip_face inverteu ordem do quarto vertice");
        check(state, glm::length(normalAfter) > 0.0f, "normal apos flip_face nao e nula");
        check(state, glm::dot(normalBefore, normalAfter) < 0.0f, "flip_face inverteu direcao da normal");
        check(state, count_changes(editor.diff(), LEMChangeType::LoopModified, LEMElementType::Loop) >= 4, "flip_face registrou modificacao nos loops");
        check(state, count_changes(editor.diff(), LEMChangeType::FaceModified, LEMElementType::Face) >= 1, "flip_face registrou modificacao na face");
        check(state, count_changes(editor.diff(), LEMChangeType::NormalsChanged, LEMElementType::Face) >= 1, "flip_face registrou mudanca de normal");

        bool flippedInvalid = editor.flip_face(FaceHandle{});

        check(state, !flippedInvalid, "flip_face rejeitou handle invalido");
    }

    void test_flip_all_faces(TestState& state)
    {
        print_section("flip_all_faces");

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

        glm::vec3 n0 = mesh.face(f0).normal;
        glm::vec3 n1 = mesh.face(f1).normal;

        editor.clear_diff();

        std::size_t count = editor.flip_all_faces();

        check(state, count == 2, "flip_all_faces retornou 2");
        check(state, glm::dot(n0, mesh.face(f0).normal) < 0.0f, "flip_all_faces inverteu normal da primeira face");
        check(state, glm::dot(n1, mesh.face(f1).normal) < 0.0f, "flip_all_faces inverteu normal da segunda face");
        check(state, TopologyTraversal::faces(mesh).size() == 2, "flip_all_faces manteve 2 faces ativas");
        check(state, TopologyTraversal::loops(mesh).size() == 8, "flip_all_faces manteve 8 loops ativos");
        check(state, count_changes(editor.diff(), LEMChangeType::FaceModified, LEMElementType::Face) >= 2, "flip_all_faces registrou faces modificadas");
    }

    void test_remove_face_and_loose_cleanup(TestState& state)
    {
        print_section("remove_face e limpeza loose");

        LEM mesh;
        LEMEditor editor(mesh);

        VertexHandle v0 = editor.add_vertex({ 0.0f, 0.0f, 0.0f });
        VertexHandle v1 = editor.add_vertex({ 1.0f, 0.0f, 0.0f });
        VertexHandle v2 = editor.add_vertex({ 1.0f, 1.0f, 0.0f });
        VertexHandle v3 = editor.add_vertex({ 0.0f, 1.0f, 0.0f });

        FaceHandle face = editor.add_face({ v0, v1, v2, v3 });

        EdgeHandle e01 = mesh.find_edge(v0, v1);
        EdgeHandle e12 = mesh.find_edge(v1, v2);
        EdgeHandle e23 = mesh.find_edge(v2, v3);
        EdgeHandle e30 = mesh.find_edge(v3, v0);

        check(state, mesh.is_valid(face), "face inicial valida");
        check(state, mesh.is_valid(e01), "edge e01 valida antes da remocao");
        check(state, TopologyTraversal::edge_loops(mesh, e01).size() == 1, "edge e01 possui 1 loop antes da remocao");

        editor.clear_diff();

        bool removedFace = editor.remove_face(face);

        check(state, removedFace, "remove_face retornou true");
        check(state, !mesh.is_valid(face), "face removida ficou invalida");
        check(state, TopologyTraversal::faces(mesh).empty(), "traversal nao retorna face removida");
        check(state, TopologyTraversal::loops(mesh).empty(), "traversal nao retorna loops removidos");
        check(state, TopologyTraversal::edge_loops(mesh, e01).empty(), "edge e01 ficou sem loops");
        check(state, TopologyTraversal::edge_loops(mesh, e12).empty(), "edge e12 ficou sem loops");
        check(state, TopologyTraversal::edge_loops(mesh, e23).empty(), "edge e23 ficou sem loops");
        check(state, TopologyTraversal::edge_loops(mesh, e30).empty(), "edge e30 ficou sem loops");
        check(state, count_changes(editor.diff(), LEMChangeType::FaceModified, LEMElementType::Face) >= 1, "remove_face registrou FaceModified");
        check(state, count_changes(editor.diff(), LEMChangeType::LoopModified, LEMElementType::Loop) >= 4, "remove_face registrou LoopModified");

        editor.clear_diff();

        bool removedE01 = editor.remove_edge_if_loose(e01);
        bool removedE12 = editor.remove_edge_if_loose(e12);
        bool removedE23 = editor.remove_edge_if_loose(e23);
        bool removedE30 = editor.remove_edge_if_loose(e30);

        check(state, removedE01, "remove_edge_if_loose removeu e01");
        check(state, removedE12, "remove_edge_if_loose removeu e12");
        check(state, removedE23, "remove_edge_if_loose removeu e23");
        check(state, removedE30, "remove_edge_if_loose removeu e30");
        check(state, TopologyTraversal::edges(mesh).empty(), "traversal nao retorna edges removidas");
        check(state, count_changes(editor.diff(), LEMChangeType::EdgeModified, LEMElementType::Edge) >= 4, "remove_edge_if_loose registrou edges modificadas");

        editor.clear_diff();

        bool removedV0 = editor.remove_vertex_if_loose(v0);
        bool removedV1 = editor.remove_vertex_if_loose(v1);
        bool removedV2 = editor.remove_vertex_if_loose(v2);
        bool removedV3 = editor.remove_vertex_if_loose(v3);

        check(state, removedV0, "remove_vertex_if_loose removeu v0");
        check(state, removedV1, "remove_vertex_if_loose removeu v1");
        check(state, removedV2, "remove_vertex_if_loose removeu v2");
        check(state, removedV3, "remove_vertex_if_loose removeu v3");
        check(state, TopologyTraversal::vertices(mesh).empty(), "traversal nao retorna vertices removidos");
        check(state, count_changes(editor.diff(), LEMChangeType::VertexModified, LEMElementType::Vertex) >= 4, "remove_vertex_if_loose registrou vertices modificados");

        bool removedInvalidFace = editor.remove_face(FaceHandle{});
        bool removedInvalidEdge = editor.remove_edge_if_loose(EdgeHandle{});
        bool removedInvalidVertex = editor.remove_vertex_if_loose(VertexHandle{});

        check(state, !removedInvalidFace, "remove_face rejeitou face invalida");
        check(state, !removedInvalidEdge, "remove_edge_if_loose rejeitou edge invalida");
        check(state, !removedInvalidVertex, "remove_vertex_if_loose rejeitou vertex invalido");
    }

    void test_remove_shared_face_preserves_neighbor(TestState& state)
    {
        print_section("remove_face preservando vizinho compartilhado");

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

        EdgeHandle shared = mesh.find_edge(v1, v2);

        check(state, mesh.is_valid(shared), "edge compartilhada valida antes da remocao");
        check(state, TopologyTraversal::edge_loops(mesh, shared).size() == 2, "edge compartilhada com 2 loops antes");

        editor.clear_diff();

        bool removed = editor.remove_face(f0);

        check(state, removed, "remove_face removeu primeira face");
        check(state, !mesh.is_valid(f0), "primeira face ficou invalida");
        check(state, mesh.is_valid(f1), "segunda face continua valida");
        check(state, TopologyTraversal::faces(mesh).size() == 1, "traversal retorna 1 face restante");
        check(state, TopologyTraversal::face_loops(mesh, f1).size() == 4, "face restante preservou 4 loops");
        check(state, TopologyTraversal::edge_loops(mesh, shared).size() == 1, "edge compartilhada preservou 1 loop radial");
        check(state, TopologyTraversal::edge_faces(mesh, shared).size() == 1, "edge compartilhada preservou 1 face adjacente");
        check(state, TopologyTraversal::is_boundary_edge(mesh, shared), "edge compartilhada virou boundary apos remocao");
        check(state, TopologyTraversal::is_manifold_edge(mesh, shared), "edge compartilhada continua manifold");

        bool removedSharedEdge = editor.remove_edge_if_loose(shared);

        check(state, !removedSharedEdge, "remove_edge_if_loose nao remove edge ainda usada");
        check(state, mesh.is_valid(shared), "edge compartilhada continua valida");
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
    std::cout << "=== Locus3D LEMEditor Full Regression Test ===\n";

    TestState state;

    test_editor_facade_creation(state);
    test_internal_editors_creation(state);
    test_geometry_editing(state);
    test_transform_vertices(state);
    test_attribute_editing(state);
    test_flip_face(state);
    test_flip_all_faces(state);
    test_remove_face_and_loose_cleanup(state);
    test_remove_shared_face_preserves_neighbor(state);
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