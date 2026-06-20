/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#define NOMINMAX

#include "graphics/common/GraphicsConfig.h"
#include "graphics/context/OpenGLContext.h"
#include "graphics/gpu/Shader.h"
#include "graphics/mesh/GpuMesh.h"
#include "graphics/mesh/MeshUploadData.h"
#include "graphics/renderer/Renderer.h"
#include "graphics/scene/RenderObject.h"
#include "graphics/scene/RenderScene.h"
#include "graphics/window/Window.h"

#include "kernel/common/Result.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/render/MeshTriangulator.h"
#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/render/RenderMesh.h"
#include "kernel/geometry/render/WireframeBuilder.h"
#include "kernel/io/ObjImporter.h"
#include "kernel/io/StlImporter.h"

#include <glad/glad.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <windows.h>
#include <commdlg.h>

#pragma comment(lib, "Comdlg32.lib")

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

namespace {

	using namespace locus;
	using namespace locus::graphics;
	using namespace locus::kernel;
	using namespace locus::kernel::geometry;
	using namespace locus::kernel::io;

	const char* VisualVertexShader = R"(
#version 450 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec4 a_Color;

uniform mat4 u_Model;
uniform mat4 u_MVP;

out vec3 v_Normal;
out vec4 v_Color;

void main()
{
    mat3 normalMatrix = transpose(inverse(mat3(u_Model)));
    v_Normal = normalize(normalMatrix * a_Normal);
    v_Color = a_Color;
    gl_Position = u_MVP * vec4(a_Position, 1.0);
}
)";

	const char* VisualFragmentShader = R"(
#version 450 core

in vec3 v_Normal;
in vec4 v_Color;

uniform vec4 u_BaseColor;
uniform int u_UseVertexColor;

out vec4 FragColor;

void main()
{
    vec3 normal = normalize(v_Normal);
    vec3 lightDirection = normalize(vec3(-0.35, -0.65, -0.75));
    float diffuse = max(dot(normal, -lightDirection), 0.0);
    float lighting = 0.28 + diffuse * 0.72;

    vec4 color = u_UseVertexColor != 0 ? v_Color : u_BaseColor;
    FragColor = vec4(color.rgb * lighting, color.a);
}
)";

	const char* WireVertexShader = R"(
#version 450 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec4 a_Color;

uniform mat4 u_Model;
uniform mat4 u_MVP;

void main()
{
    gl_Position = u_MVP * vec4(a_Position, 1.0);
}
)";

	const char* WireFragmentShader = R"(
#version 450 core

uniform vec4 u_BaseColor;

out vec4 FragColor;

