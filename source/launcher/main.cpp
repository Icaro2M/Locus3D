/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/gizmo/GizmoAxis.h"
#include "editor/gizmo/GizmoConstraint.h"
#include "editor/gizmo/GizmoHit.h"
#include "editor/gizmo/GizmoMode.h"
#include "editor/gizmo/GizmoSnap.h"
#include "editor/gizmo/GizmoState.h"
#include "editor/snapping/SnapSettings.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cmath>
#include <iostream>
#include <string>

namespace {

    using namespace locus::editor;

    bool nearly_equal(float a, float b, float epsilon = 0.0001f)
    {
        return std::abs(a - b) <= epsilon;
    }

    bool nearly_equal_vec3(const glm::vec3& a, const glm::vec3& b, float epsilon = 0.0001f)
    {
        return nearly_equal(a.x, b.x, epsilon)
            && nearly_equal(a.y, b.y, epsilon)
            && nearly_equal(a.z, b.z, epsilon);
    }

    void print_vec3(const std::string& label, const glm::vec3& value)
    {
        std::cout << label << ": "
            << value.x << ", "
            << value.y << ", "
            << value.z << '\n';
    }

    void print_result(bool condition, const std::string& message)
    {
        std::cout << (condition ? "[OK] " : "[FAIL] ") << message << '\n';
    }

    GizmoRay make_ray(const glm::vec3& origin, const glm::vec3& direction)
    {
        GizmoRay ray{};
        ray.origin = origin;
        ray.direction = direction;
        return ray;
    }

    bool test_axis_helpers()
    {
        std::cout << "\n=== GizmoAxis helpers ===\n";

        bool ok = true;

        const bool xIsSingle = is_gizmo_single_axis(GizmoAxis::X);
        const bool xyIsPlane = is_gizmo_plane_axis(GizmoAxis::XY);
        const bool xyzIsFree = is_gizmo_free_axis(GizmoAxis::XYZ);
        const bool viewIsNotPlane = !is_gizmo_plane_axis(GizmoAxis::View);

        print_result(xIsSingle, "X reconhecido como eixo simples");
        print_result(xyIsPlane, "XY reconhecido como plano");
        print_result(xyzIsFree, "XYZ reconhecido como manipulacao livre");
        print_result(viewIsNotPlane, "View nao e tratado como plano cartesiano direto");

        ok = ok && xIsSingle && xyIsPlane && xyzIsFree && viewIsNotPlane;
        return ok;
    }

    bool test_gizmo_hit_and_state()
    {
        std::cout << "\n=== GizmoHit / GizmoState ===\n";

        bool ok = true;

        GizmoHit invalid = GizmoHit::none();
        print_result(!invalid.is_valid(), "GizmoHit::none retorna hit invalido");
        ok = ok && !invalid.is_valid();

        GizmoHit hit = GizmoHit::make(
            GizmoMode::Translate,
            GizmoAxis::X,
            glm::vec3{ 1.0f, 2.0f, 3.0f },
            0.15f,
            4.0f);

        const bool validHit = hit.is_valid()
            && hit.mode == GizmoMode::Translate
            && hit.axis == GizmoAxis::X
            && nearly_equal_vec3(hit.worldPosition, glm::vec3{ 1.0f, 2.0f, 3.0f });

        print_result(validHit, "GizmoHit::make cria hit valido");
        ok = ok && validHit;

        GizmoState state{};
        state.hovered = hit;
        state.active = hit;
        state.dragging = true;

        print_result(state.can_interact(), "GizmoState pode interagir quando enabled e visible");
        ok = ok && state.can_interact();

        state.clear_hover();
        print_result(!state.hovered.is_valid(), "clear_hover limpa hovered");
        ok = ok && !state.hovered.is_valid();

        state.clear_active();
        print_result(!state.active.is_valid() && !state.dragging, "clear_active limpa active e dragging");
        ok = ok && !state.active.is_valid() && !state.dragging;

        state.enabled = false;
        print_result(!state.can_interact(), "GizmoState nao interage quando disabled");
        ok = ok && !state.can_interact();

        return ok;
    }

    bool test_translation_axis()
    {
        std::cout << "\n=== GizmoConstraint: translate X ===\n";

        GizmoConstraintInput input{};
        input.mode = GizmoMode::Translate;
        input.axis = GizmoAxis::X;
        input.pivot = glm::vec3{ 0.0f, 0.0f, 0.0f };
        input.startPoint = glm::vec3{ 0.0f, 0.0f, 0.0f };
        input.startRay = make_ray(glm::vec3{ 0.0f, 1.0f, 0.0f }, glm::vec3{ 0.0f, -1.0f, 0.0f });
        input.currentRay = make_ray(glm::vec3{ 2.0f, 1.0f, 0.0f }, glm::vec3{ 0.0f, -1.0f, 0.0f });

        const GizmoConstraintResult result = GizmoConstraint::solve_translation(input);

        print_result(result.is_valid(), "resultado valido");
        print_vec3("translation", result.translation);

        const bool ok = result.is_valid()
            && nearly_equal_vec3(result.translation, glm::vec3{ 2.0f, 0.0f, 0.0f })
            && nearly_equal(result.signedAmount, 2.0f);

        print_result(ok, "translate X gerou delta esperado");
        return ok;
    }

