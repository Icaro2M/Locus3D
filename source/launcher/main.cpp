/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/primitives/PrimitiveBuilder.h"
#include "graphics/primitives/PrimitiveMesh.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace {

    constexpr float FloatTolerance = 0.0001f;

    bool expect(
        const bool condition,
        const std::string& message
    ) {
        if (condition) {
            std::cout << "[OK] " << message << '\n';
            return true;
        }

        std::cout << "[FAIL] " << message << '\n';
        return false;
    }

    bool nearly_equal(
        const float lhs,
        const float rhs,
        const float tolerance = FloatTolerance
    ) {
        return std::abs(lhs - rhs) <= tolerance;
    }

    bool nearly_equal(
        const glm::vec3& lhs,
        const glm::vec3& rhs,
        const float tolerance = FloatTolerance
    ) {
        return nearly_equal(lhs.x, rhs.x, tolerance)
            && nearly_equal(lhs.y, rhs.y, tolerance)
            && nearly_equal(lhs.z, rhs.z, tolerance);
    }

    bool color_equal(
        const locus::graphics::ColorRGBA& lhs,
        const locus::graphics::ColorRGBA& rhs,
        const float tolerance = FloatTolerance
    ) {
        return nearly_equal(lhs.r, rhs.r, tolerance)
            && nearly_equal(lhs.g, rhs.g, tolerance)
            && nearly_equal(lhs.b, rhs.b, tolerance)
            && nearly_equal(lhs.a, rhs.a, tolerance);
    }

    bool test_primitive_mesh_validation() {
        using namespace locus::graphics;

        std::cout << "\n=== PrimitiveMesh validation ===\n";

        bool ok = true;

        PrimitiveMesh emptyMesh;
        emptyMesh.topology = PrimitiveTopology::Triangles;

        ok &= expect(
            emptyMesh.is_empty(),
            "mesh vazia informa is_empty"
        );

        ok &= expect(
            !emptyMesh.has_indices(),
            "mesh vazia nao possui indices"
        );

        ok &= expect(
            emptyMesh.element_count() == 0,
            "mesh vazia possui element_count igual a zero"
        );

        ok &= expect(
            !emptyMesh.is_valid(),
            "mesh vazia e invalida"
        );

        PrimitiveMesh validPoints;
        validPoints.topology = PrimitiveTopology::Points;
        validPoints.vertices.push_back(PrimitiveVertex{});

        ok &= expect(
            validPoints.is_valid(),
            "mesh Points com um vertice e valida"
        );

        PrimitiveMesh invalidLines;
        invalidLines.topology = PrimitiveTopology::Lines;
        invalidLines.vertices.push_back(PrimitiveVertex{});

        ok &= expect(
            !invalidLines.is_valid(),
            "mesh Lines com quantidade impar de vertices e invalida"
        );

        invalidLines.vertices.push_back(PrimitiveVertex{});

        ok &= expect(
            invalidLines.is_valid(),
            "mesh Lines com dois vertices e valida"
        );

        PrimitiveMesh invalidTriangles;
        invalidTriangles.topology = PrimitiveTopology::Triangles;
        invalidTriangles.vertices.resize(2);

        ok &= expect(
            !invalidTriangles.is_valid(),
            "mesh Triangles com dois vertices e invalida"
        );

        invalidTriangles.vertices.push_back(PrimitiveVertex{});

        ok &= expect(
            invalidTriangles.is_valid(),
            "mesh Triangles com tres vertices e valida"
        );

        PrimitiveMesh validIndexedTriangle;
        validIndexedTriangle.topology = PrimitiveTopology::Triangles;
        validIndexedTriangle.vertices.resize(3);
        validIndexedTriangle.indices = { 0, 1, 2 };

        ok &= expect(
            validIndexedTriangle.has_indices(),
            "mesh indexada informa has_indices"
        );

        ok &= expect(
            validIndexedTriangle.element_count() == 3,
            "mesh indexada usa quantidade de indices como element_count"
        );

        ok &= expect(
            validIndexedTriangle.is_valid(),
            "triangulo indexado valido e aceito"
        );

        PrimitiveMesh invalidIndexMesh;
        invalidIndexMesh.topology = PrimitiveTopology::Triangles;
        invalidIndexMesh.vertices.resize(3);
        invalidIndexMesh.indices = { 0, 1, 9 };

        ok &= expect(
            !invalidIndexMesh.is_valid(),
            "indice fora do intervalo torna a mesh invalida"
        );

        PrimitiveMesh validLineStrip;
        validLineStrip.topology = PrimitiveTopology::LineStrip;
        validLineStrip.vertices.resize(2);

        ok &= expect(
            validLineStrip.is_valid(),
            "LineStrip com dois vertices e valido"
        );

        PrimitiveMesh validTriangleStrip;
        validTriangleStrip.topology = PrimitiveTopology::TriangleStrip;
        validTriangleStrip.vertices.resize(3);

        ok &= expect(
            validTriangleStrip.is_valid(),
            "TriangleStrip com tres vertices e valido"
        );

        return ok;
    }

    bool test_point_builder() {
        using namespace locus::graphics;

        std::cout << "\n=== PrimitiveBuilder Points ===\n";

        bool ok = true;

        PrimitiveBuilder builder{ PrimitiveTopology::Points };

        const glm::vec3 position{ 1.0f, 2.0f, 3.0f };
        const ColorRGBA color{ 1.0f, 0.25f, 0.5f, 0.75f };

        ok &= expect(
            builder.topology() == PrimitiveTopology::Points,
            "builder preserva topologia Points"
        );

        ok &= expect(
            builder.is_empty(),
            "builder Points inicia vazio"
        );

        ok &= expect(
            builder.add_point(position, color),
            "add_point e aceito em builder Points"
        );

        ok &= expect(
            builder.vertex_count() == 1,
            "add_point adiciona um vertice"
        );

        ok &= expect(
            nearly_equal(builder.mesh().vertices[0].position, position),
            "add_point preserva posicao"
        );

        ok &= expect(
            nearly_equal(
                builder.mesh().vertices[0].normal,
                glm::vec3{ 0.0f }
            ),
            "add_point usa normal neutra"
        );

        ok &= expect(
            color_equal(builder.mesh().vertices[0].color, color),
            "add_point preserva cor"
        );

        ok &= expect(
            !builder.add_line(
                glm::vec3{ 0.0f },
                glm::vec3{ 1.0f }
            ),
            "builder Points rejeita add_line"
        );

        ok &= expect(
            builder.vertex_count() == 1,
            "operacao rejeitada nao altera a geometria"
        );

        PrimitiveMesh mesh = builder.build();

        ok &= expect(
            mesh.topology == PrimitiveTopology::Points,
            "build retorna mesh Points"
        );

        ok &= expect(
            mesh.vertices.size() == 1,
            "build retorna o ponto acumulado"
        );

        ok &= expect(
            mesh.is_valid(),
            "mesh de pontos produzida e valida"
        );

        ok &= expect(
            builder.is_empty(),
            "build limpa o builder"
        );

        ok &= expect(
            builder.topology() == PrimitiveTopology::Points,
            "build preserva a topologia do builder"
        );

        return ok;
    }

    bool test_line_builder() {
        using namespace locus::graphics;

        std::cout << "\n=== PrimitiveBuilder Lines ===\n";

        bool ok = true;

        PrimitiveBuilder builder{ PrimitiveTopology::Lines };

        const glm::vec3 start{ -1.0f, 0.0f, 0.0f };
        const glm::vec3 end{ 1.0f, 0.0f, 0.0f };

        const ColorRGBA startColor{
            1.0f,
            0.0f,
            0.0f,
            1.0f
        };

        const ColorRGBA endColor{
            0.0f,
            0.0f,
            1.0f,
            1.0f
        };

        ok &= expect(
            builder.add_line(
                start,
                end,
                startColor,
                endColor
            ),
            "add_line e aceito em builder Lines"
        );

        ok &= expect(
            builder.vertex_count() == 2,
            "add_line adiciona dois vertices"
        );

        ok &= expect(
            nearly_equal(builder.mesh().vertices[0].position, start)
            && nearly_equal(builder.mesh().vertices[1].position, end),
            "add_line preserva os extremos"
        );

        ok &= expect(
            color_equal(
                builder.mesh().vertices[0].color,
                startColor
            ),
            "add_line preserva a cor inicial"
        );

        ok &= expect(
            color_equal(
                builder.mesh().vertices[1].color,
                endColor
            ),
            "add_line preserva a cor final"
        );

        ok &= expect(
            !builder.add_point(glm::vec3{ 0.0f }),
            "builder Lines rejeita add_point"
        );

        ok &= expect(
            !builder.add_triangle(
                glm::vec3{ 0.0f, 0.0f, 0.0f },
                glm::vec3{ 1.0f, 0.0f, 0.0f },
                glm::vec3{ 0.0f, 1.0f, 0.0f }
            ),
            "builder Lines rejeita add_triangle"
        );

        ok &= expect(
            builder.vertex_count() == 2,
            "operacoes rejeitadas nao alteram as linhas"
        );

        PrimitiveMesh mesh = builder.build();

        ok &= expect(
            mesh.vertices.size() == 2,
            "build retorna os dois vertices da linha"
        );

        ok &= expect(
            mesh.is_valid(),
            "mesh de linha produzida e valida"
        );

        return ok;
    }

    bool test_triangle_builder() {
        using namespace locus::graphics;

        std::cout << "\n=== PrimitiveBuilder Triangles ===\n";

        bool ok = true;

        PrimitiveBuilder builder{ PrimitiveTopology::Triangles };

        const glm::vec3 a{ 0.0f, 0.0f, 0.0f };
        const glm::vec3 b{ 1.0f, 0.0f, 0.0f };
        const glm::vec3 c{ 0.0f, 1.0f, 0.0f };

        const ColorRGBA color{
            0.25f,
            0.75f,
            1.0f,
            1.0f
        };

        ok &= expect(
            builder.add_triangle(a, b, c, color),
            "add_triangle e aceito em builder Triangles"
        );

        ok &= expect(
            builder.vertex_count() == 3,
            "add_triangle adiciona tres vertices"
        );

        const glm::vec3 expectedNormal{
            0.0f,
            0.0f,
            1.0f
        };

        ok &= expect(
            nearly_equal(
                builder.mesh().vertices[0].normal,
                expectedNormal
            ),
            "triangulo recebe normal correta"
        );

        ok &= expect(
            nearly_equal(
                builder.mesh().vertices[1].normal,
                expectedNormal
            )
            && nearly_equal(
                builder.mesh().vertices[2].normal,
                expectedNormal
            ),
            "normal calculada e aplicada aos tres vertices"
        );

        ok &= expect(
            nearly_equal(
                glm::length(builder.mesh().vertices[0].normal),
                1.0f
            ),
            "normal do triangulo possui comprimento unitario"
        );

        ok &= expect(
            color_equal(builder.mesh().vertices[0].color, color)
            && color_equal(builder.mesh().vertices[1].color, color)
            && color_equal(builder.mesh().vertices[2].color, color),
            "cor e aplicada aos tres vertices"
        );

        ok &= expect(
            !builder.add_line(a, b),
            "builder Triangles rejeita add_line"
        );

        PrimitiveMesh mesh = builder.build();

        ok &= expect(
            mesh.vertices.size() == 3,
            "build retorna os tres vertices do triangulo"
        );

        ok &= expect(
            mesh.is_valid(),
            "mesh triangular produzida e valida"
        );

        return ok;
    }

    bool test_degenerate_triangle() {
        using namespace locus::graphics;

        std::cout << "\n=== Degenerate triangle ===\n";

        bool ok = true;

        PrimitiveBuilder builder{ PrimitiveTopology::Triangles };

        const glm::vec3 a{ 0.0f, 0.0f, 0.0f };
        const glm::vec3 b{ 1.0f, 0.0f, 0.0f };
        const glm::vec3 c{ 2.0f, 0.0f, 0.0f };

        ok &= expect(
            builder.add_triangle(a, b, c),
            "triangulo degenerado ainda pode ser armazenado"
        );

        ok &= expect(
            builder.vertex_count() == 3,
            "triangulo degenerado adiciona tres vertices"
        );

        const glm::vec3 zeroNormal{ 0.0f };

        ok &= expect(
            nearly_equal(
                builder.mesh().vertices[0].normal,
                zeroNormal
            )
            && nearly_equal(
                builder.mesh().vertices[1].normal,
                zeroNormal
            )
            && nearly_equal(
                builder.mesh().vertices[2].normal,
                zeroNormal
            ),
            "triangulo degenerado recebe normal zero"
        );

        return ok;
    }

    bool test_quad_builder() {
        using namespace locus::graphics;

        std::cout << "\n=== PrimitiveBuilder Quad ===\n";

        bool ok = true;

        PrimitiveBuilder builder{ PrimitiveTopology::Triangles };

        const glm::vec3 a{ -1.0f, -1.0f, 0.0f };
        const glm::vec3 b{ 1.0f, -1.0f, 0.0f };
        const glm::vec3 c{ 1.0f, 1.0f, 0.0f };
        const glm::vec3 d{ -1.0f, 1.0f, 0.0f };

        ok &= expect(
            builder.add_quad(a, b, c, d),
            "add_quad e aceito em builder Triangles"
        );

        ok &= expect(
            builder.vertex_count() == 6,
            "add_quad gera dois triangulos e seis vertices"
        );

        PrimitiveMesh mesh = builder.build();

        ok &= expect(
            mesh.is_valid(),
            "mesh produzida por add_quad e valida"
        );

        ok &= expect(
            nearly_equal(mesh.vertices[0].position, a)
            && nearly_equal(mesh.vertices[1].position, b)
            && nearly_equal(mesh.vertices[2].position, c),
            "primeiro triangulo do quad usa a, b, c"
        );

        ok &= expect(
            nearly_equal(mesh.vertices[3].position, a)
            && nearly_equal(mesh.vertices[4].position, c)
            && nearly_equal(mesh.vertices[5].position, d),
            "segundo triangulo do quad usa a, c, d"
        );

        return ok;
    }

    bool test_box_edges_builder() {
        using namespace locus::graphics;

        std::cout << "\n=== PrimitiveBuilder Box Edges ===\n";

        bool ok = true;

        PrimitiveBuilder builder{ PrimitiveTopology::Lines };

        const glm::vec3 firstCorner{
            1.0f,
            2.0f,
            3.0f
        };

        const glm::vec3 secondCorner{
            -1.0f,
            -2.0f,
            -3.0f
        };

        const ColorRGBA color{
            1.0f,
            0.75f,
            0.1f,
            1.0f
        };

        ok &= expect(
            builder.add_box_edges(
                firstCorner,
                secondCorner,
                color
            ),
            "add_box_edges aceita bounds invertidos"
        );

        ok &= expect(
            builder.vertex_count() == 24,
            "caixa possui doze arestas e vinte e quatro vertices"
        );

        ok &= expect(
            builder.mesh().is_valid(),
            "mesh de arestas da caixa e valida"
        );

        bool allColorsMatch = true;
        bool allNormalsAreZero = true;

        for (const PrimitiveVertex& vertex : builder.mesh().vertices) {
            allColorsMatch &= color_equal(vertex.color, color);

            allNormalsAreZero &= nearly_equal(
                vertex.normal,
                glm::vec3{ 0.0f }
            );
        }

        ok &= expect(
            allColorsMatch,
            "todas as arestas preservam a cor solicitada"
        );

        ok &= expect(
            allNormalsAreZero,
            "vertices das linhas usam normal neutra"
        );

        return ok;
    }

    bool test_explicit_triangle_vertices() {
        using namespace locus::graphics;

        std::cout << "\n=== Explicit PrimitiveVertex triangle ===\n";

        bool ok = true;

        PrimitiveBuilder builder{ PrimitiveTopology::Triangles };

        const PrimitiveVertex a{
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 1.0f, 0.0f, 0.0f },
            ColorRGBA{ 1.0f, 0.0f, 0.0f, 1.0f }
        };

        const PrimitiveVertex b{
            glm::vec3{ 1.0f, 0.0f, 0.0f },
            glm::vec3{ 0.0f, 1.0f, 0.0f },
            ColorRGBA{ 0.0f, 1.0f, 0.0f, 1.0f }
        };

        const PrimitiveVertex c{
            glm::vec3{ 0.0f, 1.0f, 0.0f },
            glm::vec3{ 0.0f, 0.0f, 1.0f },
            ColorRGBA{ 0.0f, 0.0f, 1.0f, 1.0f }
        };

        ok &= expect(
            builder.add_triangle(a, b, c),
            "add_triangle aceita PrimitiveVertex explicito"
        );

        ok &= expect(
            nearly_equal(builder.mesh().vertices[0].normal, a.normal)
            && nearly_equal(builder.mesh().vertices[1].normal, b.normal)
            && nearly_equal(builder.mesh().vertices[2].normal, c.normal),
            "add_triangle explicito preserva normais individuais"
        );

        ok &= expect(
            color_equal(builder.mesh().vertices[0].color, a.color)
            && color_equal(builder.mesh().vertices[1].color, b.color)
            && color_equal(builder.mesh().vertices[2].color, c.color),
            "add_triangle explicito preserva cores individuais"
        );

        return ok;
    }

    bool test_clear_preserves_topology() {
        using namespace locus::graphics;

        std::cout << "\n=== PrimitiveBuilder clear ===\n";

        bool ok = true;

        PrimitiveBuilder builder{ PrimitiveTopology::Lines };

        builder.add_line(
            glm::vec3{ 0.0f },
            glm::vec3{ 1.0f }
        );

        ok &= expect(
            !builder.is_empty(),
            "builder possui geometria antes de clear"
        );

        builder.clear();

        ok &= expect(
            builder.is_empty(),
            "clear remove a geometria acumulada"
        );

        ok &= expect(
            builder.vertex_count() == 0,
            "clear zera a quantidade de vertices"
        );

        ok &= expect(
            builder.topology() == PrimitiveTopology::Lines,
            "clear preserva a topologia"
        );

        ok &= expect(
            builder.add_line(
                glm::vec3{ 0.0f },
                glm::vec3{ 2.0f }
            ),
            "builder continua utilizavel depois de clear"
        );

        return ok;
    }

} // namespace

int main() {
    std::cout
        << "=== Locus3D Graphics PrimitiveBuilder Smoke Test ===\n";

    bool ok = true;

    ok &= test_primitive_mesh_validation();
    ok &= test_point_builder();
    ok &= test_line_builder();
    ok &= test_triangle_builder();
    ok &= test_degenerate_triangle();
    ok &= test_quad_builder();
    ok &= test_box_edges_builder();
    ok &= test_explicit_triangle_vertices();
    ok &= test_clear_preserves_topology();

    std::cout << "\n=== Resultado final ===\n";

    if (!ok) {
        std::cout
            << "[FAIL] Um ou mais testes de primitivas falharam.\n";

        return EXIT_FAILURE;
    }

    std::cout
        << "[OK] Todos os testes de primitivas passaram.\n";

    return EXIT_SUCCESS;
}