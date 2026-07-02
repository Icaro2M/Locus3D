/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../CommandCommandsTestSuite.h"

#include "editor/command/CommandDispatcher.h"
#include "editor/command/transform/NodeTransformSnapshot.h"
#include "editor/command/transform/RotateNodeCommand.h"
#include "editor/command/transform/ScaleNodeCommand.h"
#include "editor/command/transform/SetNodePivotCommand.h"
#include "editor/command/transform/SetNodeTransformCommand.h"
#include "editor/command/transform/TranslateNodeCommand.h"

#include <cmath>

namespace {

constexpr float epsilon = 0.0001f;

[[nodiscard]] bool near(float lhs, float rhs)
{
    return std::fabs(lhs - rhs) <= epsilon;
}

[[nodiscard]] bool near_vec3(const glm::vec3& lhs, const glm::vec3& rhs)
{
    return near(lhs.x, rhs.x) && near(lhs.y, rhs.y) && near(lhs.z, rhs.z);
}

[[nodiscard]] bool near_quat(const glm::quat& lhs, const glm::quat& rhs)
{
    return near(lhs.w, rhs.w) &&
        near(lhs.x, rhs.x) &&
        near(lhs.y, rhs.y) &&
        near(lhs.z, rhs.z);
}

[[nodiscard]] bool has_scene_render_picking(locus::editor::EditorDirtyFlags mask)
{
    return locus::editor::has_flag(mask, locus::editor::EditorDirtyFlags::Scene) &&
        locus::editor::has_flag(mask, locus::editor::EditorDirtyFlags::Render) &&
        locus::editor::has_flag(mask, locus::editor::EditorDirtyFlags::Picking);
}

[[nodiscard]] locus::editor::NodeTransform make_transform(
    const glm::vec3& position,
    const glm::quat& rotation,
    const glm::vec3& scale)
{
    locus::editor::NodeTransform transform;
    transform.set_position(position);
    transform.set_rotation(rotation);
    transform.set_scale(scale);
    return transform;
}

} // namespace

namespace locus::tests {

TestResult run_node_transform_command_tests()
{
    editor::Editor editor;
    editor::CommandDispatcher dispatcher(editor);

    const editor::SceneNodeId nodeId = editor.scene().create_empty("Transform Target");
    editor::SceneNode* node = editor.scene().find_node(nodeId);
    if (!node) {
        return TestResult::fail("test scene should contain a transform target");
    }

    editor::NodeTransform original = make_transform(
        glm::vec3{ 1.0f, 2.0f, 3.0f },
        glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f },
        glm::vec3{ 2.0f, 3.0f, 4.0f });
    node->transform() = original;

    const editor::NodeTransformSnapshot snapshot =
        editor::NodeTransformSnapshot::capture(node->transform());
    node->transform().reset();
    snapshot.apply_to(node->transform());
    if (!near_vec3(node->transform().position(), original.position()) ||
        !near_quat(node->transform().rotation(), original.rotation()) ||
        !near_vec3(node->transform().scale(), original.scale())) {
        return TestResult::fail("NodeTransformSnapshot should capture and restore a transform value");
    }

    editor::NodeTransform replacement = make_transform(
        glm::vec3{ 5.0f, 6.0f, 7.0f },
        glm::quat{ 0.0f, 0.0f, 1.0f, 0.0f },
        glm::vec3{ 8.0f, 9.0f, 10.0f });

    editor::SetNodeTransformCommand invalidSet{ editor::SceneNodeId{}, replacement };
    if (invalidSet.execute(dispatcher.context())) {
        return TestResult::fail("SetNodeTransformCommand should reject invalid node ids");
    }

    editor::SetNodeTransformCommand missingSet{ editor::SceneNodeId{ 999 }, replacement };
    if (missingSet.execute(dispatcher.context())) {
        return TestResult::fail("SetNodeTransformCommand should reject missing nodes");
    }

    editor::SetNodeTransformCommand setTransform{ nodeId, replacement };
    if (setTransform.name() != "Set Node Transform" ||
        setTransform.undo(dispatcher.context()) ||
        setTransform.redo(dispatcher.context())) {
        return TestResult::fail("SetNodeTransformCommand should expose name and reject undo/redo before execution");
    }

