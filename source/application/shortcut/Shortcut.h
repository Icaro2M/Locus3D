/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/input/InputEvent.h"

namespace locus::application {

    /**
     * @brief Semantic application action resolved from a keyboard shortcut.
     */
    enum class ShortcutAction {
        None,
        ActivateSelectTool,
        ActivateTranslateTool,
        ActivateRotateTool,
        ActivateScaleTool,
        ActivateUniversalTool,
        ActivatePivotTool,
        ActivateExtrudeFaceTool,
        ActivateInsetFaceTool,
        ActivateSolidifyTool,
        ActivateShrinkFattenTool,
        ActivateEdgeSlideTool,
        ActivateBevelTool,
        ActivateLoopCutTool,
        ExecuteBridgeEdgeAction,
        ExecuteFillHoleAction,
        ExecuteFlipFacesAction,
        ExecuteDissolveAction,
        SetObjectGranularity,
        SetVertexGranularity,
        SetEdgeGranularity,
        SetFaceGranularity,
        ToggleProjection,
        FrontView,
        BackView,
        LeftView,
        RightView,
        TopView,
        BottomView,
        ToggleViewportShading,
        ToggleFaceOrientation,
        ToggleManufacturingAnalysis,
        Undo,
        Redo,
        Save,
        Open,
        CopySelection,
        PasteSelection,
        DeleteSelection,
        Cancel
    };

    /**
     * @brief Minimal context used to resolve shortcuts safely.
     */
    struct ShortcutContext {
        bool viewportFocused = true;
        bool textInputActive = false;
        bool modalActive = false;
        bool objectMode = true;
        bool vertexSelectionContext = false;
        bool faceSelectionContext = false;
        bool edgeSelectionContext = false;
        bool transformSelectionContext = false;
        bool transformToolActive = false;
    };

    /**
     * @brief One normalized keyboard shortcut binding.
     */
    struct ShortcutBinding {
        Key key = Key::Unknown;
        InputModifiers requiredModifiers = InputModifiers::None;
        InputModifiers forbiddenModifiers = InputModifiers::None;
        ShortcutAction action = ShortcutAction::None;
    };

} // namespace locus::application
