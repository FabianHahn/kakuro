#ifndef SOLVER_H
#define SOLVER_H

#include "board.h"
#include <fstream>
#include <random>
#include <unordered_set>

namespace kakuro {

class Solver {
public:
  Solver(bool verboseLogs = true, bool verboseBacktracking = false, bool dumpBoards = false)
      : verboseLogs_{verboseLogs},
        verboseBacktracking_{verboseBacktracking},
        dumpBoards_{dumpBoards} {}

  bool Solve(Board& board) {
    // Need to solve free cells in a loop because there could be multiple separate regions.
    while (true) {
      backtrackIndex_ = 0;
      numFreeCells_ = 0;
      minimumDepth_ = 0;
      maximumDepth_ = 0;

      Cell& cell = FindFirstFreeCell(board);
      if (cell.isBlock || cell.number != 0) {
        // If there are no more free cells, we consider the board solved.
        return true;
      }

      cells_ = FindSubboard(board, cell);
      if (verboseLogs_) {
        std::cout << "Attempting to solve subboard at cell (" << cell.row << ", " << cell.column
                  << ") with " << cells_.size() << " free cells." << std::endl;
      }

      solution_.clear();
      if (!SolveCells(board, /* depth */ 0)) {
        // If we cannot solve any individual subboard, then we cannot solve the board as a whole.
        return false;
      }

      if (verboseLogs_) {
        std::cout << "Solved subboard of size " << solution_.size() << " after " << backtrackIndex_
                  << " backtracks." << std::endl;
      }
    }
  }

private:
  Cell& FindFirstFreeCell(Board& board) {
    Cell* foundCell = &board(0, 0);
    for (int row = 0; row < board.Rows(); row++) {
      for (int column = 0; column < board.Columns(); column++) {
        Cell& cell = board(row, column);
        if (!cell.isBlock && cell.number == 0) {
          foundCell = &cell;
          numFreeCells_++;
        }
      }
    }

    return *foundCell;
  }

  // Finds a subboard around a given free cell
  std::vector<Cell*> FindSubboard(Board& board, Cell& cell) {
    assert(cell.IsFree());

    std::vector<Cell*> subboard{&cell};
    std::unordered_set<Cell*> visitedCells{&cell};

    // breadth first search
    int lastNewCell = -1;
    int newCells = 1;
    while (newCells > 0) {
      int firstNewCell = lastNewCell + 1;
      lastNewCell += newCells;
      newCells = 0;
      assert(lastNewCell < subboard.size());

      for (int i = firstNewCell; i <= lastNewCell; i++) {
        Cell& newCell = *subboard[i];

        board.ForEachFreeNeighborCell(
            newCell, [&subboard, &visitedCells, &newCells](Cell& currentCell) {
              if (visitedCells.find(&currentCell) == visitedCells.end()) {
                subboard.emplace_back(&currentCell);
                visitedCells.emplace(&currentCell);
                newCells++;
              }
              return true;
            });
      }
    }

    return subboard;
  }

  bool SolveCells(Board& board, int depth) {
    Cell& cell = *cells_[depth];
    assert(cell.IsFree());

    if (depth < minimumDepth_) {
      minimumDepth_ = depth;
    }

    if (depth > maximumDepth_) {
      if (verboseLogs_) {
        std::cout << "Solver first entering depth " << depth << " / " << numFreeCells_
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

    for (int number = 1; number <= 9; number++) {
      if (!cell.numberCandidates.Has(number)) {
        continue;
      }

      Board::FillNumberUndoContext undoContext;
      if (!board.FillNumber(cell, number, undoContext)) {
        continue;
      }
      solution_.emplace_back(undoContext);

      if (depth == cells_.size() - 1) {
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
  bool verboseLogs_;
  bool verboseBacktracking_;
  bool dumpBoards_;
  std::vector<Cell*> cells_;
  std::vector<Board::FillNumberUndoContext> solution_;
  int backtrackIndex_;
  int numFreeCells_;
  int minimumDepth_;
  int maximumDepth_;
};

} // namespace kakuro

#endif
