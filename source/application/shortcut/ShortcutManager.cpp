/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/shortcut/ShortcutManager.h"

#include "application/input/InputState.h"

namespace locus::application {

    namespace {

        [[nodiscard]] bool modifiers_match(
            InputModifiers active,
            InputModifiers required,
            InputModifiers forbidden) noexcept
        {
            return has_input_modifier(active, required)
                && (active & forbidden) == InputModifiers::None;
        }

        [[nodiscard]] bool context_allows_shortcuts(
            const ShortcutContext& context) noexcept
        {
            return context.viewportFocused
                && !context.textInputActive;
        }

        [[nodiscard]] bool context_allows_action(
            ShortcutAction action,
            const ShortcutContext& context) noexcept
        {
            switch (action) {
            case ShortcutAction::ActivateExtrudeFaceTool:
            case ShortcutAction::ActivateInsetFaceTool:
            case ShortcutAction::ActivateSolidifyTool:
                return context.faceSelectionContext
                    && !context.transformToolActive
                    && !context.modalActive;

            case ShortcutAction::ActivateShrinkFattenTool:
                return context.vertexSelectionContext
                    && !context.transformToolActive
                    && !context.modalActive;

            case ShortcutAction::ActivateEdgeSlideTool:
            case ShortcutAction::ActivateBevelTool:
            case ShortcutAction::ActivateLoopCutTool:
            case ShortcutAction::ExecuteBridgeEdgeAction:
                return context.edgeSelectionContext
                    && !context.transformToolActive
                    && !context.modalActive;

            case ShortcutAction::SetObjectGranularity:
            case ShortcutAction::SetVertexGranularity:
            case ShortcutAction::SetEdgeGranularity:
            case ShortcutAction::SetFaceGranularity:
                return !context.modalActive;

            case ShortcutAction::ActivateTranslateTool:
            case ShortcutAction::ActivateRotateTool:
            case ShortcutAction::ActivateScaleTool:
            case ShortcutAction::ActivateUniversalTool:
                return context.transformSelectionContext
                    && !context.modalActive;

            case ShortcutAction::ActivateSelectTool:
            case ShortcutAction::DeleteSelection:
                return !context.modalActive;

            case ShortcutAction::Cancel:
                return true;

            case ShortcutAction::None:
            case ShortcutAction::Undo:
            case ShortcutAction::Redo:
            case ShortcutAction::Save:
            case ShortcutAction::Open:
                return !context.modalActive;
            }

            return false;
        }

    } // namespace

    ShortcutManager::ShortcutManager()
    {
        reset_to_defaults();
    }

    void ShortcutManager::reset_to_defaults()
    {
        bindings_ = {
            { Key::Q, InputModifiers::None, InputModifiers::Control, ShortcutAction::ActivateSelectTool },
            { Key::W, InputModifiers::None, InputModifiers::Control, ShortcutAction::ActivateTranslateTool },
            { Key::E, InputModifiers::None, InputModifiers::Control, ShortcutAction::ActivateExtrudeFaceTool },
            { Key::E, InputModifiers::None, InputModifiers::Control, ShortcutAction::ActivateRotateTool },
            { Key::G, InputModifiers::None, InputModifiers::Control, ShortcutAction::ActivateEdgeSlideTool },
            { Key::B, InputModifiers::None, InputModifiers::Control, ShortcutAction::ActivateBevelTool },
            { Key::J, InputModifiers::None, InputModifiers::Control, ShortcutAction::ExecuteBridgeEdgeAction },
            { Key::F, InputModifiers::None, InputModifiers::Control, ShortcutAction::ActivateSolidifyTool },
            { Key::S, InputModifiers::Alt, InputModifiers::Control, ShortcutAction::ActivateShrinkFattenTool },
            { Key::I, InputModifiers::None, InputModifiers::Control, ShortcutAction::ActivateInsetFaceTool },
            { Key::R, InputModifiers::None, InputModifiers::Control, ShortcutAction::ActivateLoopCutTool },
            { Key::R, InputModifiers::None, InputModifiers::Control, ShortcutAction::ActivateScaleTool },
            { Key::T, InputModifiers::None, InputModifiers::Control, ShortcutAction::ActivateUniversalTool },
            { Key::Num1, InputModifiers::None, InputModifiers::Control, ShortcutAction::SetObjectGranularity },
            { Key::Num2, InputModifiers::None, InputModifiers::Control, ShortcutAction::SetVertexGranularity },
            { Key::Num3, InputModifiers::None, InputModifiers::Control, ShortcutAction::SetEdgeGranularity },
            { Key::Num4, InputModifiers::None, InputModifiers::Control, ShortcutAction::SetFaceGranularity },
            { Key::Z, InputModifiers::Control, InputModifiers::Shift, ShortcutAction::Undo },
            { Key::Z, InputModifiers::Control | InputModifiers::Shift, InputModifiers::None, ShortcutAction::Redo },
            { Key::Y, InputModifiers::Control, InputModifiers::None, ShortcutAction::Redo },
            { Key::S, InputModifiers::Control, InputModifiers::None, ShortcutAction::Save },
            { Key::O, InputModifiers::Control, InputModifiers::None, ShortcutAction::Open },
            { Key::Delete, InputModifiers::None, InputModifiers::None, ShortcutAction::DeleteSelection },
            { Key::Escape, InputModifiers::None, InputModifiers::None, ShortcutAction::Cancel }
        };
    }

    ShortcutAction ShortcutManager::resolve(
        const InputState& input,
        const ShortcutContext& context) const
    {
        if (!context_allows_shortcuts(context)) {
            return ShortcutAction::None;
        }

        for (const ShortcutBinding& binding : bindings_) {
            if (input.key_pressed(binding.key)
                && modifiers_match(
                    input.modifiers(),
                    binding.requiredModifiers,
                    binding.forbiddenModifiers)
                && context_allows_action(
                    binding.action,
                    context)) {
                return binding.action;
            }
        }

        return ShortcutAction::None;
    }

    const std::vector<ShortcutBinding>&
    ShortcutManager::bindings() const noexcept
    {
        return bindings_;
    }

} // namespace locus::application
