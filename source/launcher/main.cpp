
#include "kernel/math/Bounds.h"
#include "kernel/math/GeometryMath.h"
#include "kernel/math/Mat.h"
#include "kernel/math/Quaternion.h"
#include "kernel/math/Transform.h"
#include "kernel/math/Vec.h"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

	using namespace locus::kernel;
	using namespace locus::kernel::math;

	void print_vec2(const Vec2& value)
	{
		std::cout << "(" << value.x << ", " << value.y << ")";
	}

	void print_vec3(const Vec3& value)
	{
		std::cout << "(" << value.x << ", " << value.y << ", " << value.z << ")";
	}

	void print_quaternion(const Quaternion& value)
	{
		std::cout << "(w=" << value.w << ", x=" << value.x << ", y=" << value.y << ", z=" << value.z << ")";
	}

	bool expect(bool condition, const std::string& message)
	{
		if (condition) {
			std::cout << "[OK] " << message << "\n";
			return true;
		}

		std::cout << "[FAIL] " << message << "\n";
		return false;
	}

	bool test_vec()
	{
		std::cout << "\n=== Vec.h ===\n";

		bool passed = true;

		const Vec3 xAxis = axis_vector(Axis::X);
		const Vec3 negativeY = axis_vector(Axis::Y, Orientation::Negative);
		const Vec3 zAxis = axis_vector(Axis::Z);

		std::cout << "xAxis: ";
		print_vec3(xAxis);
		std::cout << "\nnegativeY: ";
		print_vec3(negativeY);
		std::cout << "\nzAxis: ";
		print_vec3(zAxis);
		std::cout << "\n";

		passed &= expect(nearly_equal(xAxis, Vec3{ 1.0f, 0.0f, 0.0f }), "axis_vector(Axis::X)");
		passed &= expect(nearly_equal(negativeY, Vec3{ 0.0f, -1.0f, 0.0f }), "axis_vector(Axis::Y, Negative)");
		passed &= expect(axis_index(Axis::Z) == 2, "axis_index(Axis::Z)");

		Vec3 editable{ 1.0f, 2.0f, 3.0f };
		set_component(editable, Axis::Y, 10.0f);

		std::cout << "editable apos set_component Y: ";
		print_vec3(editable);
		std::cout << "\n";

		passed &= expect(nearly_equal(component(editable, Axis::Y), 10.0f), "component/set_component por eixo");

		const Vec3 a{ 1.0f, 2.0f, 3.0f };
		const Vec3 b{ 4.0f, 6.0f, 3.0f };

		std::cout << "distance_squared(a, b): " << distance_squared(a, b) << "\n";

		passed &= expect(nearly_equal(distance_squared(a, b), 25.0f), "distance_squared(Vec3)");

		const Vec3 vector{ 2.0f, 2.0f, 0.0f };
		const Vec3 direction{ 1.0f, 0.0f, 0.0f };
		const Vec3 projected = project(vector, direction);
		const Vec3 rejected = reject(vector, direction);

		std::cout << "project((2,2,0), X): ";
		print_vec3(projected);
		std::cout << "\nreject((2,2,0), X): ";
		print_vec3(rejected);
		std::cout << "\n";

		passed &= expect(nearly_equal(projected, Vec3{ 2.0f, 0.0f, 0.0f }), "project(Vec3)");
		passed &= expect(nearly_equal(rejected, Vec3{ 0.0f, 2.0f, 0.0f }), "reject(Vec3)");

		const Axis dominant = dominant_axis(Vec3{ 1.0f, -5.0f, 2.0f });
		passed &= expect(dominant == Axis::Y, "dominant_axis(Vec3)");

		const Vec2 normalized = safe_normalize(Vec2{ 3.0f, 4.0f });
		const Vec2 fallback = safe_normalize(Vec2{ 0.0f, 0.0f }, Vec2{ 1.0f, 0.0f });

		std::cout << "safe_normalize((3,4)): ";
		print_vec2(normalized);
		std::cout << "\nsafe_normalize(zero, fallback): ";
		print_vec2(fallback);
		std::cout << "\n";

		passed &= expect(nearly_equal(normalized, Vec2{ 0.6f, 0.8f }), "safe_normalize(Vec2)");
		passed &= expect(nearly_equal(fallback, Vec2{ 1.0f, 0.0f }), "safe_normalize(Vec2) fallback");

		const Vec3 clamped = clamp01(Vec3{ -1.0f, 0.5f, 2.0f });

		std::cout << "clamp01((-1, 0.5, 2)): ";
		print_vec3(clamped);
		std::cout << "\n";

		passed &= expect(nearly_equal(clamped, Vec3{ 0.0f, 0.5f, 1.0f }), "clamp01(Vec3)");
		passed &= expect(is_finite(Vec3{ 1.0f, 2.0f, 3.0f }), "is_finite(Vec3)");

		return passed;
	}

	bool test_mat()
	{
		std::cout << "\n=== Mat.h ===\n";

		bool passed = true;

		const Mat4 identity = identity_mat4();
		const Mat4 translation = translation_matrix(Vec3{ 1.0f, 2.0f, 3.0f });
		const Mat4 scale = scale_matrix(Vec3{ 2.0f, 3.0f, 4.0f });

		const Vec3 point{ 1.0f, 1.0f, 1.0f };
		const Vec3 translatedPoint = transform_point(translation, point);
		const Vec3 scaledVector = transform_vector(scale, Vec3{ 1.0f, 1.0f, 1.0f });

		std::cout << "translatedPoint: ";
		print_vec3(translatedPoint);
		std::cout << "\nscaledVector: ";
		print_vec3(scaledVector);
		std::cout << "\n";

		passed &= expect(is_finite(identity), "is_finite(Mat4)");
		passed &= expect(nearly_equal(identity, identity_mat4()), "identity_mat4()");
		passed &= expect(nearly_equal(translatedPoint, Vec3{ 2.0f, 3.0f, 4.0f }), "transform_point(translation)");
		passed &= expect(nearly_equal(scaledVector, Vec3{ 2.0f, 3.0f, 4.0f }), "transform_vector(scale)");

		const Quaternion rotation = quaternion_from_axis_angle(Vec3{ 0.0f, 0.0f, 1.0f }, HalfPi);
		const Mat4 composed = compose_trs(Vec3{ 1.0f, 0.0f, 0.0f }, rotation, Vec3{ 2.0f, 2.0f, 2.0f });

		const Vec3 composedPoint = transform_point(composed, Vec3{ 1.0f, 0.0f, 0.0f });

		std::cout << "compose_trs point: ";
		print_vec3(composedPoint);
		std::cout << "\n";

		passed &= expect(nearly_equal(composedPoint, Vec3{ 1.0f, 2.0f, 0.0f }, 0.001f), "compose_trs translation/rotation/scale");

		const Mat4 inverse = inverse_or_identity(translation);
		const Vec3 restoredPoint = transform_point(inverse, translatedPoint);

		std::cout << "restoredPoint: ";
		print_vec3(restoredPoint);
		std::cout << "\n";

		passed &= expect(nearly_equal(restoredPoint, point, 0.001f), "inverse_or_identity(Mat4)");

		const Vec3 normal = transform_normal(scale, Vec3{ 0.0f, 1.0f, 0.0f });

		std::cout << "transform_normal: ";
		print_vec3(normal);
		std::cout << "\n";

		passed &= expect(nearly_equal(normal, Vec3{ 0.0f, 1.0f, 0.0f }, 0.001f), "transform_normal(Mat4)");

		const Mat4 perspective = perspective_matrix(radians(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);
		const Mat4 orthographic = orthographic_matrix(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);

		passed &= expect(is_finite(perspective), "perspective_matrix()");
		passed &= expect(is_finite(orthographic), "orthographic_matrix()");

		return passed;
	}

	bool test_quaternion()
	{
		std::cout << "\n=== Quaternion.h ===\n";

		bool passed = true;

		const Quaternion identity = identity_quaternion();
		const Quaternion zRotation = quaternion_from_axis_angle(Vec3{ 0.0f, 0.0f, 1.0f }, HalfPi);
		const Vec3 rotated = rotate_vector(zRotation, Vec3{ 1.0f, 0.0f, 0.0f });

		std::cout << "identity: ";
		print_quaternion(identity);
		std::cout << "\nzRotation 90 graus em Z: ";
		print_quaternion(zRotation);
		std::cout << "\nrotated X: ";
		print_vec3(rotated);
		std::cout << "\n";

		passed &= expect(is_finite(identity), "is_finite(Quaternion)");
		passed &= expect(nearly_equal(identity, Quaternion{ 1.0f, 0.0f, 0.0f, 0.0f }), "identity_quaternion()");
		passed &= expect(nearly_equal(rotated, Vec3{ 0.0f, 1.0f, 0.0f }, 0.001f), "rotate_vector 90 graus em Z");

		const Quaternion fromEuler = quaternion_from_euler_xyz(Vec3{ 0.0f, 0.0f, HalfPi });
		const Vec3 eulerRotated = rotate_vector(fromEuler, Vec3{ 1.0f, 0.0f, 0.0f });

		std::cout << "eulerRotated X: ";
		print_vec3(eulerRotated);
		std::cout << "\n";

		passed &= expect(nearly_equal(eulerRotated, Vec3{ 0.0f, 1.0f, 0.0f }, 0.001f), "quaternion_from_euler_xyz()");

		const Quaternion between = quaternion_between_vectors(Vec3{ 1.0f, 0.0f, 0.0f }, Vec3{ 0.0f, 1.0f, 0.0f });
		const Vec3 betweenRotated = rotate_vector(between, Vec3{ 1.0f, 0.0f, 0.0f });

		std::cout << "betweenRotated X->Y: ";
		print_vec3(betweenRotated);
		std::cout << "\n";

		passed &= expect(nearly_equal(betweenRotated, Vec3{ 0.0f, 1.0f, 0.0f }, 0.001f), "quaternion_between_vectors X para Y");

		const Quaternion inverse = inverse_rotation(zRotation);
		const Vec3 restored = rotate_vector(inverse, rotated);

		std::cout << "restored apos inverse_rotation: ";
		print_vec3(restored);
		std::cout << "\n";

		passed &= expect(nearly_equal(restored, Vec3{ 1.0f, 0.0f, 0.0f }, 0.001f), "inverse_rotation()");

		const Quaternion halfRotation = slerp(identity, zRotation, 0.5f);
		const Vec3 halfRotated = rotate_vector(halfRotation, Vec3{ 1.0f, 0.0f, 0.0f });

		std::cout << "halfRotated: ";
		print_vec3(halfRotated);
		std::cout << "\n";

		passed &= expect(nearly_equal(glm::length(halfRotated), 1.0f, 0.001f), "slerp() manteve vetor unitario");

		return passed;
	}

	bool test_existing_math_integration()
	{
		std::cout << "\n=== Integracao com math existente ===\n";

		bool passed = true;

		Transform transform;
		transform.translation = Vec3{ 1.0f, 2.0f, 3.0f };
		transform.rotation = quaternion_from_axis_angle(Vec3{ 0.0f, 0.0f, 1.0f }, HalfPi);
		transform.scale = Vec3{ 2.0f, 2.0f, 2.0f };

		const Vec3 transformedPoint = transform.transform_point(Vec3{ 1.0f, 0.0f, 0.0f });

		std::cout << "Transform::transform_point: ";
		print_vec3(transformedPoint);
		std::cout << "\n";

		passed &= expect(nearly_equal(transformedPoint, Vec3{ 1.0f, 4.0f, 3.0f }, 0.001f), "Transform integrado com Quaternion/Vec");

		Bounds bounds = Bounds::empty();
		bounds.expand(Vec3{ -1.0f, -2.0f, -3.0f });
		bounds.expand(Vec3{ 1.0f, 2.0f, 3.0f });

		std::cout << "Bounds center: ";
		print_vec3(bounds.center());
		std::cout << "\nBounds size: ";
		print_vec3(bounds.size());
		std::cout << "\n";

		passed &= expect(bounds.is_valid(), "Bounds valido apos expand");
		passed &= expect(nearly_equal(bounds.center(), Vec3{ 0.0f, 0.0f, 0.0f }), "Bounds::center()");
		passed &= expect(nearly_equal(bounds.size(), Vec3{ 2.0f, 4.0f, 6.0f }), "Bounds::size()");
		passed &= expect(bounds.contains(Vec3{ 0.0f, 0.0f, 0.0f }), "Bounds::contains()");

		const float area = triangle_area(
			Vec3{ 0.0f, 0.0f, 0.0f },
			Vec3{ 1.0f, 0.0f, 0.0f },
			Vec3{ 0.0f, 1.0f, 0.0f }
		);

		const Vec3 normal = triangle_normal(
			Vec3{ 0.0f, 0.0f, 0.0f },
			Vec3{ 1.0f, 0.0f, 0.0f },
			Vec3{ 0.0f, 1.0f, 0.0f }
		);

		std::cout << "triangle_area: " << area << "\n";
		std::cout << "triangle_normal: ";
		print_vec3(normal);
		std::cout << "\n";

		passed &= expect(nearly_equal(area, 0.5f), "triangle_area()");
		passed &= expect(nearly_equal(normal, Vec3{ 0.0f, 0.0f, 1.0f }), "triangle_normal()");

		return passed;
	}

}

int main()
{
	std::cout << std::fixed << std::setprecision(3);

	std::cout << "=== Locus3D Kernel Math Test ===\n";

	bool passed = true;

	passed &= test_vec();
	passed &= test_mat();
	passed &= test_quaternion();
	passed &= test_existing_math_integration();

	std::cout << "\nResultado final: " << (passed ? "PASS" : "FAIL") << "\n";

	return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}