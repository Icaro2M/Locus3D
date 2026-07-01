#include "editor/Editor.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/command/scene/CreateEmptyNodeCommand.h"
#include "editor/command/scene/CreateMeshNodeCommand.h"
#include "editor/command/selection/ClearObjectSelectionCommand.h"
#include "editor/command/selection/SelectObjectCommand.h"
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

	std::cout << "=== Locus3D Concrete Editor Commands Smoke Test ===\n\n";

	Editor editor;
	CommandDispatcher dispatcher(editor);
	HistoryStack history{};

	const Editor& constEditor = editor;

	if (!check(editor.dirty_flags() == EditorDirtyFlags::All, "editor comeca com dirty flags All")) {
		return 1;
	}

	editor.clear_dirty();

	if (!check(constEditor.scene().tree().empty(), "cena comeca vazia")) {
		return 1;
	}

	if (!check(history.empty(), "history comeca vazio")) {
		return 1;
	}

	auto createEmpty = std::make_unique<CreateEmptyNodeCommand>("Empty A");
	CreateEmptyNodeCommand* createEmptyPtr = createEmpty.get();

	const CommandResult createEmptyResult = history.execute(dispatcher, std::move(createEmpty));

	if (!check(createEmptyResult.success, "CreateEmptyNodeCommand executou com sucesso")) {
		return 1;
	}

	const SceneNodeId emptyId = createEmptyPtr->created_node();

	if (!check(emptyId.is_valid(), "CreateEmptyNodeCommand guardou id valido")) {
		return 1;
	}

	if (!check(constEditor.scene().tree().size() == 1, "cena tem 1 node apos criar empty")) {
		return 1;
	}

	if (!check(constEditor.scene().find_node(emptyId) != nullptr, "empty criado existe na cena")) {
		return 1;
	}

	if (!check(constEditor.scene().find_node(emptyId)->metadata().name == "Empty A", "empty criado tem nome esperado")) {
		return 1;
	}

	if (!check(history.undo_size() == 1, "history guardou CreateEmptyNodeCommand")) {
		return 1;
	}

	if (!check(history.undo_name() == "Create Empty Node", "undo_name aponta para Create Empty Node")) {
		return 1;
	}

	if (!check(has_flag(editor.dirty_flags(), EditorDirtyFlags::Scene), "criar empty marcou Scene")) {
		return 1;
	}

	if (!check(has_flag(editor.dirty_flags(), EditorDirtyFlags::Render), "criar empty marcou Render")) {
		return 1;
	}

	if (!check(has_flag(editor.dirty_flags(), EditorDirtyFlags::Picking), "criar empty marcou Picking")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult undoEmptyResult = history.undo(dispatcher);

	if (!check(undoEmptyResult.success, "undo CreateEmptyNodeCommand executou com sucesso")) {
		return 1;
	}

	if (!check(constEditor.scene().tree().empty(), "undo removeu empty da cena")) {
		return 1;
	}

	if (!check(constEditor.scene().find_node(emptyId) == nullptr, "id antigo do empty nao existe apos undo")) {
		return 1;
	}

	if (!check(history.undo_size() == 0, "undo_size 0 apos undo do empty")) {
		return 1;
	}

	if (!check(history.redo_size() == 1, "redo_size 1 apos undo do empty")) {
		return 1;
	}

	if (!check(history.redo_name() == "Create Empty Node", "redo_name aponta para Create Empty Node")) {
		return 1;
	}

	if (!check(has_flag(editor.dirty_flags(), EditorDirtyFlags::Scene), "undo empty marcou Scene")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult redoEmptyResult = history.redo(dispatcher);

	if (!check(redoEmptyResult.success, "redo CreateEmptyNodeCommand executou com sucesso")) {
		return 1;
	}

	const SceneNodeId redoneEmptyId = createEmptyPtr->created_node();

	if (!check(redoneEmptyId.is_valid(), "redo guardou novo id valido")) {
		return 1;
	}

	if (!check(constEditor.scene().tree().size() == 1, "redo recriou empty na cena")) {
		return 1;
	}

	if (!check(constEditor.scene().find_node(redoneEmptyId) != nullptr, "empty recriado existe na cena")) {
		return 1;
	}

	if (!check(constEditor.scene().find_node(redoneEmptyId)->metadata().name == "Empty A", "empty recriado manteve nome")) {
		return 1;
	}

	if (!check(history.undo_size() == 1, "undo_size 1 apos redo do empty")) {
		return 1;
	}

	if (!check(history.redo_size() == 0, "redo_size 0 apos redo do empty")) {
		return 1;
	}

	editor.clear_dirty();

	auto createMesh = std::make_unique<CreateMeshNodeCommand>("Mesh A");
	CreateMeshNodeCommand* createMeshPtr = createMesh.get();

	const CommandResult createMeshResult = history.execute(dispatcher, std::move(createMesh));

	if (!check(createMeshResult.success, "CreateMeshNodeCommand executou com sucesso")) {
		return 1;
	}

	const SceneNodeId meshId = createMeshPtr->created_node();

	if (!check(meshId.is_valid(), "CreateMeshNodeCommand guardou id valido")) {
		return 1;
	}

	if (!check(constEditor.scene().tree().size() == 2, "cena tem 2 nodes apos criar mesh")) {
		return 1;
	}

	if (!check(constEditor.scene().find_mesh(meshId) != nullptr, "mesh criado existe como MeshNode")) {
		return 1;
	}

	if (!check(constEditor.scene().find_node(meshId)->metadata().name == "Mesh A", "mesh criado tem nome esperado")) {
		return 1;
	}

	if (!check(history.undo_size() == 2, "history guardou dois comandos de criacao")) {
		return 1;
	}

	if (!check(history.undo_name() == "Create Mesh Node", "undo_name aponta para Create Mesh Node")) {
		return 1;
	}

	if (!check(has_flag(editor.dirty_flags(), EditorDirtyFlags::Mesh), "criar mesh marcou Mesh")) {
		return 1;
	}

	editor.clear_dirty();

	auto selectEmpty = std::make_unique<SelectObjectCommand>(redoneEmptyId);

	const CommandResult selectEmptyResult = history.execute(dispatcher, std::move(selectEmpty));

	if (!check(selectEmptyResult.success, "SelectObjectCommand selecionou empty")) {
		return 1;
	}

	if (!check(constEditor.selection().objects().contains(redoneEmptyId), "empty esta selecionado")) {
		return 1;
	}

	if (!check(constEditor.selection().objects().active() == redoneEmptyId, "empty virou objeto ativo")) {
		return 1;
	}

	if (!check(history.undo_size() == 3, "history guardou comando de selecao do empty")) {
		return 1;
	}

	editor.clear_dirty();

	auto selectMesh = std::make_unique<SelectObjectCommand>(meshId);

	const CommandResult selectMeshResult = history.execute(dispatcher, std::move(selectMesh));

	if (!check(selectMeshResult.success, "SelectObjectCommand selecionou mesh")) {
		return 1;
	}

	if (!check(constEditor.selection().objects().contains(meshId), "mesh esta selecionado")) {
		return 1;
	}

	if (!check(!constEditor.selection().objects().contains(redoneEmptyId), "selecionar mesh substituiu selecao anterior")) {
		return 1;
	}

	if (!check(constEditor.selection().objects().active() == meshId, "mesh virou objeto ativo")) {
		return 1;
	}

	if (!check(history.undo_name() == "Select Object", "undo_name aponta para Select Object")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult undoSelectMeshResult = history.undo(dispatcher);

	if (!check(undoSelectMeshResult.success, "undo SelectObjectCommand do mesh executou com sucesso")) {
		return 1;
	}

	if (!check(constEditor.selection().objects().contains(redoneEmptyId), "undo da selecao restaurou empty selecionado")) {
		return 1;
	}

	if (!check(!constEditor.selection().objects().contains(meshId), "undo da selecao removeu mesh da selecao")) {
		return 1;
	}

	if (!check(constEditor.selection().objects().active() == redoneEmptyId, "undo da selecao restaurou active empty")) {
		return 1;
	}

	if (!check(history.redo_size() == 1, "redo_size 1 apos undo da selecao")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult redoSelectMeshResult = history.redo(dispatcher);

	if (!check(redoSelectMeshResult.success, "redo SelectObjectCommand do mesh executou com sucesso")) {
		return 1;
	}

	if (!check(constEditor.selection().objects().contains(meshId), "redo da selecao selecionou mesh novamente")) {
		return 1;
	}

	if (!check(!constEditor.selection().objects().contains(redoneEmptyId), "redo da selecao removeu empty novamente")) {
		return 1;
	}

	if (!check(constEditor.selection().objects().active() == meshId, "redo da selecao restaurou active mesh")) {
		return 1;
	}

	editor.clear_dirty();

	auto clearSelection = std::make_unique<ClearObjectSelectionCommand>();

	const CommandResult clearSelectionResult = history.execute(dispatcher, std::move(clearSelection));

	if (!check(clearSelectionResult.success, "ClearObjectSelectionCommand executou com sucesso")) {
		return 1;
	}

	if (!check(constEditor.selection().objects().empty(), "clear selection deixou selecao vazia")) {
		return 1;
	}

	if (!check(constEditor.selection().objects().active().is_invalid(), "clear selection limpou active object")) {
		return 1;
	}

	if (!check(history.undo_name() == "Clear Object Selection", "undo_name aponta para Clear Object Selection")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult undoClearSelectionResult = history.undo(dispatcher);

	if (!check(undoClearSelectionResult.success, "undo ClearObjectSelectionCommand executou com sucesso")) {
		return 1;
	}

	if (!check(constEditor.selection().objects().contains(meshId), "undo clear restaurou mesh selecionado")) {
		return 1;
	}

	if (!check(constEditor.selection().objects().active() == meshId, "undo clear restaurou active mesh")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult undoSelectMeshAgainResult = history.undo(dispatcher);

	if (!check(undoSelectMeshAgainResult.success, "undo SelectObjectCommand apos clear executou com sucesso")) {
		return 1;
	}

	if (!check(constEditor.selection().objects().contains(redoneEmptyId), "undo select mesh restaurou empty novamente")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult undoSelectEmptyResult = history.undo(dispatcher);

	if (!check(undoSelectEmptyResult.success, "undo SelectObjectCommand do empty executou com sucesso")) {
		return 1;
	}

	if (!check(constEditor.selection().objects().empty(), "undo select empty restaurou selecao inicial vazia")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult undoMeshResult = history.undo(dispatcher);

	if (!check(undoMeshResult.success, "undo CreateMeshNodeCommand executou com sucesso")) {
		return 1;
	}

	if (!check(constEditor.scene().find_mesh(meshId) == nullptr, "undo mesh removeu MeshNode")) {
		return 1;
	}

	if (!check(constEditor.scene().tree().size() == 1, "cena ficou com 1 node apos undo mesh")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult undoEmptyAgainResult = history.undo(dispatcher);

	if (!check(undoEmptyAgainResult.success, "undo CreateEmptyNodeCommand final executou com sucesso")) {
		return 1;
	}

	if (!check(constEditor.scene().tree().empty(), "cena ficou vazia apos desfazer tudo")) {
		return 1;
	}

	if (!check(!history.can_undo(), "history ficou sem undo apos desfazer tudo")) {
		return 1;
	}

	if (!check(history.can_redo(), "history tem redo apos desfazer tudo")) {
		return 1;
	}

	std::cout << "\n=== Concrete Editor Commands Smoke Test PASSED ===\n";
	return 0;
}