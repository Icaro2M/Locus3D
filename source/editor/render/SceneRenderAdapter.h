/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/render/MeshNodeRenderAdapter.h"
#include "editor/scene/SceneNodeId.h"
#include "editor/scene/NodeType.h"
#include "graphics/common/GraphicsResult.h"
#include "graphics/mesh/GpuMesh.h"
#include "graphics/mesh/MeshRenderCache.h"
#include "graphics/mesh/MeshUploader.h"
#include "graphics/scene/RenderScene.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace locus::editor {

    class EditorScene;
    class MeshNode;

    /**
     * @brief Lookup table used to resolve GPU meshes for editor mesh nodes.
     */
    using SceneRenderGpuMeshMap = std::unordered_map<SceneNodeId, const graphics::GpuMesh*>;

    /**
     * @brief Resolves the mesh revision used by MeshRenderCache for a mesh node.
     */
    using SceneRenderMeshRevisionResolver = std::function<graphics::u64(const MeshNode&)>;

    /**
     * @brief Options used when converting an editor scene to a graphics render scene.
     */
    struct SceneRenderOptions {
        /**
         * @brief Options forwarded to MeshNodeRenderAdapter.
         */
        MeshNodeRenderOptions meshOptions{};

        /**
         * @brief True when invisible mesh nodes should still be emitted with visibility=false.
         */
        bool includeHiddenNodes = true;

        /**
         * @brief True when render objects are allowed to reference null GPU meshes.
         *
         * This is useful for CPU-only smoke tests. Real render paths should usually
         * keep this disabled or use build_cached_render_scene().
         */
        bool allowNullGpuMeshes = true;

        /**
         * @brief True when conversion should stop at the first failed mesh node.
         */
        bool stopOnError = false;

        /**
         * @brief Fallback revision used when no revision resolver is provided.
         */
        graphics::u64 fallbackMeshRevision = 1;

        /**
         * @brief Optional per-node revision resolver used by cached scene builds.
         */
        SceneRenderMeshRevisionResolver meshRevisionResolver{};
    };

    /**
     * @brief Diagnostic information for one visited editor scene node.
     */
    struct SceneRenderNodeResult {
        /**
         * @brief Visited editor node identifier.
         */
        SceneNodeId nodeId{};

        /**
         * @brief Runtime node type.
         */
        NodeType nodeType = NodeType::Empty;

        /**
         * @brief True when this node was a mesh node.
         */
        bool meshNode = false;

        /**
         * @brief True when a render object was emitted.
         */
        bool emitted = false;

        /**
         * @brief True when this node was intentionally skipped.
         */
        bool skipped = false;

        /**
         * @brief True when this node failed conversion.
         */
        bool failed = false;

        /**
         * @brief Mesh-node-level diagnostic data.
         */
        MeshNodeRenderResult meshResult{};

        /**
         * @brief Human-readable diagnostic message.
         */
        std::string message;
    };

    /**
     * @brief Diagnostics produced when converting an EditorScene to RenderScene.
     */
    struct SceneRenderResult {
        /**
         * @brief Total number of node identifiers visited.
         */
        std::size_t visitedNodeCount = 0;

        /**
         * @brief Total number of mesh nodes found.
         */
        std::size_t meshNodeCount = 0;

        /**
         * @brief Total number of render objects emitted.
         */
        std::size_t objectCount = 0;

        /**
         * @brief Total number of intentionally skipped nodes.
         */
        std::size_t skippedNodeCount = 0;

        /**
         * @brief Total number of failed node conversions.
         */
        std::size_t failedNodeCount = 0;

        /**
         * @brief Per-node diagnostics.
         */
        std::vector<SceneRenderNodeResult> nodes;

        /**
         * @brief Checks whether scene conversion had any failures.
         *
         * @return True when at least one node failed conversion.
         */
        [[nodiscard]] bool has_failures() const
        {
            return failedNodeCount > 0;
        }
    };

    /**
     * @brief Converts editor scenes into graphics render scenes.
     *
     * SceneRenderAdapter owns the semantic editor-to-graphics conversion. It does
     * not decide when synchronization happens; that responsibility belongs to the
     * future editor/sync module.
     */
    class SceneRenderAdapter {
    public:
        /**
         * @brief Builds a render scene from already-resolved GPU mesh pointers.
         *
         * @param scene Source editor scene.
         * @param gpuMeshes Optional map from editor node id to GPU mesh pointer.
         * @param options Scene conversion options.
         * @param result Optional diagnostic output.
         * @return Graphics render scene.
         */
        [[nodiscard]] static graphics::RenderScene build_render_scene(
            const EditorScene& scene,
            const SceneRenderGpuMeshMap& gpuMeshes = {},
            const SceneRenderOptions& options = {},
            SceneRenderResult* result = nullptr
        );

        /**
         * @brief Builds a render scene using MeshRenderCache and MeshUploader.
         *
         * @param scene Source editor scene.
         * @param cache Mesh render cache that owns GPU meshes.
         * @param uploader GPU uploader used on cache misses.
         * @param options Scene conversion options.
         * @param result Optional diagnostic output.
         * @return Render scene on success, or a graphics error on failure.
         */
        [[nodiscard]] static graphics::GraphicsResult<graphics::RenderScene> build_cached_render_scene(
            const EditorScene& scene,
            graphics::MeshRenderCache& cache,
            const graphics::MeshUploader& uploader,
            const SceneRenderOptions& options = {},
            SceneRenderResult* result = nullptr
        );

    private:
        /**
         * @brief Resolves the mesh revision for a node.
         *
         * @param node Source mesh node.
         * @param options Scene conversion options.
         * @return Mesh revision value.
         */
        [[nodiscard]] static graphics::u64 resolve_mesh_revision(
            const MeshNode& node,
            const SceneRenderOptions& options
        );

        /**
         * @brief Looks up a GPU mesh pointer for a node.
         *
         * @param id Editor node identifier.
         * @param gpuMeshes Lookup table.
         * @return GPU mesh pointer, or null when not found.
         */
        [[nodiscard]] static const graphics::GpuMesh* find_gpu_mesh(
            SceneNodeId id,
            const SceneRenderGpuMeshMap& gpuMeshes
        );

        /**
         * @brief Appends a node diagnostic to the optional scene result.
         *
         * @param result Optional scene result.
         * @param nodeResult Node diagnostic to append.
         */
        static void push_node_result(
            SceneRenderResult* result,
            SceneRenderNodeResult nodeResult
        );
    };

} // namespace locus::editor