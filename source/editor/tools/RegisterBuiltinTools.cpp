/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/RegisterBuiltinTools.h"

#include "editor/tools/management/ToolRegistry.h"
#include "editor/tools/mesh/edge/BevelTool.h"
#include "editor/tools/mesh/edge/EdgeSlideTool.h"
#include "editor/tools/mesh/face/ExtrudeFaceTool.h"
#include "editor/tools/mesh/face/InsetFaceTool.h"
#include "editor/tools/mesh/face/SolidifyTool.h"
#include "editor/tools/mesh/topology/LoopCutTool.h"
#include "editor/tools/selection/SelectTool.h"
#include "editor/tools/transform/TransformTool.h"

#include <memory>

namespace locus::editor {

    bool register_builtin_tools(
        ToolRegistry& registry)
    {
        bool registeredAll = true;

        registeredAll &=
            registry.register_tool(
                SelectTool::make_descriptor(),
                [] {
                    return std::make_unique<SelectTool>();
                });

        registeredAll &=
            registry.register_tool(
                TransformTool::make_descriptor(),
                [] {
                    return std::make_unique<TransformTool>();
                });

        registeredAll &=
            registry.register_tool(
                ExtrudeFaceTool::make_descriptor(),
                [] {
                    return std::make_unique<ExtrudeFaceTool>();
                });

        registeredAll &=
            registry.register_tool(
                InsetFaceTool::make_descriptor(),
                [] {
                    return std::make_unique<InsetFaceTool>();
                });

        registeredAll &=
            registry.register_tool(
                SolidifyTool::make_descriptor(),
                [] {
                    return std::make_unique<SolidifyTool>();
                });

        registeredAll &=
            registry.register_tool(
                EdgeSlideTool::make_descriptor(),
                [] {
                    return std::make_unique<EdgeSlideTool>();
                });

        registeredAll &=
            registry.register_tool(
                BevelTool::make_descriptor(),
                [] {
                    return std::make_unique<BevelTool>();
                });

        registeredAll &=
            registry.register_tool(
                LoopCutTool::make_descriptor(),
                [] {
                    return std::make_unique<LoopCutTool>();
                });

        return registeredAll;
    }

} // namespace locus::editor
