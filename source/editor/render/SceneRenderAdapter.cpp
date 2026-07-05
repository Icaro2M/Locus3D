/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/render/SceneRenderAdapter.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/MeshNode.h"
#include "editor/scene/SceneNode.h"
#include "graphics/common/GraphicsError.h"

namespace locus::editor {

    graphics::RenderScene SceneRenderAdapter::build_render_scene(
        const EditorScene& scene,
        const SceneRenderGpuMeshMap& gpuMeshes,
        const SceneRenderOptions& options,
        SceneRenderResult* result
    ) {
        if (result) {
            *result = {};
        }

        graphics::RenderScene renderScene;
        const std::vector<SceneNodeId> nodeIds = scene.tree().node_ids();

        renderScene.reserve(nodeIds.size());

        for (SceneNodeId id : nodeIds) {
            SceneRenderNodeResult nodeResult{};
            nodeResult.nodeId = id;

            if (result) {
                ++result->visitedNodeCount;
            }

            const SceneNode* node = scene.find_node(id);

            if (!node) {
                nodeResult.failed = true;
                nodeResult.message = "Scene tree returned an id that could not be resolved.";

                if (result) {
                    ++result->failedNodeCount;
                }

                push_node_result(result, std::move(nodeResult));

                if (options.stopOnError) {
                    break;
                }

                continue;
            }

            nodeResult.nodeType = node->type();

            if (node->type() != NodeType::Mesh) {
                nodeResult.skipped = true;
                nodeResult.message = "Node skipped because it is not a mesh node.";

                if (result) {
                    ++result->skippedNodeCount;
                }

                push_node_result(result, std::move(nodeResult));
                continue;
            }

            nodeResult.meshNode = true;

            if (result) {
                ++result->meshNodeCount;
            }

            const MeshNode* meshNode = scene.find_mesh(id);

            if (!meshNode) {
                nodeResult.failed = true;
                nodeResult.message = "Node type is Mesh, but find_mesh() returned null.";

                if (result) {
                    ++result->failedNodeCount;
                }

                push_node_result(result, std::move(nodeResult));

                if (options.stopOnError) {
                    break;
                }

                continue;
            }

            if (!options.includeHiddenNodes && !meshNode->is_visible()) {
                nodeResult.skipped = true;
                nodeResult.message = "Mesh node skipped because it is hidden.";

                if (result) {
                    ++result->skippedNodeCount;
                }

                push_node_result(result, std::move(nodeResult));
                continue;
            }

            const graphics::GpuMesh* gpuMesh = find_gpu_mesh(id, gpuMeshes);

            if (!gpuMesh && !options.allowNullGpuMeshes) {
                nodeResult.skipped = true;
                nodeResult.message = "Mesh node skipped because no GPU mesh was resolved.";

                if (result) {
                    ++result->skippedNodeCount;
                }

                push_node_result(result, std::move(nodeResult));
                continue;
            }

            graphics::RenderObject object =
                MeshNodeRenderAdapter::build_render_object(*meshNode, gpuMesh, options.meshOptions);

            renderScene.add_object(std::move(object));

            nodeResult.emitted = true;
            nodeResult.message = "Mesh node emitted as render object.";

            if (result) {
                ++result->objectCount;
            }

            push_node_result(result, std::move(nodeResult));
        }

        return renderScene;
    }

