#include "editor/Editor.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/command/scene/CreateEmptyNodeCommand.h"
#include "editor/command/scene/RenameNodeCommand.h"
#include "editor/command/scene/SetNodeLockCommand.h"
#include "editor/command/scene/SetNodeSelectableCommand.h"
#include "editor/command/scene/SetNodeVisibilityCommand.h"
#include "editor/history/HistoryStack.h"
#include "editor/scene/SceneNode.h"

#include <iostream>
#include <memory>

namespace {

	bool check(bool condition, const char* message)
	{
		if (condition) {
			std::cout << "[OK] " << message << '\n';
			return true;
		}

		std::cout << "[FAIL] " << message << '\n';
		return false;
	}

}

int main()
{
	using namespace locus::editor;

	std::cout << "=== Locus3D Node Metadata Commands Smoke Test ===\n\n";

	Editor editor;
	CommandDispatcher dispatcher(editor);
	HistoryStack history{};

	const Editor& constEditor = editor;

	if (!check(editor.dirty_flags() == EditorDirtyFlags::All, "editor comeca com dirty flags All")) {
		return 1;
	}

	editor.clear_dirty();

	auto createNode = std::make_unique<CreateEmptyNodeCommand>("Original Name");
	CreateEmptyNodeCommand* createNodePtr = createNode.get();

	const CommandResult createResult = history.execute(dispatcher, std::move(createNode));

	if (!check(createResult.success, "CreateEmptyNodeCommand executou com sucesso")) {
		return 1;
	}

	const SceneNodeId nodeId = createNodePtr->created_node();

	if (!check(nodeId.is_valid(), "node criado tem id valido")) {
		return 1;
	}

	const SceneNode* constNode = constEditor.scene().find_node(nodeId);

	if (!check(constNode != nullptr, "node criado existe na cena")) {
		return 1;
	}

	if (!check(constNode->metadata().name == "Original Name", "node comecou com nome original")) {
		return 1;
	}

	if (!check(constNode->metadata().visible, "node comecou visible true")) {
		return 1;
	}

	if (!check(constNode->metadata().selectable, "node comecou selectable true")) {
		return 1;
	}

	if (!check(!constNode->metadata().locked, "node comecou locked false")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult renameResult = history.execute(
		dispatcher,
		std::make_unique<RenameNodeCommand>(nodeId, "Renamed Node"));

	if (!check(renameResult.success, "RenameNodeCommand executou com sucesso")) {
		return 1;
	}

	if (!check(constEditor.scene().find_node(nodeId)->metadata().name == "Renamed Node", "rename alterou nome")) {
		return 1;
	}

	if (!check(history.undo_name() == "Rename Node", "undo_name aponta para Rename Node")) {
		return 1;
	}

	if (!check(has_flag(editor.dirty_flags(), EditorDirtyFlags::Scene), "rename marcou Scene")) {
		return 1;
	}

	if (!check(has_flag(editor.dirty_flags(), EditorDirtyFlags::Render), "rename marcou Render")) {
		return 1;
	}

	if (!check(has_flag(editor.dirty_flags(), EditorDirtyFlags::Picking), "rename marcou Picking")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult undoRenameResult = history.undo(dispatcher);

	if (!check(undoRenameResult.success, "undo RenameNodeCommand executou com sucesso")) {
		return 1;
	}

	if (!check(constEditor.scene().find_node(nodeId)->metadata().name == "Original Name", "undo rename restaurou nome original")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult redoRenameResult = history.redo(dispatcher);

	if (!check(redoRenameResult.success, "redo RenameNodeCommand executou com sucesso")) {
		return 1;
	}

	if (!check(constEditor.scene().find_node(nodeId)->metadata().name == "Renamed Node", "redo rename reaplicou novo nome")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult emptyRenameResult = history.execute(
		dispatcher,
		std::make_unique<RenameNodeCommand>(nodeId, ""));

	if (!check(!emptyRenameResult.success, "RenameNodeCommand rejeita nome vazio")) {
		return 1;
	}

	if (!check(constEditor.scene().find_node(nodeId)->metadata().name == "Renamed Node", "rename vazio nao alterou nome")) {
		return 1;
	}

	if (!check(history.undo_name() == "Rename Node", "comando falho nao entrou no historico")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult setInvisibleResult = history.execute(
		dispatcher,
		std::make_unique<SetNodeVisibilityCommand>(nodeId, false));

	if (!check(setInvisibleResult.success, "SetNodeVisibilityCommand executou com sucesso")) {
		return 1;
	}

	if (!check(!constEditor.scene().find_node(nodeId)->metadata().visible, "visibility alterou visible para false")) {
		return 1;
	}

	if (!check(history.undo_name() == "Set Node Visibility", "undo_name aponta para Set Node Visibility")) {
		return 1;
	}

	if (!check(has_flag(editor.dirty_flags(), EditorDirtyFlags::Selection), "visibility marcou Selection")) {
		return 1;
	}

	if (!check(has_flag(editor.dirty_flags(), EditorDirtyFlags::Render), "visibility marcou Render")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult undoVisibilityResult = history.undo(dispatcher);

	if (!check(undoVisibilityResult.success, "undo SetNodeVisibilityCommand executou com sucesso")) {
		return 1;
	}

	if (!check(constEditor.scene().find_node(nodeId)->metadata().visible, "undo visibility restaurou visible true")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult redoVisibilityResult = history.redo(dispatcher);

	if (!check(redoVisibilityResult.success, "redo SetNodeVisibilityCommand executou com sucesso")) {
		return 1;
	}

	if (!check(!constEditor.scene().find_node(nodeId)->metadata().visible, "redo visibility reaplicou visible false")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult setNotSelectableResult = history.execute(
		dispatcher,
		std::make_unique<SetNodeSelectableCommand>(nodeId, false));

	if (!check(setNotSelectableResult.success, "SetNodeSelectableCommand executou com sucesso")) {
		return 1;
	}

	if (!check(!constEditor.scene().find_node(nodeId)->metadata().selectable, "selectable alterou selectable para false")) {
		return 1;
	}

	if (!check(history.undo_name() == "Set Node Selectable", "undo_name aponta para Set Node Selectable")) {
		return 1;
	}

	if (!check(has_flag(editor.dirty_flags(), EditorDirtyFlags::Selection), "selectable marcou Selection")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult undoSelectableResult = history.undo(dispatcher);

	if (!check(undoSelectableResult.success, "undo SetNodeSelectableCommand executou com sucesso")) {
		return 1;
	}

	if (!check(constEditor.scene().find_node(nodeId)->metadata().selectable, "undo selectable restaurou selectable true")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult redoSelectableResult = history.redo(dispatcher);

	if (!check(redoSelectableResult.success, "redo SetNodeSelectableCommand executou com sucesso")) {
		return 1;
	}

	if (!check(!constEditor.scene().find_node(nodeId)->metadata().selectable, "redo selectable reaplicou selectable false")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult setLockedResult = history.execute(
		dispatcher,
		std::make_unique<SetNodeLockCommand>(nodeId, true));

	if (!check(setLockedResult.success, "SetNodeLockCommand executou com sucesso")) {
		return 1;
	}

	if (!check(constEditor.scene().find_node(nodeId)->metadata().locked, "lock alterou locked para true")) {
		return 1;
	}

	if (!check(history.undo_name() == "Set Node Lock", "undo_name aponta para Set Node Lock")) {
		return 1;
	}

	if (!check(has_flag(editor.dirty_flags(), EditorDirtyFlags::Selection), "lock marcou Selection")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult undoLockResult = history.undo(dispatcher);

	if (!check(undoLockResult.success, "undo SetNodeLockCommand executou com sucesso")) {
		return 1;
	}

	if (!check(!constEditor.scene().find_node(nodeId)->metadata().locked, "undo lock restaurou locked false")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult redoLockResult = history.redo(dispatcher);

	if (!check(redoLockResult.success, "redo SetNodeLockCommand executou com sucesso")) {
		return 1;
	}

	if (!check(constEditor.scene().find_node(nodeId)->metadata().locked, "redo lock reaplicou locked true")) {
		return 1;
	}

	editor.clear_dirty();

	if (!check(history.undo_size() == 5, "history tem 5 comandos undoable validos")) {
		return 1;
	}

	const CommandResult undoLockAgainResult = history.undo(dispatcher);

	if (!check(undoLockAgainResult.success, "undo final do lock executou com sucesso")) {
		return 1;
	}

	if (!check(!constEditor.scene().find_node(nodeId)->metadata().locked, "undo final removeu lock")) {
		return 1;
	}

	const CommandResult undoSelectableAgainResult = history.undo(dispatcher);

	if (!check(undoSelectableAgainResult.success, "undo final do selectable executou com sucesso")) {
		return 1;
	}

	if (!check(constEditor.scene().find_node(nodeId)->metadata().selectable, "undo final restaurou selectable")) {
		return 1;
	}

	const CommandResult undoVisibilityAgainResult = history.undo(dispatcher);

	if (!check(undoVisibilityAgainResult.success, "undo final da visibility executou com sucesso")) {
		return 1;
	}

	if (!check(constEditor.scene().find_node(nodeId)->metadata().visible, "undo final restaurou visible")) {
		return 1;
	}

	const CommandResult undoRenameAgainResult = history.undo(dispatcher);

	if (!check(undoRenameAgainResult.success, "undo final do rename executou com sucesso")) {
		return 1;
	}

	if (!check(constEditor.scene().find_node(nodeId)->metadata().name == "Original Name", "undo final restaurou nome original")) {
		return 1;
	}

	const CommandResult undoCreateResult = history.undo(dispatcher);

	if (!check(undoCreateResult.success, "undo final da criacao executou com sucesso")) {
		return 1;
	}

	if (!check(constEditor.scene().tree().empty(), "undo final removeu node da cena")) {
		return 1;
	}

	if (!check(!history.can_undo(), "history ficou sem undo no final")) {
		return 1;
	}

	if (!check(history.can_redo(), "history tem redo no final")) {
		return 1;
	}

	std::cout << "\n=== Node Metadata Commands Smoke Test PASSED ===\n";
	return 0;
}