void main()
{
    FragColor = u_BaseColor;
}
)";

	struct MeshFit {
		glm::vec3 center{ 0.0f, 0.0f, 0.0f };
		float scale = 1.0f;
	};

	bool expect(bool condition, const std::string& message)
	{
		if (condition) {
			std::cout << "[OK] " << message << '\n';
			return true;
		}

		std::cout << "[FAIL] " << message << '\n';
		return false;
	}

	void print_graphics_error(const std::string& label, const GraphicsError& error)
	{
		std::cout << label << ": " << error.message << '\n';
	}

	void print_kernel_error(const std::string& label, const Error& error)
	{
		std::cout << label << ": " << error.message << '\n';
	}

	std::optional<std::filesystem::path> open_mesh_file_dialog()
	{
		wchar_t fileName[MAX_PATH] = L"";

		OPENFILENAMEW openFileName{};
		openFileName.lStructSize = sizeof(openFileName);
		openFileName.hwndOwner = nullptr;
		openFileName.lpstrFile = fileName;
		openFileName.nMaxFile = MAX_PATH;
		openFileName.lpstrFilter =
			L"Mesh files (*.stl;*.obj)\0*.stl;*.obj\0"
			L"STL files (*.stl)\0*.stl\0"
			L"OBJ files (*.obj)\0*.obj\0"
			L"All files (*.*)\0*.*\0";
		openFileName.nFilterIndex = 1;
		openFileName.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
		openFileName.lpstrTitle = L"Selecione um arquivo .stl ou .obj";

		if (!GetOpenFileNameW(&openFileName)) {
			return std::nullopt;
		}

		return std::filesystem::path(fileName);
	}

	std::string lowercase_extension(const std::filesystem::path& path)
	{
		std::string extension = path.extension().string();

		std::transform(
			extension.begin(),
			extension.end(),
			extension.begin(),
			[](unsigned char character) {
				return static_cast<char>(std::tolower(character));
			}
		);

		return extension;
	}

	MeshFit compute_mesh_fit(const RenderMesh& renderMesh)
	{
		if (renderMesh.vertices.empty()) {
			return {};
		}

		glm::vec3 minPoint{
			std::numeric_limits<float>::max(),
			std::numeric_limits<float>::max(),
			std::numeric_limits<float>::max()
		};

		glm::vec3 maxPoint{
			std::numeric_limits<float>::lowest(),
			std::numeric_limits<float>::lowest(),
			std::numeric_limits<float>::lowest()
		};

		for (const RenderVertex& vertex : renderMesh.vertices) {
			minPoint = glm::min(minPoint, vertex.position);
			maxPoint = glm::max(maxPoint, vertex.position);
		}

		const glm::vec3 size = maxPoint - minPoint;
		const float maxDimension = std::max({ size.x, size.y, size.z });

		MeshFit fit;
		fit.center = (minPoint + maxPoint) * 0.5f;
		fit.scale = maxDimension > 0.00001f ? 3.0f / maxDimension : 1.0f;

		return fit;
	}

	MeshUploadData build_triangle_upload_data(const RenderMesh& renderMesh, const MeshFit& fit)
	{
		MeshUploadData uploadData;
		uploadData.topology = PrimitiveTopology::Triangles;
		uploadData.usage = BufferUsage::Static;
		uploadData.vertices.reserve(renderMesh.vertices.size());
		uploadData.indices.reserve(renderMesh.triangles.size() * 3);

		for (const RenderVertex& renderVertex : renderMesh.vertices) {
			const glm::vec3 fittedPosition = (renderVertex.position - fit.center) * fit.scale;

			MeshVertex vertex;

			vertex.position[0] = fittedPosition.x;
			vertex.position[1] = fittedPosition.y;
			vertex.position[2] = fittedPosition.z;

			vertex.normal[0] = renderVertex.normal.x;
			vertex.normal[1] = renderVertex.normal.y;
			vertex.normal[2] = renderVertex.normal.z;

			vertex.color[0] = 0.72f;
			vertex.color[1] = 0.78f;
			vertex.color[2] = 0.92f;
			vertex.color[3] = 1.0f;

			uploadData.vertices.push_back(vertex);
		}

		for (const RenderTriangle& triangle : renderMesh.triangles) {
			uploadData.indices.push_back(static_cast<u32>(triangle.a));
			uploadData.indices.push_back(static_cast<u32>(triangle.b));
			uploadData.indices.push_back(static_cast<u32>(triangle.c));
		}

		return uploadData;
	}

	MeshUploadData build_wire_upload_data(const RenderMesh& wireRenderMesh, const MeshFit& fit)
	{
		MeshUploadData uploadData;
		uploadData.topology = PrimitiveTopology::Lines;
		uploadData.usage = BufferUsage::Static;
		uploadData.vertices.reserve(wireRenderMesh.vertices.size());
		uploadData.indices.reserve(wireRenderMesh.lines.size() * 2);

		for (const RenderVertex& renderVertex : wireRenderMesh.vertices) {
			const glm::vec3 fittedPosition = (renderVertex.position - fit.center) * fit.scale;

			MeshVertex vertex;

			vertex.position[0] = fittedPosition.x;
			vertex.position[1] = fittedPosition.y;
			vertex.position[2] = fittedPosition.z;

			vertex.normal[0] = renderVertex.normal.x;
			vertex.normal[1] = renderVertex.normal.y;
			vertex.normal[2] = renderVertex.normal.z;

			vertex.color[0] = 0.02f;
			vertex.color[1] = 0.02f;
			vertex.color[2] = 0.025f;
			vertex.color[3] = 1.0f;

			uploadData.vertices.push_back(vertex);
		}

		for (const RenderLine& line : wireRenderMesh.lines) {
			uploadData.indices.push_back(static_cast<u32>(line.a));
			uploadData.indices.push_back(static_cast<u32>(line.b));
		}

		return uploadData;
	}

	Result<LEM> import_mesh_from_path(const std::filesystem::path& path)
	{
		const std::string extension = lowercase_extension(path);

		MeshImportOptions options;
		options.mergeDuplicateVertices = true;
		options.mergeEpsilon = 1.0e-5f;
		options.rebuildNormals = true;
		options.requireFaces = true;

		if (extension == ".obj") {
			ObjImporter importer;
			return importer.import_mesh(path, options);
		}

		if (extension == ".stl") {
			StlImporter importer;
			return importer.import_mesh(path, options);
		}

		return Result<LEM>::fail(ErrorCode::UnsupportedOperation, "Formato não suportado. Selecione um arquivo .stl ou .obj.");
	}

	bool prepare_imported_mesh(
		const std::filesystem::path& selectedPath,
		LEM& importedMesh,
		RenderMesh& solidRenderMesh,
		RenderMesh& wireRenderMesh,
		MeshFit& fit)
	{
		std::cout << "\n=== Mesh import setup ===\n";
		std::cout << "Arquivo selecionado: " << selectedPath.string() << '\n';
		std::cout << "Extensao detectada: " << lowercase_extension(selectedPath) << '\n';

		Result<LEM> importResult = import_mesh_from_path(selectedPath);
		if (importResult.is_error()) {
			print_kernel_error("Import error", importResult.error());
			return false;
		}

		importedMesh = std::move(importResult.value());

		bool passed = true;

		passed &= expect(!importedMesh.empty(), "malha importada nao esta vazia");
		passed &= expect(importedMesh.vertex_count() > 0, "malha importada possui vertices");
		passed &= expect(importedMesh.face_count() > 0, "malha importada possui faces");

		std::cout << "\n=== Imported LEM summary ===\n";
		std::cout << "Vertices: " << importedMesh.vertex_count() << '\n';
		std::cout << "Edges:    " << importedMesh.edge_count() << '\n';
		std::cout << "Loops:    " << importedMesh.loop_count() << '\n';
		std::cout << "Faces:    " << importedMesh.face_count() << '\n';

		solidRenderMesh = MeshTriangulator::triangulate(importedMesh);
		NormalBuilder::rebuild_normals(solidRenderMesh, NormalBuildMode::Flat);

		wireRenderMesh = WireframeBuilder::build(importedMesh);

		passed &= expect(solidRenderMesh.vertex_count() > 0, "RenderMesh solido possui vertices");
		passed &= expect(solidRenderMesh.triangle_count() > 0, "RenderMesh solido possui triangulos");
		passed &= expect(wireRenderMesh.line_count() > 0, "Wireframe topologico possui linhas");

		fit = compute_mesh_fit(solidRenderMesh);

		std::cout << "\n=== Render summary ===\n";
		std::cout << "Render vertices: " << solidRenderMesh.vertex_count() << '\n';
		std::cout << "Render triangles: " << solidRenderMesh.triangle_count() << '\n';
		std::cout << "Topology wire lines: " << wireRenderMesh.line_count() << '\n';
		std::cout << "Fit center: (" << fit.center.x << ", " << fit.center.y << ", " << fit.center.z << ")\n";
		std::cout << "Fit scale: " << fit.scale << '\n';

		return passed;
	}

	bool initialize_graphics(Window& window, OpenGLContext& context)
	{
		std::cout << "\n=== Graphics setup ===\n";

		WindowCreateInfo windowInfo;
		windowInfo.width = 1280;
		windowInfo.height = 720;
		windowInfo.title = "Locus3D - External Mesh Import Viewer";
		windowInfo.resizable = true;
		windowInfo.visible = true;
		windowInfo.requestOpenGLContext = true;
		windowInfo.openglMajorVersion = 4;
		windowInfo.openglMinorVersion = 5;
		windowInfo.openglCoreProfile = true;
		windowInfo.openglForwardCompatible = true;
		windowInfo.openglDebugContext = true;

		GraphicsResult windowResult = window.create(windowInfo);
		if (!windowResult) {
			print_graphics_error("Window error", windowResult.error());
			return false;
		}

		GraphicsConfig config;
		config.api = GraphicsApi::OpenGL;
		config.enableDebugOutput = true;
		config.enableVSync = true;
		config.requestedMajorVersion = 4;
		config.requestedMinorVersion = 5;
		config.coreProfile = true;
		config.forwardCompatible = true;

		GraphicsResult contextResult = context.initialize(window, config);
		if (!contextResult) {
			print_graphics_error("OpenGL context error", contextResult.error());
			return false;
		}

		context.make_current();
		context.set_vsync(true);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glLineWidth(1.5f);
		glViewport(0, 0, window.framebuffer_width(), window.framebuffer_height());

		std::cout << "[OK] janela e contexto OpenGL inicializados\n";
		std::cout << "Framebuffer: " << window.framebuffer_width() << "x" << window.framebuffer_height() << '\n';

		return true;
	}

	bool create_shader(Shader& shader, const char* vertexSource, const char* fragmentSource, const std::string& label)
	{
		GraphicsResult result = shader.create_from_source(vertexSource, fragmentSource);
		if (!result) {
			print_graphics_error(label, result.error());
			return false;
		}

		std::cout << "[OK] " << label << '\n';
		return true;
	}

	bool create_gpu_mesh(GpuMesh& mesh, const MeshUploadData& uploadData, const std::string& label)
	{
		GraphicsResult result = mesh.create(uploadData);
		if (!result) {
			print_graphics_error(label, result.error());
			return false;
		}

		std::cout << "[OK] " << label << '\n';
		return true;
	}

	void render_loop(
		Window& window,
		Renderer& renderer,
		const GpuMesh& solidMesh,
		const GpuMesh& wireMesh,
		const Shader& solidShader,
		const Shader& wireShader,
		const std::filesystem::path& selectedPath)
	{
		const auto start = std::chrono::steady_clock::now();

		while (!window.should_close()) {
			window.poll_events();

			const int framebufferWidth = window.framebuffer_width();
			const int framebufferHeight = window.framebuffer_height();

			if (framebufferWidth <= 0 || framebufferHeight <= 0) {
				continue;
			}

			const auto now = std::chrono::steady_clock::now();
			const float time = std::chrono::duration<float>(now - start).count();

			glViewport(0, 0, framebufferWidth, framebufferHeight);
			glClearColor(0.08f, 0.08f, 0.09f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			const float aspect = static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight);

			const glm::mat4 projection = glm::perspective(
				glm::radians(45.0f),
				aspect,
				0.1f,
				100.0f
			);

			const glm::mat4 view = glm::lookAt(
				glm::vec3{ 4.5f, 3.2f, 5.5f },
				glm::vec3{ 0.0f, 0.0f, 0.0f },
				glm::vec3{ 0.0f, 1.0f, 0.0f }
			);

			const glm::quat rotation =
				glm::angleAxis(time * 0.45f, glm::vec3{ 0.0f, 1.0f, 0.0f })
				* glm::angleAxis(glm::radians(16.0f), glm::vec3{ 1.0f, 0.0f, 0.0f });

			RenderObject solidObject;
			solidObject.id = 1;
			solidObject.name = "Imported Mesh - Solid";
			solidObject.mesh = &solidMesh;
			solidObject.shader = &solidShader;
			solidObject.transform.position = glm::vec3{ 0.0f, 0.0f, 0.0f };
			solidObject.transform.rotation = rotation;
			solidObject.transform.scale = glm::vec3{ 1.0f, 1.0f, 1.0f };
			solidObject.layer = RenderLayer::Default;

			RenderObject wireObject;
			wireObject.id = 2;
			wireObject.name = "Imported Mesh - Topology Wire";
			wireObject.mesh = &wireMesh;
			wireObject.shader = &wireShader;
			wireObject.transform = solidObject.transform;
			wireObject.layer = RenderLayer::Overlay;

			RenderScene scene;
			scene.reserve(2);
			scene.add_object(solidObject);
			scene.add_object(wireObject);

			renderer.set_view_matrix(view);
			renderer.set_projection_matrix(projection);
			renderer.render(scene);

			window.swap_buffers();
		}
	}

}

