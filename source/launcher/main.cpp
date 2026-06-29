#include "editor/Editor.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/command/CommandRegistry.h"
#include "editor/command/ICommand.h"

#include <iostream>
#include <memory>
#include <string_view>

namespace {

	class FakeSceneCommand final : public locus::editor::ICommand {
	public:
		[[nodiscard]] std::string_view name() const override
		{
			return "Fake Scene Command";
		}

		locus::editor::CommandResult execute(locus::editor::CommandContext& context) override
		{
			++executeCount_;

			context.scene().create_empty("Command Empty");

			return locus::editor::CommandResult::ok(
				locus::editor::EditorDirtyFlags::Scene |
				locus::editor::EditorDirtyFlags::Render |
				locus::editor::EditorDirtyFlags::Picking,
				"Fake scene command executed.");
		}

		locus::editor::CommandResult undo(locus::editor::CommandContext& context) override
		{
			(void)context;

			++undoCount_;

			return locus::editor::CommandResult::ok(
				locus::editor::EditorDirtyFlags::Scene |
				locus::editor::EditorDirtyFlags::Render |
				locus::editor::EditorDirtyFlags::Picking,
				"Fake scene command undone.");
		}

		locus::editor::CommandResult redo(locus::editor::CommandContext& context) override
		{
			(void)context;

			++redoCount_;

			return locus::editor::CommandResult::ok(
				locus::editor::EditorDirtyFlags::Scene |
				locus::editor::EditorDirtyFlags::Render |
				locus::editor::EditorDirtyFlags::Picking,
				"Fake scene command redone.");
		}

		[[nodiscard]] int execute_count() const
		{
			return executeCount_;
		}

		[[nodiscard]] int undo_count() const
		{
			return undoCount_;
		}

		[[nodiscard]] int redo_count() const
		{
			return redoCount_;
		}

	private:
		int executeCount_ = 0;
		int undoCount_ = 0;
		int redoCount_ = 0;
	};

	class FakeSelectionCommand final : public locus::editor::ICommand {
	public:
		[[nodiscard]] std::string_view name() const override
		{
			return "Fake Selection Command";
		}

		locus::editor::CommandResult execute(locus::editor::CommandContext& context) override
		{
			context.selection().clear();

			return locus::editor::CommandResult::ok(
				locus::editor::EditorDirtyFlags::Selection,
				"Fake selection command executed.");
		}

		locus::editor::CommandResult undo(locus::editor::CommandContext& context) override
		{
			context.selection().clear();

			return locus::editor::CommandResult::ok(
				locus::editor::EditorDirtyFlags::Selection,
				"Fake selection command undone.");
		}
	};

	class NonUndoableCommand final : public locus::editor::ICommand {
	public:
		[[nodiscard]] std::string_view name() const override
		{
			return "Non Undoable Command";
		}

		[[nodiscard]] bool is_undoable() const override
		{
			return false;
		}

		locus::editor::CommandResult execute(locus::editor::CommandContext& context) override
		{
			(void)context;

			return locus::editor::CommandResult::ok(
				locus::editor::EditorDirtyFlags::None,
				"Non undoable command executed.");
		}

		locus::editor::CommandResult undo(locus::editor::CommandContext& context) override
		{
			(void)context;

			return locus::editor::CommandResult::fail(
				"Non undoable command cannot be undone.");
		}
	};

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

	std::cout << "=== Locus3D Editor Command Smoke Test ===\n\n";

	Editor editor;
	CommandDispatcher dispatcher(editor);

	const Editor& constEditor = editor;

	if (!check(editor.dirty_flags() == EditorDirtyFlags::All, "editor comeca com dirty flags All")) {
		return 1;
	}

	editor.clear_dirty();

	if (!check(editor.dirty_flags() == EditorDirtyFlags::None, "clear_dirty deixa editor sem dirty flags")) {
		return 1;
	}

	if (!check(&dispatcher.context().editor() == &editor, "dispatcher aponta para o editor correto")) {
		return 1;
	}

	FakeSceneCommand sceneCommand{};

	const CommandResult executeResult = dispatcher.execute(sceneCommand);

	if (!check(executeResult.success, "execute retornou sucesso")) {
		return 1;
	}

	if (!check(sceneCommand.execute_count() == 1, "execute chamado uma vez")) {
		return 1;
	}

	if (!check(constEditor.scene().tree().size() == 1, "comando criou um node na cena")) {
		return 1;
	}

	if (!check(has_flag(editor.dirty_flags(), EditorDirtyFlags::Scene), "dirty flag Scene marcada")) {
		return 1;
	}

	if (!check(has_flag(editor.dirty_flags(), EditorDirtyFlags::Render), "dirty flag Render marcada")) {
		return 1;
	}

	if (!check(has_flag(editor.dirty_flags(), EditorDirtyFlags::Picking), "dirty flag Picking marcada")) {
		return 1;
	}

	if (!check(dispatcher.last_result().message == "Fake scene command executed.", "last_result guarda mensagem do execute")) {
		return 1;
	}