    editor.clear_dirty();
    const editor::CommandResult setResult = dispatcher.execute(setTransform);
    if (!setResult ||
        !near_vec3(node->transform().position(), replacement.position()) ||
        !near_quat(node->transform().rotation(), replacement.rotation()) ||
        !near_vec3(node->transform().scale(), replacement.scale()) ||
        !has_scene_render_picking(setResult.dirtyFlags) ||
        !editor::has_flag(editor.dirty_flags(), editor::EditorDirtyFlags::Scene)) {
        return TestResult::fail("SetNodeTransformCommand should replace the node transform and report dirty flags");
    }

    if (!dispatcher.undo(setTransform) ||
        !near_vec3(node->transform().position(), original.position()) ||
        !near_vec3(node->transform().scale(), original.scale())) {
        return TestResult::fail("SetNodeTransformCommand undo should restore the previous transform");
    }

    if (!dispatcher.redo(setTransform) ||
        !near_vec3(node->transform().position(), replacement.position()) ||
        !near_vec3(node->transform().scale(), replacement.scale())) {
        return TestResult::fail("SetNodeTransformCommand redo should reapply the replacement transform");
    }

    node->transform() = original;

    editor::TranslateNodeCommand invalidTranslate{ editor::SceneNodeId{}, glm::vec3{ 1.0f } };
    if (invalidTranslate.execute(dispatcher.context())) {
        return TestResult::fail("TranslateNodeCommand should reject invalid node ids");
    }

    editor::TranslateNodeCommand missingTranslate{ editor::SceneNodeId{ 999 }, glm::vec3{ 1.0f } };
    if (missingTranslate.execute(dispatcher.context())) {
        return TestResult::fail("TranslateNodeCommand should reject missing nodes");
    }

    editor::TranslateNodeCommand translate{ nodeId, glm::vec3{ 4.0f, -2.0f, 1.0f } };
    if (translate.name() != "Translate Node" ||
        translate.undo(dispatcher.context()) ||
        translate.redo(dispatcher.context())) {
        return TestResult::fail("TranslateNodeCommand should expose name and reject undo/redo before execution");
    }

    const editor::CommandResult translateResult = dispatcher.execute(translate);
    if (!translateResult ||
        !near_vec3(node->transform().position(), glm::vec3{ 5.0f, 0.0f, 4.0f }) ||
        !near_vec3(node->transform().scale(), original.scale()) ||
        !has_scene_render_picking(translateResult.dirtyFlags)) {
        return TestResult::fail("TranslateNodeCommand should offset only the position");
    }

    if (!dispatcher.undo(translate) ||
        !near_vec3(node->transform().position(), original.position())) {
        return TestResult::fail("TranslateNodeCommand undo should restore the previous transform");
    }

    if (!dispatcher.redo(translate) ||
        !near_vec3(node->transform().position(), glm::vec3{ 5.0f, 0.0f, 4.0f })) {
        return TestResult::fail("TranslateNodeCommand redo should reapply the translated transform");
    }

    node->transform() = original;

    const glm::quat rotationDelta{ 0.0f, 0.0f, 1.0f, 0.0f };
    editor::RotateNodeCommand invalidRotate{ editor::SceneNodeId{}, rotationDelta };
    if (invalidRotate.execute(dispatcher.context())) {
        return TestResult::fail("RotateNodeCommand should reject invalid node ids");
    }

    editor::RotateNodeCommand missingRotate{ editor::SceneNodeId{ 999 }, rotationDelta };
    if (missingRotate.execute(dispatcher.context())) {
        return TestResult::fail("RotateNodeCommand should reject missing nodes");
    }

    editor::RotateNodeCommand rotate{ nodeId, rotationDelta };
    if (rotate.name() != "Rotate Node" ||
        rotate.undo(dispatcher.context()) ||
        rotate.redo(dispatcher.context())) {
        return TestResult::fail("RotateNodeCommand should expose name and reject undo/redo before execution");
    }

    const editor::CommandResult rotateResult = dispatcher.execute(rotate);
    if (!rotateResult ||
        !near_quat(node->transform().rotation(), rotationDelta) ||
        !near_vec3(node->transform().position(), original.position()) ||
        !near_vec3(node->transform().scale(), original.scale()) ||
        !has_scene_render_picking(rotateResult.dirtyFlags)) {
        return TestResult::fail("RotateNodeCommand should update only the rotation");
    }

    if (!dispatcher.undo(rotate) ||
        !near_quat(node->transform().rotation(), original.rotation())) {
        return TestResult::fail("RotateNodeCommand undo should restore the previous transform");
    }

    if (!dispatcher.redo(rotate) ||
        !near_quat(node->transform().rotation(), rotationDelta)) {
        return TestResult::fail("RotateNodeCommand redo should reapply the rotated transform");
    }

