/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/snapping/AngleSnapProvider.h"
#include "editor/snapping/GridSnapProvider.h"
#include "editor/snapping/IncrementSnapProvider.h"
#include "editor/snapping/SnapContext.h"
#include "editor/snapping/SnapMode.h"
#include "editor/snapping/SnapResult.h"
#include "editor/snapping/SnapSettings.h"
#include "editor/snapping/SnapSolver.h"
#include "editor/snapping/SnapTarget.h"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include <glm/glm.hpp>

namespace {

    constexpr float kEpsilon = 0.0001f;

    bool almost_equal(float lhs, float rhs, float epsilon = kEpsilon)
    {
        const float diff = lhs - rhs;
        return diff < epsilon && diff > -epsilon;
    }

    bool almost_equal_vec3(
        const glm::vec3& lhs,
        const glm::vec3& rhs,
        float epsilon = kEpsilon)
    {
        return almost_equal(lhs.x, rhs.x, epsilon)
            && almost_equal(lhs.y, rhs.y, epsilon)
            && almost_equal(lhs.z, rhs.z, epsilon);
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

    bool expect_float(
        float actual,
        float expected,
        const std::string& message,
        float epsilon = kEpsilon)
    {
        if (almost_equal(actual, expected, epsilon)) {
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

    bool expect_vec3(
        const glm::vec3& actual,
        const glm::vec3& expected,
        const std::string& message,
        float epsilon = kEpsilon)
    {
        if (almost_equal_vec3(actual, expected, epsilon)) {
            std::cout
                << "[OK] " << message
                << " = (" << actual.x << ", " << actual.y << ", " << actual.z << ")"
                << '\n';

            return true;
        }

        std::cout
            << "[FAIL] " << message
            << " | actual=(" << actual.x << ", " << actual.y << ", " << actual.z << ")"
            << " expected=(" << expected.x << ", " << expected.y << ", " << expected.z << ")"
            << '\n';

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

    const char* snap_target_type_name(locus::editor::SnapTargetType type)
    {
        using locus::editor::SnapTargetType;

        switch (type) {
        case SnapTargetType::None:
            return "None";
        case SnapTargetType::GridPoint:
            return "GridPoint";
        case SnapTargetType::Vertex:
            return "Vertex";
        case SnapTargetType::Edge:
            return "Edge";
        case SnapTargetType::Face:
            return "Face";
        case SnapTargetType::Increment:
            return "Increment";
        case SnapTargetType::Angle:
            return "Angle";
        }

        return "Unknown";
    }

    void print_result(const locus::editor::SnapResult& result)
    {
        std::cout
            << "SnapResult"
            << " | valid: " << (result.valid ? "true" : "false")
            << " | target: " << snap_target_type_name(result.target.type)
            << " | original: ("
            << result.originalPosition.x << ", "
            << result.originalPosition.y << ", "
            << result.originalPosition.z << ")"
            << " | candidate: ("
            << result.candidatePosition.x << ", "
            << result.candidatePosition.y << ", "
            << result.candidatePosition.z << ")"
            << " | snapped: ("
            << result.snappedPosition.x << ", "
            << result.snappedPosition.y << ", "
            << result.snappedPosition.z << ")"
            << " | distance: " << result.distance
            << " | score: " << result.score
            << '\n';
    }

    locus::editor::SnapContext make_context(
        const glm::vec3& original,
        const glm::vec3& candidate,
        const glm::vec3& referenceOrigin = glm::vec3{ 0.0f, 0.0f, 0.0f })
    {
        locus::editor::SnapContext context{};
        context.originalPosition = original;
        context.candidatePosition = candidate;
        context.referenceOrigin = referenceOrigin;
        return context;
    }

    bool test_settings_defaults()
    {
        using namespace locus;

        std::cout << "\n=== SnapSettings: defaults ===\n";

        bool ok = true;

        editor::SnapSettings settings;

        ok &= expect(settings.snapping_enabled(), "snapping comeca ligado");
        ok &= expect(settings.is_enabled(editor::SnapMode::Grid), "grid comeca ligado");
        ok &= expect(settings.is_enabled(editor::SnapMode::Increment), "increment comeca ligado");
        ok &= expect(settings.is_enabled(editor::SnapMode::Angle), "angle comeca ligado");
        ok &= expect(!settings.is_enabled(editor::SnapMode::Vertex), "vertex comeca desligado");
        ok &= expect(!settings.is_enabled(editor::SnapMode::Edge), "edge comeca desligado");
        ok &= expect(!settings.is_enabled(editor::SnapMode::Face), "face comeca desligado");

        ok &= expect_float(settings.grid_size(), 1.0f, "grid_size default");
        ok &= expect_float(settings.linear_increment(), 1.0f, "linear_increment default");
        ok &= expect_float(settings.angle_increment(), 0.2617993878f, "angle_increment default");
        ok &= expect_float(settings.max_distance(), 0.25f, "max_distance default");

        return ok;
    }

    bool test_settings_enable_disable()
    {
        using namespace locus;

        std::cout << "\n=== SnapSettings: enable/disable ===\n";

        bool ok = true;

        editor::SnapSettings settings;
        settings.set_modes(editor::SnapMode::None);

        ok &= expect(!settings.is_enabled(editor::SnapMode::Grid), "grid desligado por set_modes None");

        settings.enable(editor::SnapMode::Grid);
        ok &= expect(settings.is_enabled(editor::SnapMode::Grid), "grid ligado por enable");

        settings.disable(editor::SnapMode::Grid);
        ok &= expect(!settings.is_enabled(editor::SnapMode::Grid), "grid desligado por disable");

        settings.set_modes(editor::SnapMode::Grid | editor::SnapMode::Vertex);
        ok &= expect(settings.is_enabled(editor::SnapMode::Grid), "grid ligado em mascara combinada");
        ok &= expect(settings.is_enabled(editor::SnapMode::Vertex), "vertex ligado em mascara combinada");
        ok &= expect(!settings.is_enabled(editor::SnapMode::Edge), "edge segue desligado");

        settings.set_snapping_enabled(false);
        ok &= expect(!settings.snapping_enabled(), "snapping global desligado");
        ok &= expect(!settings.is_enabled(editor::SnapMode::Grid), "grid nao roda com snapping global desligado");

        settings.set_snapping_enabled(true);
        ok &= expect(settings.is_enabled(editor::SnapMode::Grid), "grid volta ao ligar snapping global");

        return ok;
    }

    bool test_settings_clamps()
    {
        using namespace locus;

        std::cout << "\n=== SnapSettings: clamps ===\n";

        bool ok = true;

        editor::SnapSettings settings;

        settings.set_grid_size(-10.0f);
        settings.set_linear_increment(0.0f);
        settings.set_angle_increment(-1.0f);
        settings.set_max_distance(-5.0f);

        ok &= expect(settings.grid_size() > 0.0f, "grid_size negativo vira valor positivo minimo");
        ok &= expect(settings.linear_increment() > 0.0f, "linear_increment zero vira valor positivo minimo");
        ok &= expect(settings.angle_increment() > 0.0f, "angle_increment negativo vira valor positivo minimo");
        ok &= expect_float(settings.max_distance(), 0.0f, "max_distance negativo vira zero");

        return ok;
    }

    bool test_grid_snap_provider()
    {
        using namespace locus;

        std::cout << "\n=== GridSnapProvider ===\n";

        bool ok = true;

        editor::SnapSettings settings;
        settings.set_modes(editor::SnapMode::Grid);
        settings.set_grid_size(1.0f);

        editor::GridSnapProvider provider;
        const editor::SnapContext context = make_context(
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 1.21f, 2.76f, -0.49f });

        const editor::SnapResult result = provider.snap(settings, context);
        print_result(result);

        ok &= expect(result.is_valid(), "grid snap gerou resultado valido");
        ok &= expect(result.mode == editor::SnapMode::Grid, "modo do resultado eh Grid");
        ok &= expect(result.target.type == editor::SnapTargetType::GridPoint, "target eh GridPoint");
        ok &= expect_vec3(result.snappedPosition, glm::vec3{ 1.0f, 3.0f, 0.0f }, "posicao snapped no grid");
        ok &= expect_vec3(result.target.position, result.snappedPosition, "target position igual snapped position");

        return ok;
    }

    bool test_grid_snap_custom_step()
    {
        using namespace locus;

        std::cout << "\n=== GridSnapProvider: step 0.5 ===\n";

        bool ok = true;

        editor::SnapSettings settings;
        settings.set_modes(editor::SnapMode::Grid);
        settings.set_grid_size(0.5f);

        editor::GridSnapProvider provider;
        const editor::SnapContext context = make_context(
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 1.24f, 2.76f, -0.26f });

        const editor::SnapResult result = provider.snap(settings, context);
        print_result(result);

        ok &= expect(result.is_valid(), "grid snap com step 0.5 gerou resultado valido");
        ok &= expect_vec3(result.snappedPosition, glm::vec3{ 1.0f, 3.0f, -0.5f }, "posicao snapped no grid 0.5");

        return ok;
    }

    bool test_increment_snap_provider()
    {
        using namespace locus;

        std::cout << "\n=== IncrementSnapProvider ===\n";

        bool ok = true;

        editor::SnapSettings settings;
        settings.set_modes(editor::SnapMode::Increment);
        settings.set_linear_increment(0.5f);

        editor::IncrementSnapProvider provider;
        const editor::SnapContext context = make_context(
            glm::vec3{ 10.0f, 10.0f, 10.0f },
            glm::vec3{ 2.26f, 2.74f, 0.24f },
            glm::vec3{ 1.0f, 1.0f, 0.0f });

        const editor::SnapResult result = provider.snap(settings, context);
        print_result(result);

        ok &= expect(result.is_valid(), "increment snap gerou resultado valido");
        ok &= expect(result.mode == editor::SnapMode::Increment, "modo do resultado eh Increment");
        ok &= expect(result.target.type == editor::SnapTargetType::Increment, "target eh Increment");
        ok &= expect_vec3(result.snappedPosition, glm::vec3{ 2.5f, 2.5f, 0.0f }, "posicao snapped por incremento");

        return ok;
    }

    bool test_angle_snap_provider()
    {
        using namespace locus;

        std::cout << "\n=== AngleSnapProvider ===\n";

        bool ok = true;

        editor::SnapSettings settings;
        settings.set_modes(editor::SnapMode::Angle);
        settings.set_angle_increment(1.57079632679f);

        editor::AngleSnapProvider provider;
        const editor::SnapContext context = make_context(
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 1.0f, 0.30f, 2.0f },
            glm::vec3{ 0.0f, 0.0f, 0.0f });

        const editor::SnapResult result = provider.snap(settings, context);
        print_result(result);

        const float radius = glm::length(glm::vec2{ 1.0f, 0.30f });

        ok &= expect(result.is_valid(), "angle snap gerou resultado valido");
        ok &= expect(result.mode == editor::SnapMode::Angle, "modo do resultado eh Angle");
        ok &= expect(result.target.type == editor::SnapTargetType::Angle, "target eh Angle");
        ok &= expect_vec3(result.snappedPosition, glm::vec3{ radius, 0.0f, 2.0f }, "posicao snapped por angulo");

        return ok;
    }

    bool test_angle_snap_zero_radius_returns_none()
    {
        using namespace locus;

        std::cout << "\n=== AngleSnapProvider: raio zero ===\n";

        bool ok = true;

        editor::SnapSettings settings;
        settings.set_modes(editor::SnapMode::Angle);
        settings.set_angle_increment(1.57079632679f);

        editor::AngleSnapProvider provider;
        const editor::SnapContext context = make_context(
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 1.0f, 1.0f, 5.0f },
            glm::vec3{ 1.0f, 1.0f, 0.0f });

        const editor::SnapResult result = provider.snap(settings, context);
        print_result(result);

        ok &= expect(!result.is_valid(), "angle snap com raio zero retorna invalid");

        return ok;
    }

