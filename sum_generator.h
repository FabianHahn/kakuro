#ifndef SUM_GENERATOR_H
#define SUM_GENERATOR_H

#include "board.h"
#include "combinations.h"
#include "solver.h"
#include <fstream>
#include <random>
#include <unordered_set>

namespace kakuro {

class SumGenerator {
public:
  SumGenerator(bool verboseLogs = true)
      : solver_{/* solveTrivial */ true, true, true, true}, verboseLogs_{verboseLogs} {}

  bool GenerateSums(Board& board) {
    // Solve any initially trivial cells.
    auto trivialSolution = solver_.SolveTrivialCells(board);
    if (!trivialSolution) {
      if (verboseLogs_) {
        std::cout << "Board starting with invalid trivial solution." << std::endl;
      }
      return false;
    }
    if (verboseLogs_ && !trivialSolution->empty()) {
      std::cout << "Prefilled " << trivialSolution->size() << " trivial cells." << std::endl;
    }

    while (true) {
      auto freeCells = board.FindFreeCells();
      if (freeCells.empty()) {
        // If there are no more free cells, we consider the board solved.
        return true;
      }

      auto& cell = **freeCells.begin();
      cells_ = board.FindSubboard(cell);

      if (verboseLogs_) {
        std::cout << "Verifying solvability for subboard at cell (" << cell.row << ", "
                  << cell.column << ") with " << cells_.size() << " free cells." << std::endl;
      }

      // First check if the board is solvable
      auto solution = solver_.SolveCells(board, cells_);
      if (solution.empty()) {
        if (verboseLogs_) {
          std::cout << "Encountered unsolvable subboard at cell (" << cell.row << ", "
                    << cell.column << ") with " << cells_.size() << " free cells." << std::endl;
        }
        return false;
      }
      solver_.UndoSolution(board, solution);

      blocks_ = board.FindSubboardBlocks(cells_);
      if (verboseLogs_) {
        std::cout << "Generating sums for subboard at cell (" << cell.row << ", " << cell.column
                  << ") with " << cells_.size() << " free cells and " << blocks_.size()
                  << " blocks." << std::endl;
      }
      GenerateSubboardSums(board);

      solution = solver_.SolveCells(board, cells_);
      assert(solution.size() == cells_.size());
    }
  }

private:
  // Precondition: subboard must be solvable
  void GenerateSubboardSums(Board& board) {
    while (!blocks_.empty()) {
      auto& cell = **blocks_.begin();

      if (cell.rowBlockSize > 0 && cell.rowBlockSum == 0) {
        bool chosen = ChooseRowBlockSum(board, cell);
        assert(chosen); // cannot fail because our precondition is that the subboard is solvable

        if (verboseLogs_) {
          std::cout << "Chose row block sum " << cell.rowBlockSum << " for cell (" << cell.row
                    << ", " << cell.column << ")." << std::endl;
        }
      }

      if (cell.columnBlockSize > 0 && cell.columnBlockSum == 0) {
        bool chosen = ChooseColumnBlockSum(board, cell);
        assert(chosen); // cannot fail because our precondition is that the subboard is solvable

        if (verboseLogs_) {
          std::cout << "Chose column block sum " << cell.columnBlockSum << " for cell (" << cell.row
                    << ", " << cell.column << ")." << std::endl;
        }
      }

      blocks_.erase(blocks_.begin());
    }
  }

  bool ChooseRowBlockSum(Board& board, Cell& cell) {
    for (int sum = 1; sum <= 45; sum++) {
      Board::SetSumUndoContext undo;
      if (!board.SetRowBlockSum(cell, sum, undo)) {
        continue;
      }

      std::cout << "Attempting to set row block (" << cell.row << ", " << cell.column << ") to sum "
                << sum << ": " << attempt_ << "." << std::endl;
      board.Dump("choose", attempt_++);

      auto trivialSolution = solver_.SolveTrivialCells(board);
      if (!trivialSolution) {
        // If the block sum makes any trivial solution invalid, it must be invalid itself.
        board.UndoSetSum(undo);
        continue;
      }

      auto solution = solver_.SolveCells(board, cells_);

      // Always undo the trivial solution
      solver_.UndoSolution(board, *trivialSolution);

      if (trivialSolution->size() + solution.size() == cells_.size()) {
        // This sum works, so let's undo the solution and return.
        solver_.UndoSolution(board, solution);
        return true;
      }
      assert(solution.empty());

      board.UndoSetSum(undo);
    }

    return false;
  }

  bool ChooseColumnBlockSum(Board& board, Cell& cell) {
    for (int sum = 1; sum <= 45; sum++) {
      Board::SetSumUndoContext undo;
      if (!board.SetColumnBlockSum(cell, sum, undo)) {
        continue;
      }

      std::cout << "Attempting to set column block (" << cell.row << ", " << cell.column
                << ") to sum " << sum << ": " << attempt_ << "." << std::endl;
      board.Dump("choose", attempt_++);

      auto trivialSolution = solver_.SolveTrivialCells(board);
      if (!trivialSolution) {
        // If the block sum makes any trivial solution invalid, it must be invalid itself.
        board.UndoSetSum(undo);
        continue;
      }

      auto solution = solver_.SolveCells(board, cells_);

      // Always undo the trivial solution
      solver_.UndoSolution(board, *trivialSolution);

      if (trivialSolution->size() + solution.size() == cells_.size()) {
        // This sum works, so let's undo the solution and return.
        solver_.UndoSolution(board, solution);
        return true;
      }
      assert(solution.empty());

      board.UndoSetSum(undo);
    }

    return false;
  }

  Combinations combinations_;
  Solver solver_;
  bool verboseLogs_;
  std::vector<Cell*> cells_;
  std::unordered_set<Cell*> blocks_;
  int attempt_;
};

} // namespace kakuro

#endif
