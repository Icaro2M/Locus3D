/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/primitives/PrimitiveBuilder.h"
#include "graphics/primitives/PrimitiveMeshConverter.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

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

    bool mesh_vertex_position_equals(
        const locus::graphics::MeshVertex& vertex,
        const glm::vec3& expected
    ) {
        return nearly_equal(vertex.position[0], expected.x)
            && nearly_equal(vertex.position[1], expected.y)
            && nearly_equal(vertex.position[2], expected.z);
    }

    bool mesh_vertex_normal_equals(
        const locus::graphics::MeshVertex& vertex,
        const glm::vec3& expected
    ) {
        return nearly_equal(vertex.normal[0], expected.x)
            && nearly_equal(vertex.normal[1], expected.y)
            && nearly_equal(vertex.normal[2], expected.z);
    }

    bool mesh_vertex_color_equals(
        const locus::graphics::MeshVertex& vertex,
        const locus::graphics::ColorRGBA& expected
    ) {
        return nearly_equal(vertex.color[0], expected.r)
            && nearly_equal(vertex.color[1], expected.g)
            && nearly_equal(vertex.color[2], expected.b)
            && nearly_equal(vertex.color[3], expected.a);
    }

    bool test_empty_mesh_conversion() {
        using namespace locus::graphics;

        std::cout << "\n=== Empty PrimitiveMesh conversion ===\n";

        bool ok = true;

        PrimitiveMesh primitiveMesh;
        primitiveMesh.topology = PrimitiveTopology::Lines;

        const MeshUploadData uploadData =
            PrimitiveMeshConverter::to_upload_data(
                primitiveMesh,
                BufferUsage::Stream
            );

        ok &= expect(
            uploadData.vertices.empty(),
            "mesh vazia produz vertices vazios"
        );

        ok &= expect(
            uploadData.indices.empty(),
            "mesh vazia produz indices vazios"
        );

        ok &= expect(
            uploadData.topology == PrimitiveTopology::Lines,
            "mesh vazia preserva topologia"
        );

        ok &= expect(
            uploadData.usage == BufferUsage::Stream,
            "mesh vazia preserva BufferUsage solicitado"
        );

        return ok;
    }

    bool test_point_conversion() {
        using namespace locus::graphics;

        std::cout << "\n=== Point conversion ===\n";

        bool ok = true;

        PrimitiveBuilder builder{ PrimitiveTopology::Points };

        const glm::vec3 position{
            1.5f,
            -2.0f,
            4.25f
        };

        const ColorRGBA color{
            0.25f,
            0.5f,
            0.75f,
            0.8f
        };

        builder.add_point(position, color);

        const PrimitiveMesh primitiveMesh = builder.build();

        const MeshUploadData uploadData =
            PrimitiveMeshConverter::to_upload_data(primitiveMesh);

        ok &= expect(
            uploadData.vertices.size() == 1,
            "ponto produz um MeshVertex"
        );

        ok &= expect(
            uploadData.indices.empty(),
            "ponto nao indexado preserva indices vazios"
        );

        ok &= expect(
            uploadData.topology == PrimitiveTopology::Points,
            "ponto preserva PrimitiveTopology::Points"
        );

        ok &= expect(
            uploadData.usage == BufferUsage::Dynamic,
            "conversao usa Dynamic por padrao"
        );

        if (!uploadData.vertices.empty()) {
            const MeshVertex& vertex = uploadData.vertices[0];

            ok &= expect(
                mesh_vertex_position_equals(vertex, position),
                "posicao do ponto e convertida corretamente"
            );

            ok &= expect(
                mesh_vertex_normal_equals(
                    vertex,
                    glm::vec3{ 0.0f }
                ),
                "normal neutra do ponto e preservada"
            );

            ok &= expect(
                mesh_vertex_color_equals(vertex, color),
                "cor do ponto e convertida corretamente"
            );
        }

        return ok;
    }

    bool test_line_conversion() {
        using namespace locus::graphics;

        std::cout << "\n=== Line conversion ===\n";

        bool ok = true;

        PrimitiveBuilder builder{ PrimitiveTopology::Lines };

        const glm::vec3 start{
            -3.0f,
            1.0f,
            2.0f
        };

        const glm::vec3 end{
            5.0f,
            -1.0f,
            7.0f
        };

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
            0.5f
        };

        builder.add_line(
            start,
            end,
            startColor,
            endColor
        );

        const MeshUploadData uploadData =
            PrimitiveMeshConverter::to_upload_data(
                builder.build(),
                BufferUsage::Stream
            );

        ok &= expect(
            uploadData.vertices.size() == 2,
            "linha produz dois MeshVertex"
        );

        ok &= expect(
            uploadData.indices.empty(),
            "linha nao indexada preserva indices vazios"
        );

        ok &= expect(
            uploadData.topology == PrimitiveTopology::Lines,
            "linha preserva PrimitiveTopology::Lines"
        );

        ok &= expect(
            uploadData.usage == BufferUsage::Stream,
            "linha preserva BufferUsage::Stream"
        );

        if (uploadData.vertices.size() == 2) {
            ok &= expect(
                mesh_vertex_position_equals(
                    uploadData.vertices[0],
                    start
                ),
                "vertice inicial preserva posicao"
            );

            ok &= expect(
                mesh_vertex_position_equals(
                    uploadData.vertices[1],
                    end
                ),
                "vertice final preserva posicao"
            );

            ok &= expect(
                mesh_vertex_normal_equals(
                    uploadData.vertices[0],
                    glm::vec3{ 0.0f }
                )
                && mesh_vertex_normal_equals(
                    uploadData.vertices[1],
                    glm::vec3{ 0.0f }
                ),
                "linha preserva normais neutras"
            );

            ok &= expect(
                mesh_vertex_color_equals(
                    uploadData.vertices[0],
                    startColor
                ),
                "vertice inicial preserva cor"
            );

            ok &= expect(
                mesh_vertex_color_equals(
                    uploadData.vertices[1],
                    endColor
                ),
                "vertice final preserva cor"
            );
        }

        return ok;
    }

    bool test_triangle_conversion() {
        using namespace locus::graphics;

        std::cout << "\n=== Triangle conversion ===\n";

        bool ok = true;

        PrimitiveBuilder builder{
            PrimitiveTopology::Triangles
        };

        const glm::vec3 a{
            0.0f,
            0.0f,
            0.0f
        };

        const glm::vec3 b{
            1.0f,
            0.0f,
            0.0f
        };

        const glm::vec3 c{
            0.0f,
            1.0f,
            0.0f
        };

        const ColorRGBA color{
            0.2f,
            0.8f,
            0.4f,
            1.0f
        };

        builder.add_triangle(a, b, c, color);

        const MeshUploadData uploadData =
            PrimitiveMeshConverter::to_upload_data(
                builder.build(),
                BufferUsage::Static
            );

        ok &= expect(
            uploadData.vertices.size() == 3,
            "triangulo produz tres MeshVertex"
        );

        ok &= expect(
            uploadData.indices.empty(),
            "triangulo do builder permanece nao indexado"
        );

        ok &= expect(
            uploadData.topology
            == PrimitiveTopology::Triangles,
            "triangulo preserva PrimitiveTopology::Triangles"
        );

        ok &= expect(
            uploadData.usage == BufferUsage::Static,
            "triangulo preserva BufferUsage::Static"
        );

        const glm::vec3 expectedNormal{
            0.0f,
            0.0f,
            1.0f
        };

        if (uploadData.vertices.size() == 3) {
            ok &= expect(
                mesh_vertex_position_equals(
                    uploadData.vertices[0],
                    a
                )
                && mesh_vertex_position_equals(
                    uploadData.vertices[1],
                    b
                )
                && mesh_vertex_position_equals(
                    uploadData.vertices[2],
                    c
                ),
                "triangulo preserva as tres posicoes"
            );

            ok &= expect(
                mesh_vertex_normal_equals(
                    uploadData.vertices[0],
                    expectedNormal
                )
                && mesh_vertex_normal_equals(
                    uploadData.vertices[1],
                    expectedNormal
                )
                && mesh_vertex_normal_equals(
                    uploadData.vertices[2],
                    expectedNormal
                ),
                "triangulo preserva a normal calculada"
            );

            ok &= expect(
                mesh_vertex_color_equals(
                    uploadData.vertices[0],
                    color
                )
                && mesh_vertex_color_equals(
                    uploadData.vertices[1],
                    color
                )
                && mesh_vertex_color_equals(
                    uploadData.vertices[2],
                    color
                ),
                "triangulo preserva as cores"
            );
        }

        return ok;
    }

    bool test_indexed_mesh_conversion() {
        using namespace locus::graphics;

        std::cout << "\n=== Indexed PrimitiveMesh conversion ===\n";

        bool ok = true;

        const ColorRGBA colorA{
            1.0f,
            0.0f,
            0.0f,
            1.0f
        };

        const ColorRGBA colorB{
            0.0f,
            1.0f,
            0.0f,
            1.0f
        };

        const ColorRGBA colorC{
            0.0f,
            0.0f,
            1.0f,
            1.0f
        };

        PrimitiveMesh primitiveMesh;
        primitiveMesh.topology =
            PrimitiveTopology::Triangles;

        primitiveMesh.vertices = {
            PrimitiveVertex{
                glm::vec3{ 0.0f, 0.0f, 0.0f },
                glm::vec3{ 0.0f, 0.0f, 1.0f },
                colorA
            },
            PrimitiveVertex{
                glm::vec3{ 1.0f, 0.0f, 0.0f },
                glm::vec3{ 0.0f, 0.0f, 1.0f },
                colorB
            },
            PrimitiveVertex{
                glm::vec3{ 0.0f, 1.0f, 0.0f },
                glm::vec3{ 0.0f, 0.0f, 1.0f },
                colorC
            }
        };

        primitiveMesh.indices = {
            0,
            1,
            2
        };

        ok &= expect(
            primitiveMesh.is_valid(),
            "PrimitiveMesh indexada de entrada e valida"
        );

        const MeshUploadData uploadData =
            PrimitiveMeshConverter::to_upload_data(
                primitiveMesh
            );

        ok &= expect(
            uploadData.vertices.size()
            == primitiveMesh.vertices.size(),
            "mesh indexada preserva quantidade de vertices"
        );

        ok &= expect(
            uploadData.indices
            == primitiveMesh.indices,
            "mesh indexada preserva os indices"
        );

        ok &= expect(
            uploadData.topology
            == PrimitiveTopology::Triangles,
            "mesh indexada preserva topologia"
        );

        if (uploadData.vertices.size() == 3) {
            ok &= expect(
                mesh_vertex_color_equals(
                    uploadData.vertices[0],
                    colorA
                )
                && mesh_vertex_color_equals(
                    uploadData.vertices[1],
                    colorB
                )
                && mesh_vertex_color_equals(
                    uploadData.vertices[2],
                    colorC
                ),
                "mesh indexada preserva cores individuais"
            );
        }

        return ok;
    }

    bool test_source_mesh_is_not_modified() {
        using namespace locus::graphics;

        std::cout << "\n=== Source mesh immutability ===\n";

        bool ok = true;

        PrimitiveBuilder builder{ PrimitiveTopology::Lines };

        builder.add_line(
            glm::vec3{ -1.0f, 0.0f, 0.0f },
            glm::vec3{ 1.0f, 0.0f, 0.0f }
        );

        const PrimitiveMesh primitiveMesh = builder.build();

        const std::size_t originalVertexCount =
            primitiveMesh.vertices.size();

        const std::size_t originalIndexCount =
            primitiveMesh.indices.size();

        const PrimitiveTopology originalTopology =
            primitiveMesh.topology;

        const MeshUploadData uploadData =
            PrimitiveMeshConverter::to_upload_data(
                primitiveMesh
            );

        static_cast<void>(uploadData);

        ok &= expect(
            primitiveMesh.vertices.size()
            == originalVertexCount,
            "conversao nao altera vertices de origem"
        );

        ok &= expect(
            primitiveMesh.indices.size()
            == originalIndexCount,
            "conversao nao altera indices de origem"
        );

        ok &= expect(
            primitiveMesh.topology
            == originalTopology,
            "conversao nao altera topologia de origem"
        );

        return ok;
    }

} // namespace

int main() {
    std::cout
        << "=== Locus3D Graphics PrimitiveMeshConverter "
        "Smoke Test ===\n";

    bool ok = true;

    ok &= test_empty_mesh_conversion();
    ok &= test_point_conversion();
    ok &= test_line_conversion();
    ok &= test_triangle_conversion();
    ok &= test_indexed_mesh_conversion();
    ok &= test_source_mesh_is_not_modified();

    std::cout << "\n=== Resultado final ===\n";

    if (!ok) {
        std::cout
            << "[FAIL] Um ou mais testes do converter "
            "falharam.\n";

        return EXIT_FAILURE;
    }

    std::cout
        << "[OK] Todos os testes do "
        "PrimitiveMeshConverter passaram.\n";

    return EXIT_SUCCESS;
}