    node->transform() = original;

    editor::ScaleNodeCommand invalidScale{ editor::SceneNodeId{}, glm::vec3{ 2.0f } };
    if (invalidScale.execute(dispatcher.context())) {
        return TestResult::fail("ScaleNodeCommand should reject invalid node ids");
    }

    editor::ScaleNodeCommand missingScale{ editor::SceneNodeId{ 999 }, glm::vec3{ 2.0f } };
    if (missingScale.execute(dispatcher.context())) {
        return TestResult::fail("ScaleNodeCommand should reject missing nodes");
    }

    editor::ScaleNodeCommand scale{ nodeId, glm::vec3{ 2.0f, 0.5f, 3.0f } };
    if (scale.name() != "Scale Node" ||
        scale.undo(dispatcher.context()) ||
        scale.redo(dispatcher.context())) {
        return TestResult::fail("ScaleNodeCommand should expose name and reject undo/redo before execution");
    }

    const editor::CommandResult scaleResult = dispatcher.execute(scale);
    if (!scaleResult ||
        !near_vec3(node->transform().scale(), glm::vec3{ 4.0f, 1.5f, 12.0f }) ||
        !near_vec3(node->transform().position(), original.position()) ||
        !near_quat(node->transform().rotation(), original.rotation()) ||
        !has_scene_render_picking(scaleResult.dirtyFlags)) {
        return TestResult::fail("ScaleNodeCommand should multiply only the scale");
    }

    if (!dispatcher.undo(scale) ||
        !near_vec3(node->transform().scale(), original.scale())) {
        return TestResult::fail("ScaleNodeCommand undo should restore the previous transform");
    }

    if (!dispatcher.redo(scale) ||
        !near_vec3(node->transform().scale(), glm::vec3{ 4.0f, 1.5f, 12.0f })) {
        return TestResult::fail("ScaleNodeCommand redo should reapply the scaled transform");
    }

    editor::SetNodePivotCommand invalidPivot{ editor::SceneNodeId{}, glm::vec3{ 1.0f }, true };
    if (invalidPivot.execute(dispatcher.context())) {
        return TestResult::fail("SetNodePivotCommand should reject invalid node ids");
    }

    editor::SetNodePivotCommand missingPivot{ editor::SceneNodeId{ 999 }, glm::vec3{ 1.0f }, true };
    if (missingPivot.execute(dispatcher.context())) {
        return TestResult::fail("SetNodePivotCommand should reject missing nodes");
    }

    node->pivot().offset = glm::vec3{ -1.0f, -2.0f, -3.0f };
    node->pivot().custom = false;

    editor::NodePivot nextPivot;
    nextPivot.offset = glm::vec3{ 3.0f, 2.0f, 1.0f };
    nextPivot.custom = true;

    editor::SetNodePivotCommand setPivot{ nodeId, nextPivot };
    if (setPivot.name() != "Set Node Pivot" ||
        setPivot.undo(dispatcher.context()) ||
        setPivot.redo(dispatcher.context())) {
        return TestResult::fail("SetNodePivotCommand should expose name and reject undo/redo before execution");
    }

    const editor::CommandResult pivotResult = dispatcher.execute(setPivot);
    if (!pivotResult ||
        !near_vec3(node->pivot().offset, nextPivot.offset) ||
        !node->pivot().custom ||
        !has_scene_render_picking(pivotResult.dirtyFlags)) {
        return TestResult::fail("SetNodePivotCommand should replace the node pivot");
    }

    if (!dispatcher.undo(setPivot) ||
        !near_vec3(node->pivot().offset, glm::vec3{ -1.0f, -2.0f, -3.0f }) ||
        node->pivot().custom) {
        return TestResult::fail("SetNodePivotCommand undo should restore the previous pivot");
    }

    if (!dispatcher.redo(setPivot) ||
        !near_vec3(node->pivot().offset, nextPivot.offset) ||
        !node->pivot().custom) {
        return TestResult::fail("SetNodePivotCommand redo should reapply the new pivot");
    }

    editor::SetNodePivotCommand offsetPivot{ nodeId, glm::vec3{ 9.0f, 8.0f, 7.0f }, false };
    if (!dispatcher.execute(offsetPivot) ||
        !near_vec3(node->pivot().offset, glm::vec3{ 9.0f, 8.0f, 7.0f }) ||
        node->pivot().custom) {
        return TestResult::fail("SetNodePivotCommand offset constructor should set offset and custom flag");
    }

    return TestResult::pass();
}

} // namespace locus::tests