int main()
{
	std::cout << std::fixed << std::setprecision(3);
	std::cout << "=== Locus3D External STL/OBJ Import Viewer ===\n";

	const std::optional<std::filesystem::path> selectedPath = open_mesh_file_dialog();
	if (!selectedPath.has_value()) {
		std::cout << "Nenhum arquivo selecionado.\n";
		return EXIT_SUCCESS;
	}

	LEM importedMesh;
	RenderMesh solidRenderMesh;
	RenderMesh wireRenderMesh;
	MeshFit fit;

	if (!prepare_imported_mesh(*selectedPath, importedMesh, solidRenderMesh, wireRenderMesh, fit)) {
		std::cout << "\nResultado final: FAIL\n";
		return EXIT_FAILURE;
	}

	Window window;
	OpenGLContext context;

	if (!initialize_graphics(window, context)) {
		std::cout << "\nResultado final: FAIL\n";
		return EXIT_FAILURE;
	}

	Shader solidShader;
	Shader wireShader;

	if (!create_shader(solidShader, VisualVertexShader, VisualFragmentShader, "shader solido criado")) {
		std::cout << "\nResultado final: FAIL\n";
		return EXIT_FAILURE;
	}

	if (!create_shader(wireShader, WireVertexShader, WireFragmentShader, "shader wire topologico criado")) {
		std::cout << "\nResultado final: FAIL\n";
		return EXIT_FAILURE;
	}

	const MeshUploadData solidUploadData = build_triangle_upload_data(solidRenderMesh, fit);
	const MeshUploadData wireUploadData = build_wire_upload_data(wireRenderMesh, fit);

	GpuMesh solidGpuMesh;
	GpuMesh wireGpuMesh;

	if (!create_gpu_mesh(solidGpuMesh, solidUploadData, "GpuMesh solido criado a partir do arquivo importado")) {
		std::cout << "\nResultado final: FAIL\n";
		return EXIT_FAILURE;
	}

	if (!create_gpu_mesh(wireGpuMesh, wireUploadData, "GpuMesh wire topologico criado a partir do arquivo importado")) {
		std::cout << "\nResultado final: FAIL\n";
		return EXIT_FAILURE;
	}

	Renderer renderer;

	std::cout << "\n=== Visual result ===\n";
	std::cout << "Arquivo: " << selectedPath->string() << '\n';
	std::cout << "Deve aparecer a malha importada centralizada e normalizada na tela.\n";
	std::cout << "STL tende a aparecer triangulado; OBJ pode preservar quads/ngons no wireframe topologico.\n";
	std::cout << "Feche a janela para encerrar o teste.\n";

	render_loop(window, renderer, solidGpuMesh, wireGpuMesh, solidShader, wireShader, *selectedPath);

	std::cout << "\nResultado final: PASS\n";
	return EXIT_SUCCESS;
}