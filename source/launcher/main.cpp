#include "editor/Editor.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/command/ICommand.h"
#include "editor/history/HistoryStack.h"

#include <iostream>
#include <memory>
#include <string_view>

namespace {

	class CountingCommand final : public locus::editor::ICommand {
	public:
		explicit CountingCommand(int& value, int delta, std::string_view commandName = "Counting Command")
			: value_(&value)
			, delta_(delta)
			, name_(commandName)
		{
		}

		[[nodiscard]] std::string_view name() const override
		{
			return name_;
		}

		locus::editor::CommandResult execute(locus::editor::CommandContext& context) override
		{
			(void)context;

			*value_ += delta_;
			++executeCount_;

			return locus::editor::CommandResult::ok(
				locus::editor::EditorDirtyFlags::Scene |
				locus::editor::EditorDirtyFlags::Render,
				"Counting command executed.");
		}

		locus::editor::CommandResult undo(locus::editor::CommandContext& context) override
		{
			(void)context;

			*value_ -= delta_;
			++undoCount_;

			return locus::editor::CommandResult::ok(
				locus::editor::EditorDirtyFlags::Scene |
				locus::editor::EditorDirtyFlags::Render,
				"Counting command undone.");
		}

		locus::editor::CommandResult redo(locus::editor::CommandContext& context) override
		{
			(void)context;

			*value_ += delta_;
			++redoCount_;

			return locus::editor::CommandResult::ok(
				locus::editor::EditorDirtyFlags::Scene |
				locus::editor::EditorDirtyFlags::Render,
				"Counting command redone.");
		}

	private:
		int* value_ = nullptr;
		int delta_ = 0;
		std::string_view name_{};

		int executeCount_ = 0;
		int undoCount_ = 0;
		int redoCount_ = 0;
	};

	class NonUndoableCountingCommand final : public locus::editor::ICommand {
	public:
		explicit NonUndoableCountingCommand(int& value, int delta)
			: value_(&value)
			, delta_(delta)
		{
		}

		[[nodiscard]] std::string_view name() const override
		{
			return "Non Undoable Counting Command";
		}

		[[nodiscard]] bool is_undoable() const override
		{
			return false;
		}

		locus::editor::CommandResult execute(locus::editor::CommandContext& context) override
		{
			(void)context;

			*value_ += delta_;

			return locus::editor::CommandResult::ok(
				locus::editor::EditorDirtyFlags::Selection,
				"Non undoable counting command executed.");
		}

		locus::editor::CommandResult undo(locus::editor::CommandContext& context) override
		{
			(void)context;

			return locus::editor::CommandResult::fail(
				"Non undoable counting command cannot be undone.");
		}

	private:
		int* value_ = nullptr;
		int delta_ = 0;
	};

	class FailingCommand final : public locus::editor::ICommand {
	public:
		[[nodiscard]] std::string_view name() const override
		{
			return "Failing Command";
		}

		locus::editor::CommandResult execute(locus::editor::CommandContext& context) override
		{
			(void)context;

			return locus::editor::CommandResult::fail("Intentional execute failure.");
		}