    bool test_solver_empty()
    {
        using namespace locus;

        std::cout << "\n=== SnapSolver: vazio ===\n";

        bool ok = true;

        editor::SnapSettings settings;
        editor::SnapSolver solver;

        const editor::SnapContext context = make_context(
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 1.2f, 2.7f, 0.0f });

        const editor::SnapResult result = solver.solve(settings, context);
        print_result(result);

        ok &= expect_size(solver.provider_count(), 0, "provider_count");
        ok &= expect(!result.is_valid(), "solver vazio retorna invalid");

        return ok;
    }

    bool test_solver_disabled()
    {
        using namespace locus;

        std::cout << "\n=== SnapSolver: snapping global desligado ===\n";

        bool ok = true;

        editor::SnapSettings settings;
        settings.set_snapping_enabled(false);

        editor::SnapSolver solver;
        solver.register_provider(std::make_unique<editor::GridSnapProvider>());

        const editor::SnapContext context = make_context(
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 1.2f, 2.7f, 0.0f });

        const editor::SnapResult result = solver.solve(settings, context);
        print_result(result);

        ok &= expect_size(solver.provider_count(), 1, "provider_count");
        ok &= expect(!result.is_valid(), "solver nao roda com snapping global desligado");

        return ok;
    }

    bool test_solver_rejects_by_max_distance()
    {
        using namespace locus;

        std::cout << "\n=== SnapSolver: rejeita por max distance ===\n";

        bool ok = true;

        editor::SnapSettings settings;
        settings.set_modes(editor::SnapMode::Grid);
        settings.set_grid_size(1.0f);
        settings.set_max_distance(0.10f);

        editor::SnapSolver solver;
        solver.register_provider(std::make_unique<editor::GridSnapProvider>());

        const editor::SnapContext context = make_context(
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 0.49f, 0.49f, 0.0f });

        const editor::SnapResult result = solver.solve(settings, context);
        print_result(result);

        ok &= expect(!result.is_valid(), "resultado distante demais foi rejeitado");

        return ok;
    }

    bool test_solver_accepts_by_context_override()
    {
        using namespace locus;

        std::cout << "\n=== SnapSolver: max distance override ===\n";

        bool ok = true;

        editor::SnapSettings settings;
        settings.set_modes(editor::SnapMode::Grid);
        settings.set_grid_size(1.0f);
        settings.set_max_distance(0.10f);

        editor::SnapSolver solver;
        solver.register_provider(std::make_unique<editor::GridSnapProvider>());

        editor::SnapContext context = make_context(
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 0.49f, 0.49f, 0.0f });

        context.maxDistanceOverride = 1.0f;

        const editor::SnapResult result = solver.solve(settings, context);
        print_result(result);

        ok &= expect(result.is_valid(), "override permitiu aceitar resultado");
        ok &= expect_vec3(result.snappedPosition, glm::vec3{ 0.0f, 0.0f, 0.0f }, "posicao snapped aceita por override");

        return ok;
    }

    bool test_solver_chooses_best_result()
    {
        using namespace locus;

        std::cout << "\n=== SnapSolver: escolhe melhor resultado ===\n";

        bool ok = true;

        editor::SnapSettings settings;
        settings.set_modes(editor::SnapMode::Grid | editor::SnapMode::Increment);
        settings.set_grid_size(1.0f);
        settings.set_linear_increment(0.25f);
        settings.set_max_distance(10.0f);

        editor::SnapSolver solver;
        solver.register_provider(std::make_unique<editor::GridSnapProvider>());
        solver.register_provider(std::make_unique<editor::IncrementSnapProvider>());

        const editor::SnapContext context = make_context(
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 0.26f, 0.26f, 0.0f },
            glm::vec3{ 0.0f, 0.0f, 0.0f });

        const editor::SnapResult result = solver.solve(settings, context);
        print_result(result);

        ok &= expect(result.is_valid(), "solver encontrou resultado valido");
        ok &= expect(result.mode == editor::SnapMode::Increment, "increment venceu por estar mais perto");
        ok &= expect(result.target.type == editor::SnapTargetType::Increment, "target vencedor eh Increment");
        ok &= expect_vec3(result.snappedPosition, glm::vec3{ 0.25f, 0.25f, 0.0f }, "posicao snapped vencedora");

        return ok;
    }

    bool test_solver_respects_enabled_modes()
    {
        using namespace locus;

        std::cout << "\n=== SnapSolver: respeita modos habilitados ===\n";

        bool ok = true;

        editor::SnapSettings settings;
        settings.set_modes(editor::SnapMode::Grid);
        settings.set_grid_size(1.0f);
        settings.set_linear_increment(0.25f);
        settings.set_max_distance(10.0f);

        editor::SnapSolver solver;
        solver.register_provider(std::make_unique<editor::GridSnapProvider>());
        solver.register_provider(std::make_unique<editor::IncrementSnapProvider>());

        const editor::SnapContext context = make_context(
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 0.26f, 0.26f, 0.0f },
            glm::vec3{ 0.0f, 0.0f, 0.0f });

        const editor::SnapResult result = solver.solve(settings, context);
        print_result(result);

        ok &= expect(result.is_valid(), "solver encontrou resultado valido");
        ok &= expect(result.mode == editor::SnapMode::Grid, "grid venceu porque increment esta desabilitado");
        ok &= expect(result.target.type == editor::SnapTargetType::GridPoint, "target vencedor eh GridPoint");
        ok &= expect_vec3(result.snappedPosition, glm::vec3{ 0.0f, 0.0f, 0.0f }, "posicao snapped por grid");

        return ok;
    }

    bool test_solver_clear()
    {
        using namespace locus;

        std::cout << "\n=== SnapSolver: clear ===\n";

        bool ok = true;

        editor::SnapSolver solver;
        ok &= expect(solver.register_provider(std::make_unique<editor::GridSnapProvider>()), "registrou GridSnapProvider");
        ok &= expect(solver.register_provider(std::make_unique<editor::IncrementSnapProvider>()), "registrou IncrementSnapProvider");
        ok &= expect_size(solver.provider_count(), 2, "provider_count antes do clear");

        solver.clear();

        ok &= expect_size(solver.provider_count(), 0, "provider_count depois do clear");

        return ok;
    }

} // namespace

int main()
{
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "=== Locus3D Editor Snapping Smoke Test ===\n";

    bool ok = true;

    ok &= test_settings_defaults();
    ok &= test_settings_enable_disable();
    ok &= test_settings_clamps();

    ok &= test_grid_snap_provider();
    ok &= test_grid_snap_custom_step();
    ok &= test_increment_snap_provider();
    ok &= test_angle_snap_provider();
    ok &= test_angle_snap_zero_radius_returns_none();

    ok &= test_solver_empty();
    ok &= test_solver_disabled();
    ok &= test_solver_rejects_by_max_distance();
    ok &= test_solver_accepts_by_context_override();
    ok &= test_solver_chooses_best_result();
    ok &= test_solver_respects_enabled_modes();
    ok &= test_solver_clear();

    std::cout << "\n=== Resultado final ===\n";

    if (ok) {
        std::cout << "[OK] Todos os testes de snapping passaram.\n";
        return EXIT_SUCCESS;
    }

    std::cout << "[FAIL] Algum teste de snapping falhou.\n";
    return EXIT_FAILURE;
}