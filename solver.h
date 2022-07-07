#ifndef SOLVER_H
#define SOLVER_H

#include "board.h"
#include <fstream>
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
    if (solveTrivial_) {
      // Solve any initially trivial cells.
      int numTrivialCells = SolveTrivialCells(board);
      if (verboseLogs_ && numTrivialCells > 0) {
        std::cout << "Prefilled " << numTrivialCells << " trivial cells." << std::endl;
      }
    }

    // Need to solve free cells in a loop because there could be multiple separate regions.
    while (true) {
      auto freeCells = board.FindFreeCells();
      if (freeCells.empty()) {
        // If there are no more free cells, we consider the board solved.
        return true;
      }

      auto& cell = **freeCells.begin();
      auto subboard = board.FindSubboard(cell);
      if (verboseLogs_) {
        std::cout << "Attempting to solve subboard at cell (" << cell.row << ", " << cell.column
                  << ") with " << subboard.size() << " free cells." << std::endl;
      }

      if (!SolveCells(board, subboard)) {
        // If we cannot solve any individual subboard, then we cannot solve the board as a whole.
        if (verboseLogs_) {
          std::cout << "Failed to solved subboard of size " << cells_.size() << " after "
                    << backtrackIndex_ << " backtracks." << std::endl;
        }

        return false;
      }

      if (verboseLogs_) {
        std::cout << "Solved subboard of size " << cells_.size() << " after " << backtrackIndex_
                  << " backtracks." << std::endl;
      }
    }
  }

  bool SolveCells(Board& board, std::vector<Cell*> cells) {
    backtrackIndex_ = 0;
    minimumDepth_ = 0;
    maximumDepth_ = 0;
    cells_ = cells;
    solution_.clear();
    return SolveCells(board, /* depth */ 0);
  }

private:
  bool SolveCells(Board& board, int depth) {
    Cell& cell = *cells_[depth];
    assert(!cell.isBlock);

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

      DumpBoard(board, "maxDepth", maximumDepth_);
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
      if (!cell.numberCandidates.Has(number)) {
        continue;
      }

      Board::FillNumberUndoContext undoContext;
      if (!board.FillNumber(cell, number, undoContext)) {
        continue;
      }
      solution_.emplace_back(undoContext);

      int numTrivialCells = 0;
      if (solveTrivial_) {
        // Solve any now trivial cells.
        numTrivialCells = SolveTrivialCells(board);
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
      while (solution_.size() > depth) {
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
      DumpBoard(board, "backtrack", backtrackIndex_);
    }
    backtrackIndex_++;

    return false;
  }

  int SolveTrivialCells(Board& board) {
    int numTrivialCells = 0;
    // Trivial cells might change while we fill existing ones, so we make sure to keep checking if
    // they are empty.
    while (!board.TrivialCells().empty()) {
      auto nextTrivial = board.TrivialCells().begin();
      auto& nextTrivialCell = *nextTrivial->first;
      int nextTrivialNumber = nextTrivial->second;

      Board::FillNumberUndoContext undoContext;
      bool filled = board.FillNumber(nextTrivialCell, nextTrivialNumber, undoContext);
      assert(filled); // this is supposed to be trivial, so it must work
      solution_.emplace_back(undoContext);
      numTrivialCells++;
    }
    return numTrivialCells;
  }

  void DumpBoard(Board& board, std::string prefix, int index) {
    if (!dumpBoards_) {
      return;
    }

    std::ofstream outputFile{prefix + std::to_string(index) + ".html"};
    if (outputFile) {
      board.RenderHtml(outputFile);
    }
  }

private:
  bool solveTrivial_;
  bool verboseLogs_;
  bool verboseBacktracking_;
  bool dumpBoards_;
  std::vector<Cell*> cells_;
  std::vector<Board::FillNumberUndoContext> solution_;
  int backtrackIndex_;
  int minimumDepth_; // counts the minimum depth since we last hit current maximum depth
  int maximumDepth_;
};

} // namespace kakuro

#endif
