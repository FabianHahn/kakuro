#ifndef SOLVER_H
#define SOLVER_H

#include "board.h"
#include "constrained_board.h"
#include <fstream>
#include <optional>
#include <random>
#include <unordered_set>

namespace kakuro {

class Solver {
public:
  Solver(
      bool solveTrivial = true,
      bool verboseLogs = true,
      bool verboseBacktracking = false,
      bool dumpBoards = false)
      : solveTrivial_{solveTrivial},
        verboseLogs_{verboseLogs},
        verboseBacktracking_{verboseBacktracking},
        dumpBoards_{dumpBoards} {}

  bool Solve(Board& board) {
    ConstrainedBoard constrainedBoard{board};
    auto solution = Solve(constrainedBoard);
    return !solution.empty();
  }

  std::vector<FillNumberUndoContext> Solve(ConstrainedBoard& board) {
    std::vector<FillNumberUndoContext> solution;

    if (solveTrivial_) {
      // Solve any initially trivial cells.
      auto trivialSolution = SolveTrivialCells(board);
      if (!trivialSolution) {
        if (verboseLogs_) {
          std::cout << "Board starting with invalid trivial solution." << std::endl;
        }
        return {};
      }
      solution.insert(solution.end(), trivialSolution->begin(), trivialSolution->end());
      if (verboseLogs_ && !trivialSolution->empty()) {
        std::cout << "Prefilled " << trivialSolution->size() << " trivial cells." << std::endl;
      }
    }

    // Need to solve free cells in a loop because there could be multiple separate regions.
    while (true) {
      auto freeCells = board.UnderlyingBoard().FindFreeCells();
      if (freeCells.empty()) {
        // If there are no more free cells, we consider the board solved.
        return solution;
      }

      const auto& cell = **freeCells.begin();
      auto subboard = board.UnderlyingBoard().FindSubboard(cell);
      if (verboseLogs_) {
        std::cout << "Attempting to solve subboard at cell (" << cell.row << ", " << cell.column
                  << ") with " << subboard.size() << " free cells." << std::endl;
      }

      auto subboardSolution = SolveCells(board, subboard);
      if (subboardSolution.empty()) {
        // If we cannot solve any individual subboard, then we cannot solve the board as a whole.
        if (verboseLogs_) {
          std::cout << "Failed to solve subboard of size " << cells_.size() << " after "
                    << backtrackIndex_ << " backtracks." << std::endl;
        }

        return {};
      }

      if (verboseLogs_) {
        std::cout << "Solved subboard of size " << cells_.size() << " after " << backtrackIndex_
                  << " backtracks." << std::endl;
      }
      solution.insert(solution.end(), subboardSolution.begin(), subboardSolution.end());
    }
  }

  std::optional<std::vector<FillNumberUndoContext>> SolveTrivialCells(ConstrainedBoard& board) {
    std::vector<FillNumberUndoContext> solution;
    int numTrivialCells = 0;
    // Trivial cells might change while we fill existing ones, so we make sure to keep checking if
    // they are empty.
    while (!board.TrivialCells().empty()) {
      auto nextTrivial = board.TrivialCells().begin();
      auto& nextTrivialCell = *nextTrivial->first;
      int nextTrivialNumber = nextTrivial->second;

      FillNumberUndoContext undo;
      if (!board.FillNumber(nextTrivialCell, nextTrivialNumber, undo)) {
        // There aren't supposed to be any conflicts for filling trivial cells, so there must be
        // contradictory board constraints.
        UndoSolution(board, solution);
        return std::nullopt;
      }
      solution.emplace_back(undo);
      numTrivialCells++;
    }
    return solution;
  }

  std::vector<FillNumberUndoContext> SolveCells(
      ConstrainedBoard& board, std::vector<const Cell*> cells) {
    backtrackIndex_ = 0;
    minimumDepth_ = 0;
    maximumDepth_ = 0;
    cells_ = cells;
    solution_.clear();
    if (!SolveCells(board, /* depth */ 0)) {
      return {};
    }

    return std::move(solution_);
  }

  void UndoSolution(ConstrainedBoard& board, const std::vector<FillNumberUndoContext>& solution) {
    for (auto iter = solution.rbegin(); iter != solution.rend(); ++iter) {
      board.UndoFillNumber(*iter);
    }
  }

private:
  bool SolveCells(ConstrainedBoard& board, int depth) {
    const Cell& cell = *cells_[depth];
    assert(!cell.isBlock);
    auto& cellConstraints = board.Constraints(cell);

    if (depth < minimumDepth_) {
      minimumDepth_ = depth;
    }

    if (depth > maximumDepth_) {
      if (verboseLogs_) {
        std::cout << "Solver first entering depth " << depth << " / " << cells_.size()
                  << " at cell (" << cell.row << ", " << cell.column << ")";
        if (minimumDepth_ < maximumDepth_) {
          std::cout << " after having backtracked to depth " << minimumDepth_;
        }
        std::cout << "." << std::endl;
      }
      maximumDepth_ = depth;
      minimumDepth_ = depth;

      if (dumpBoards_) {
        board.Dump("maxDepth", maximumDepth_);
      }
    }

    if (!cell.IsFree()) {
      if (depth == cells_.size() - 1) {
        // We've filled all the cells successfully, this is a solution!
        return true;
      }

      // We've solved this cell already, skip straight to the next.
      return SolveCells(board, depth + 1);
    }

    for (int number = 1; number <= 9; number++) {
      if (!cellConstraints.numberCandidates.Has(number)) {
        continue;
      }

      int initialSolutionSize = solution_.size();
      FillNumberUndoContext undoContext;
      if (!board.FillNumber(cell, number, undoContext)) {
        continue;
      }
      solution_.emplace_back(undoContext);

      int numTrivialCells = 0;
      if (solveTrivial_) {
        // Solve any now trivial cells.
        auto trivialSolution = SolveTrivialCells(board);
        if (!trivialSolution) {
          // Undo the filled number
          board.UndoFillNumber(solution_.back());
          solution_.pop_back();
          // The filled number makes the trivial solution invalid, so it cannot be right.
          continue;
        }
        solution_.insert(solution_.end(), trivialSolution->begin(), trivialSolution->end());
        numTrivialCells = trivialSolution->size();
      }

      if (depth + numTrivialCells == cells_.size() - 1) {
        // We've filled all the cells successfully, this is a solution!
        return true;
      }

      if (SolveCells(board, depth + 1)) {
        // All remaining cells were filled successfully, this is a solution!
        return true;
      }

      // This wasn't actually a solution, so undo the partial one we have.
      while (solution_.size() > initialSolutionSize) {
        auto undo = solution_.back();
        board.UndoFillNumber(undo);
        solution_.pop_back();
      }
    }

    if (verboseBacktracking_) {
      if (verboseLogs_) {
        std::cout << "Could not find a solution for cell (" << cell.row << ", " << cell.column
                  << ") at depth " << depth << ", backtrack index " << backtrackIndex_ << "."
                  << std::endl;
      }
      if (dumpBoards_) {
        board.Dump("backtrack", backtrackIndex_);
      }
    }
    backtrackIndex_++;

    return false;
  }

private:
  bool solveTrivial_;
  bool verboseLogs_;
  bool verboseBacktracking_;
  bool dumpBoards_;
  std::vector<const Cell*> cells_;
  std::vector<FillNumberUndoContext> solution_;
  int backtrackIndex_;
  int minimumDepth_; // counts the minimum depth since we last hit current maximum depth
  int maximumDepth_;
};

} // namespace kakuro

#endif