	editor.clear_dirty();

	if (!check(editor.dirty_flags() == EditorDirtyFlags::None, "clear_dirty limpou flags")) {
		return 1;
	}

	const CommandResult undoResult = dispatcher.undo(sceneCommand);

	if (!check(undoResult.success, "undo retornou sucesso")) {
		return 1;
	}

	if (!check(sceneCommand.undo_count() == 1, "undo chamado uma vez")) {
		return 1;
	}

	if (!check(has_flag(editor.dirty_flags(), EditorDirtyFlags::Scene), "undo marcou dirty flag Scene")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult redoResult = dispatcher.redo(sceneCommand);

	if (!check(redoResult.success, "redo retornou sucesso")) {
		return 1;
	}

	if (!check(sceneCommand.redo_count() == 1, "redo chamado uma vez")) {
		return 1;
	}

	if (!check(has_flag(editor.dirty_flags(), EditorDirtyFlags::Scene), "redo marcou dirty flag Scene")) {
		return 1;
	}

	editor.clear_dirty();

	auto ownedSelectionCommand = std::make_unique<FakeSelectionCommand>();
	const CommandResult ownedExecuteResult = dispatcher.execute(std::move(ownedSelectionCommand));

	if (!check(ownedExecuteResult.success, "execute com unique_ptr retornou sucesso")) {
		return 1;
	}

	if (!check(has_flag(editor.dirty_flags(), EditorDirtyFlags::Selection), "comando owned marcou Selection")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult nullExecuteResult = dispatcher.execute(std::unique_ptr<ICommand>{});

	if (!check(!nullExecuteResult.success, "execute com comando nulo falha corretamente")) {
		return 1;
	}

	if (!check(!nullExecuteResult.message.empty(), "falha de comando nulo tem mensagem")) {
		return 1;
	}

	NonUndoableCommand nonUndoableCommand{};
	const CommandResult nonUndoableExecuteResult = dispatcher.execute(nonUndoableCommand);

	if (!check(nonUndoableExecuteResult.success, "comando nao undoable executa normalmente")) {
		return 1;
	}

	const CommandResult nonUndoableUndoResult = dispatcher.undo(nonUndoableCommand);

	if (!check(!nonUndoableUndoResult.success, "undo de comando nao undoable falha")) {
		return 1;
	}

	CommandRegistry registry{};

	if (!check(registry.empty(), "registry comeca vazio")) {
		return 1;
	}

	const bool registered = registry.register_command(
		"test.fake_selection",
		[]() {
			return std::make_unique<FakeSelectionCommand>();
		});

	if (!check(registered, "registry registra comando")) {
		return 1;
	}

	if (!check(registry.contains("test.fake_selection"), "registry contem id registrado")) {
		return 1;
	}

	if (!check(registry.size() == 1, "registry size 1")) {
		return 1;
	}

	auto createdCommand = registry.create("test.fake_selection");

	if (!check(createdCommand != nullptr, "registry cria comando registrado")) {
		return 1;
	}

	if (!check(createdCommand->name() == "Fake Selection Command", "comando criado tem nome esperado")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult registryExecuteResult = dispatcher.execute(*createdCommand);

	if (!check(registryExecuteResult.success, "comando criado pelo registry executa")) {
		return 1;
	}

	if (!check(has_flag(editor.dirty_flags(), EditorDirtyFlags::Selection), "comando criado pelo registry marcou Selection")) {
		return 1;
	}

	const bool duplicateRegistered = registry.register_command(
		"test.fake_selection",
		[]() {
			return std::make_unique<FakeSceneCommand>();
		});

	if (!check(!duplicateRegistered, "registry rejeita id duplicado em register_command")) {
		return 1;
	}

	const bool replaced = registry.replace_command(
		"test.fake_selection",
		[]() {
			return std::make_unique<FakeSceneCommand>();
		});

	if (!check(replaced, "registry substitui comando")) {
		return 1;
	}

	auto replacedCommand = registry.create("test.fake_selection");

	if (!check(replacedCommand != nullptr, "registry cria comando substituido")) {
		return 1;
	}

	if (!check(replacedCommand->name() == "Fake Scene Command", "comando substituido tem nome esperado")) {
		return 1;
	}

	const auto ids = registry.command_ids();

	if (!check(ids.size() == 1, "registry lista um id")) {
		return 1;
	}

	const bool removed = registry.unregister_command("test.fake_selection");

	if (!check(removed, "registry remove comando")) {
		return 1;
	}

	if (!check(!registry.contains("test.fake_selection"), "registry nao contem id removido")) {
		return 1;
	}

	if (!check(registry.create("test.fake_selection") == nullptr, "registry retorna nullptr para id inexistente")) {
		return 1;
	}

	registry.clear();

	if (!check(registry.empty(), "registry clear deixa vazio")) {
		return 1;
	}

	std::cout << "\n=== Editor Command Smoke Test PASSED ===\n";
	return 0;
}