		locus::editor::CommandResult undo(locus::editor::CommandContext& context) override
		{
			(void)context;

			return locus::editor::CommandResult::fail("Intentional undo failure.");
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

	std::cout << "=== Locus3D Editor History Smoke Test ===\n\n";

	Editor editor;
	CommandDispatcher dispatcher(editor);
	HistoryStack history{};

	if (!check(editor.dirty_flags() == EditorDirtyFlags::All, "editor comeca com dirty flags All")) {
		return 1;
	}

	editor.clear_dirty();

	if (!check(history.empty(), "history comeca vazio")) {
		return 1;
	}

	if (!check(!history.can_undo(), "history comeca sem undo")) {
		return 1;
	}

	if (!check(!history.can_redo(), "history comeca sem redo")) {
		return 1;
	}

	if (!check(history.undo_size() == 0, "undo_size inicial 0")) {
		return 1;
	}

	if (!check(history.redo_size() == 0, "redo_size inicial 0")) {
		return 1;
	}

	if (!check(history.undo_name().empty(), "undo_name inicial vazio")) {
		return 1;
	}

	if (!check(history.redo_name().empty(), "redo_name inicial vazio")) {
		return 1;
	}

	const CommandResult emptyUndoResult = history.undo(dispatcher);

	if (!check(!emptyUndoResult.success, "undo vazio falha corretamente")) {
		return 1;
	}

	const CommandResult emptyRedoResult = history.redo(dispatcher);

	if (!check(!emptyRedoResult.success, "redo vazio falha corretamente")) {
		return 1;
	}

	int value = 0;

	const CommandResult executeA = history.execute(
		dispatcher,
		std::make_unique<CountingCommand>(value, 10, "Add Ten"));

	if (!check(executeA.success, "execute Add Ten retornou sucesso")) {
		return 1;
	}

	if (!check(value == 10, "execute Add Ten alterou valor para 10")) {
		return 1;
	}

	if (!check(history.can_undo(), "history tem undo depois de execute undoable")) {
		return 1;
	}

	if (!check(!history.can_redo(), "history nao tem redo depois de execute novo")) {
		return 1;
	}

	if (!check(history.undo_size() == 1, "undo_size 1 depois de execute undoable")) {
		return 1;
	}

	if (!check(history.redo_size() == 0, "redo_size 0 depois de execute undoable")) {
		return 1;
	}

	if (!check(history.undo_name() == "Add Ten", "undo_name aponta para Add Ten")) {
		return 1;
	}

	if (!check(has_flag(editor.dirty_flags(), EditorDirtyFlags::Scene), "execute marcou dirty Scene")) {
		return 1;
	}

	if (!check(has_flag(editor.dirty_flags(), EditorDirtyFlags::Render), "execute marcou dirty Render")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult undoA = history.undo(dispatcher);

	if (!check(undoA.success, "undo Add Ten retornou sucesso")) {
		return 1;
	}

	if (!check(value == 0, "undo Add Ten voltou valor para 0")) {
		return 1;
	}

	if (!check(!history.can_undo(), "history ficou sem undo apos desfazer unico comando")) {
		return 1;
	}

	if (!check(history.can_redo(), "history tem redo apos undo")) {
		return 1;
	}

	if (!check(history.undo_size() == 0, "undo_size 0 apos undo")) {
		return 1;
	}

	if (!check(history.redo_size() == 1, "redo_size 1 apos undo")) {
		return 1;
	}

	if (!check(history.redo_name() == "Add Ten", "redo_name aponta para Add Ten")) {
		return 1;
	}

	if (!check(has_flag(editor.dirty_flags(), EditorDirtyFlags::Scene), "undo marcou dirty Scene")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult redoA = history.redo(dispatcher);

	if (!check(redoA.success, "redo Add Ten retornou sucesso")) {
		return 1;
	}

	if (!check(value == 10, "redo Add Ten voltou valor para 10")) {
		return 1;
	}

	if (!check(history.can_undo(), "history tem undo apos redo")) {
		return 1;
	}

	if (!check(!history.can_redo(), "history ficou sem redo apos redo")) {
		return 1;
	}

	if (!check(history.undo_size() == 1, "undo_size 1 apos redo")) {
		return 1;
	}

	if (!check(history.redo_size() == 0, "redo_size 0 apos redo")) {
		return 1;
	}

	editor.clear_dirty();

	const CommandResult executeB = history.execute(
		dispatcher,
		std::make_unique<CountingCommand>(value, 5, "Add Five"));

	if (!check(executeB.success, "execute Add Five retornou sucesso")) {
		return 1;
	}

	if (!check(value == 15, "execute Add Five alterou valor para 15")) {
		return 1;
	}

	if (!check(history.undo_size() == 2, "undo_size 2 depois de dois comandos")) {
		return 1;
	}

	if (!check(history.undo_name() == "Add Five", "undo_name aponta para comando mais recente")) {
		return 1;
	}

	const CommandResult undoB = history.undo(dispatcher);

	if (!check(undoB.success, "undo Add Five retornou sucesso")) {
		return 1;
	}

	if (!check(value == 10, "undo Add Five voltou valor para 10")) {
		return 1;
	}

	if (!check(history.undo_size() == 1, "undo_size 1 apos desfazer Add Five")) {
		return 1;
	}

	if (!check(history.redo_size() == 1, "redo_size 1 apos desfazer Add Five")) {
		return 1;
	}

	const CommandResult executeC = history.execute(
		dispatcher,
		std::make_unique<CountingCommand>(value, 20, "Add Twenty"));

	if (!check(executeC.success, "execute Add Twenty retornou sucesso")) {
		return 1;
	}

	if (!check(value == 30, "execute Add Twenty alterou valor para 30")) {
		return 1;
	}

	if (!check(history.undo_size() == 2, "undo_size 2 apos novo comando")) {
		return 1;
	}

	if (!check(history.redo_size() == 0, "novo comando limpou redo")) {
		return 1;
	}

	const CommandResult nonUndoableResult = history.execute(
		dispatcher,
		std::make_unique<NonUndoableCountingCommand>(value, 7));

	if (!check(nonUndoableResult.success, "execute nao undoable retornou sucesso")) {
		return 1;
	}

	if (!check(value == 37, "comando nao undoable alterou valor para 37")) {
		return 1;
	}

	if (!check(history.undo_size() == 2, "comando nao undoable nao entrou no undo")) {
		return 1;
	}

	if (!check(history.redo_size() == 0, "comando nao undoable manteve redo vazio")) {
		return 1;
	}

	const CommandResult failingResult = history.execute(
		dispatcher,
		std::make_unique<FailingCommand>());

	if (!check(!failingResult.success, "execute com comando falho retorna falha")) {
		return 1;
	}

	if (!check(history.undo_size() == 2, "comando falho nao entrou no undo")) {
		return 1;
	}

	const CommandResult nullResult = history.execute(dispatcher, std::unique_ptr<ICommand>{});

	if (!check(!nullResult.success, "execute com comando nulo falha corretamente")) {
		return 1;
	}

	if (!check(history.undo_size() == 2, "comando nulo nao entrou no undo")) {
		return 1;
	}

	history.set_max_entries(1);

	if (!check(history.max_entries() == 1, "max_entries configurado para 1")) {
		return 1;
	}

	if (!check(history.undo_size() == 1, "set_max_entries aparou undo para 1")) {
		return 1;
	}

	if (!check(history.undo_name() == "Add Twenty", "apos trim, undo_name manteve comando mais recente")) {
		return 1;
	}

	const CommandResult executeD = history.execute(
		dispatcher,
		std::make_unique<CountingCommand>(value, 3, "Add Three"));

	if (!check(executeD.success, "execute Add Three retornou sucesso")) {
		return 1;
	}

	if (!check(value == 40, "execute Add Three alterou valor para 40")) {
		return 1;
	}

	if (!check(history.undo_size() == 1, "max_entries manteve undo_size em 1")) {
		return 1;
	}

	if (!check(history.undo_name() == "Add Three", "undo_name aponta para Add Three apos limite")) {
		return 1;
	}

	const CommandResult undoD = history.undo(dispatcher);

	if (!check(undoD.success, "undo Add Three retornou sucesso")) {
		return 1;
	}

	if (!check(value == 37, "undo Add Three voltou valor para 37")) {
		return 1;
	}

	if (!check(history.undo_size() == 0, "undo_size 0 apos undo com limite 1")) {
		return 1;
	}

	if (!check(history.redo_size() == 1, "redo_size 1 apos undo com limite 1")) {
		return 1;
	}

	history.clear_redo();

	if (!check(!history.can_redo(), "clear_redo remove redo")) {
		return 1;
	}

	if (!check(history.redo_size() == 0, "redo_size 0 apos clear_redo")) {
		return 1;
	}

	history.clear();

	if (!check(history.empty(), "clear remove todo historico")) {
		return 1;
	}

	if (!check(history.undo_size() == 0, "undo_size 0 apos clear")) {
		return 1;
	}

	if (!check(history.redo_size() == 0, "redo_size 0 apos clear")) {
		return 1;
	}

	std::cout << "\n=== Editor History Smoke Test PASSED ===\n";
	return 0;
}