    bool test_translation_plane()
    {
        std::cout << "\n=== GizmoConstraint: translate XY ===\n";

        GizmoConstraintInput input{};
        input.mode = GizmoMode::Translate;
        input.axis = GizmoAxis::XY;
        input.pivot = glm::vec3{ 0.0f, 0.0f, 0.0f };
        input.startPoint = glm::vec3{ 0.0f, 0.0f, 0.0f };
        input.currentRay = make_ray(glm::vec3{ 1.5f, -2.0f, 5.0f }, glm::vec3{ 0.0f, 0.0f, -1.0f });

        const GizmoConstraintResult result = GizmoConstraint::solve_translation(input);

        print_result(result.is_valid(), "resultado valido");
        print_vec3("translation", result.translation);

        const bool ok = result.is_valid()
            && nearly_equal_vec3(result.translation, glm::vec3{ 1.5f, -2.0f, 0.0f });

        print_result(ok, "translate XY manteve movimento no plano XY");
        return ok;
    }

    bool test_scale_axis()
    {
        std::cout << "\n=== GizmoConstraint: scale X ===\n";

        GizmoConstraintInput input{};
        input.mode = GizmoMode::Scale;
        input.axis = GizmoAxis::X;
        input.pivot = glm::vec3{ 0.0f, 0.0f, 0.0f };
        input.startPoint = glm::vec3{ 0.0f, 0.0f, 0.0f };
        input.currentRay = make_ray(glm::vec3{ 2.0f, 1.0f, 0.0f }, glm::vec3{ 0.0f, -1.0f, 0.0f });
        input.scaleSensitivity = 1.0f;

        const GizmoConstraintResult result = GizmoConstraint::solve_scale(input);

        print_result(result.is_valid(), "resultado valido");
        print_vec3("scale", result.scale);

        const bool ok = result.is_valid()
            && nearly_equal_vec3(result.scale, glm::vec3{ 3.0f, 1.0f, 1.0f });

        print_result(ok, "scale X gerou fator esperado");
        return ok;
    }

    bool test_scale_plane()
    {
        std::cout << "\n=== GizmoConstraint: scale XY ===\n";

        GizmoConstraintInput input{};
        input.mode = GizmoMode::Scale;
        input.axis = GizmoAxis::XY;
        input.pivot = glm::vec3{ 0.0f, 0.0f, 0.0f };
        input.startPoint = glm::vec3{ 0.0f, 0.0f, 0.0f };
        input.currentRay = make_ray(glm::vec3{ 1.0f, 1.0f, 5.0f }, glm::vec3{ 0.0f, 0.0f, -1.0f });
        input.viewRight = glm::vec3{ 1.0f, 0.0f, 0.0f };
        input.viewUp = glm::vec3{ 0.0f, 1.0f, 0.0f };
        input.scaleSensitivity = 1.0f;

        const GizmoConstraintResult result = GizmoConstraint::solve_scale(input);

        print_result(result.is_valid(), "resultado valido");
        print_vec3("scale", result.scale);

        const bool ok = result.is_valid()
            && nearly_equal_vec3(result.scale, glm::vec3{ 2.0f, 2.0f, 1.0f });

        print_result(ok, "scale XY gerou fator em X e Y");
        return ok;
    }

    bool test_rotation_z()
    {
        std::cout << "\n=== GizmoConstraint: rotate Z ===\n";

        GizmoConstraintInput input{};
        input.mode = GizmoMode::Rotate;
        input.axis = GizmoAxis::Z;
        input.pivot = glm::vec3{ 0.0f, 0.0f, 0.0f };
        input.startPoint = glm::vec3{ 1.0f, 0.0f, 0.0f };
        input.currentRay = make_ray(glm::vec3{ 0.0f, 1.0f, 5.0f }, glm::vec3{ 0.0f, 0.0f, -1.0f });
        input.rotationSensitivity = 1.0f;

        const GizmoConstraintResult result = GizmoConstraint::solve_rotation(input);

        print_result(result.is_valid(), "resultado valido");
        std::cout << "angle rad: " << result.angle << '\n';

        const glm::vec3 rotated = result.rotation * glm::vec3{ 1.0f, 0.0f, 0.0f };
        print_vec3("rotated X", rotated);

        const bool ok = result.is_valid()
            && nearly_equal(result.angle, glm::half_pi<float>())
            && nearly_equal_vec3(rotated, glm::vec3{ 0.0f, 1.0f, 0.0f }, 0.001f);

        print_result(ok, "rotate Z gerou 90 graus no sentido esperado");
        return ok;
    }

    bool test_snap_angle()
    {
        std::cout << "\n=== GizmoSnap: angle ===\n";

        SnapSettings settings{};
        settings.set_snapping_enabled(true);
        settings.set_angle_increment(glm::quarter_pi<float>());

        const float inputAngle = glm::radians(50.0f);
        const float snapped = GizmoSnap::snap_angle(inputAngle, settings);

        std::cout << "input deg: 50\n";
        std::cout << "snapped deg: " << glm::degrees(snapped) << '\n';

        const bool ok = nearly_equal(snapped, glm::quarter_pi<float>());
        print_result(ok, "50 graus arredondou para 45 graus com incremento de 45");

        return ok;
    }

} // namespace

int main()
{
    std::cout << "=== Locus3D Editor Gizmo Core Smoke Test ===\n";

    bool ok = true;

    ok = test_axis_helpers() && ok;
    ok = test_gizmo_hit_and_state() && ok;
    ok = test_translation_axis() && ok;
    ok = test_translation_plane() && ok;
    ok = test_scale_axis() && ok;
    ok = test_scale_plane() && ok;
    ok = test_rotation_z() && ok;
    ok = test_snap_angle() && ok;

    std::cout << "\n=== Resultado final ===\n";
    print_result(ok, "Gizmo core smoke test");

    return ok ? 0 : 1;
}