    graphics::GraphicsResult<graphics::RenderScene> SceneRenderAdapter::build_cached_render_scene(
        const EditorScene& scene,
        graphics::MeshRenderCache& cache,
        const graphics::MeshUploader& uploader,
        const SceneRenderOptions& options,
        SceneRenderResult* result
    ) {
        if (result) {
            *result = {};
        }

        graphics::RenderScene renderScene;
        const std::vector<SceneNodeId> nodeIds = scene.tree().node_ids();

        renderScene.reserve(nodeIds.size());

        for (SceneNodeId id : nodeIds) {
            SceneRenderNodeResult nodeResult{};
            nodeResult.nodeId = id;

            if (result) {
                ++result->visitedNodeCount;
            }

            const SceneNode* node = scene.find_node(id);

            if (!node) {
                nodeResult.failed = true;
                nodeResult.message = "Scene tree returned an id that could not be resolved.";

                if (result) {
                    ++result->failedNodeCount;
                }

                push_node_result(result, std::move(nodeResult));

                if (options.stopOnError) {
                    return graphics::GraphicsError::make(
                        graphics::GraphicsErrorCode::ResourceNotFound,
                        "Scene tree returned an id that could not be resolved."
                    );
                }

                continue;
            }

            nodeResult.nodeType = node->type();

            if (node->type() != NodeType::Mesh) {
                nodeResult.skipped = true;
                nodeResult.message = "Node skipped because it is not a mesh node.";

                if (result) {
                    ++result->skippedNodeCount;
                }

                push_node_result(result, std::move(nodeResult));
                continue;
            }

            nodeResult.meshNode = true;

            if (result) {
                ++result->meshNodeCount;
            }

            const MeshNode* meshNode = scene.find_mesh(id);

            if (!meshNode) {
                nodeResult.failed = true;
                nodeResult.message = "Node type is Mesh, but find_mesh() returned null.";

                if (result) {
                    ++result->failedNodeCount;
                }

                push_node_result(result, std::move(nodeResult));

                if (options.stopOnError) {
                    return graphics::GraphicsError::make(
                        graphics::GraphicsErrorCode::ResourceNotFound,
                        "Node type is Mesh, but find_mesh() returned null."
                    );
                }

                continue;
            }

            if (!options.includeHiddenNodes && !meshNode->is_visible()) {
                nodeResult.skipped = true;
                nodeResult.message = "Mesh node skipped because it is hidden.";

                if (result) {
                    ++result->skippedNodeCount;
                }

                push_node_result(result, std::move(nodeResult));
                continue;
            }

            const graphics::u64 meshRevision = resolve_mesh_revision(*meshNode, options);

            MeshNodeRenderResult meshResult{};
            auto objectResult = MeshNodeRenderAdapter::build_cached_render_object(
                *meshNode,
                meshRevision,
                cache,
                uploader,
                options.meshOptions,
                &meshResult
            );

            nodeResult.meshResult = meshResult;

            if (!objectResult) {
                nodeResult.failed = true;
                nodeResult.message = objectResult.error().message;

                if (result) {
                    ++result->failedNodeCount;
                }

                push_node_result(result, std::move(nodeResult));

                if (options.stopOnError) {
                    return objectResult.error();
                }

                continue;
            }

            renderScene.add_object(objectResult.move_value());

            nodeResult.emitted = true;
            nodeResult.message = "Mesh node emitted as cached render object.";

            if (result) {
                ++result->objectCount;
            }

            push_node_result(result, std::move(nodeResult));
        }

        return renderScene;
    }

    graphics::u64 SceneRenderAdapter::resolve_mesh_revision(
        const MeshNode& node,
        const SceneRenderOptions& options
    ) {
        if (options.meshRevisionResolver) {
            return options.meshRevisionResolver(node);
        }

        return options.fallbackMeshRevision;
    }

    const graphics::GpuMesh* SceneRenderAdapter::find_gpu_mesh(
        SceneNodeId id,
        const SceneRenderGpuMeshMap& gpuMeshes
    ) {
        const auto it = gpuMeshes.find(id);

        if (it == gpuMeshes.end()) {
            return nullptr;
        }

        return it->second;
    }

    void SceneRenderAdapter::push_node_result(
        SceneRenderResult* result,
        SceneRenderNodeResult nodeResult
    ) {
        if (!result) {
            return;
        }

        result->nodes.push_back(std::move(nodeResult));
    }

} // namespace locus::editor