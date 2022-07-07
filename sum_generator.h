#ifndef SUM_GENERATOR_H
#define SUM_GENERATOR_H

#include "board.h"
#include "solver.h"
#include <fstream>
#include <random>
#include <unordered_set>

namespace kakuro {

class SumGenerator {
public:
  SumGenerator(bool verboseLogs = true)
      : solver_{/* solveTrivial */ true}, verboseLogs_{verboseLogs} {}

  bool GenerateSums(Board& board) {
    // Solve any initially trivial cells.
    auto trivialSolution = solver_.SolveTrivialCells(board);
    if (verboseLogs_ && !trivialSolution.empty()) {
      std::cout << "Prefilled " << trivialSolution.size() << " trivial cells." << std::endl;
    }

    while (true) {
      auto freeCells = board.FindFreeCells();
      if (freeCells.empty()) {
        // If there are no more free cells, we consider the board solved.
        return true;
      }

      auto& cell = **freeCells.begin();
      cells_ = board.FindSubboard(cell);

      // First check if the board is solvable
      auto solution = solver_.SolveCells(board, cells_);
      if (solution.empty()) {
        if (verboseLogs_) {
          std::cout << "Encountered unsolvable subboard at cell (" << cell.row << ", "
                    << cell.column << ") with " << cells_.size() << " free cells." << std::endl;
        }
        return false;
      }
      // Undo the solution again.
      for (auto undo : solution) {
        board.UndoFillNumber(undo);
      }

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
      }

      if (cell.columnBlockSize > 0 && cell.columnBlockSum == 0) {
        bool chosen = ChooseColumnBlockSum(board, cell);
        assert(chosen); // cannot fail because our precondition is that the subboard is solvable
      }

      blocks_.erase(blocks_.begin());
    }
  }

  bool ChooseRowBlockSum(Board& board, Cell& cell) {
    for (int i = 1; i <= 45; i++) {
      board.SetRowBlockSum(cell, i);

      auto trivialSolution = solver_.SolveTrivialCells(board);
      auto solution = solver_.SolveCells(board, cells_);

      // Always undo the trivial solution
      for (auto undo : trivialSolution) {
        board.UndoFillNumber(undo);
      }

      if (trivialSolution.size() + solution.size() == cells_.size()) {
        // This sum works, so let's undo the solution and return.
        for (auto undo : solution) {
          board.UndoFillNumber(undo);
        }
        return true;
      }
    }

    board.SetRowBlockSum(cell, 0);
    return false;
  }

  bool ChooseColumnBlockSum(Board& board, Cell& cell) {
    for (int i = 1; i <= 45; i++) {
      board.SetColumnBlockSum(cell, i);

      auto trivialSolution = solver_.SolveTrivialCells(board);
      auto solution = solver_.SolveCells(board, cells_);

      // Always undo the trivial solution
      for (auto undo : trivialSolution) {
        board.UndoFillNumber(undo);
      }

      if (trivialSolution.size() + solution.size() == cells_.size()) {
        // This sum works, so let's undo the solution and return.
        for (auto undo : solution) {
          board.UndoFillNumber(undo);
        }
        return true;
      }
      assert(solution.empty());
    }

    board.SetColumnBlockSum(cell, 0);
    return false;
  }

  Solver solver_;
  bool verboseLogs_;
  std::vector<Cell*> cells_;
  std::unordered_set<Cell*> blocks_;
};

} // namespace kakuro

#endif
