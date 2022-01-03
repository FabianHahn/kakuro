#ifndef SOLVER_H
#define SOLVER_H

#include "board.h"
#include <random>

namespace kakuro {

class Solver {
public:
  Solver() {}

  bool Solve(Board& board) {
    while (true) {
      Cell& cell = FindFirstFreeCell(board);
      if (cell.isBlock || cell.number != 0) {
        break;
      }

      if (!SolveCell(board, cell)) {
        return false;
      }
    }

    return true;
  }

private:
  Cell& FindFirstFreeCell(Board& board) {
    for (int row = 0; row < board.Rows(); row++) {
      for (int column = 0; column < board.Columns(); column++) {
        Cell& cell = board(row, column);
        if (!cell.isBlock && cell.number == 0) {
          return cell;
        }
      }
    }

    return board(0, 0);
  }

  bool SolveCell(Board& board, Cell& cell) {
    if (cell.isBlock || cell.number != 0) {
      return false;
    }

    for (int number = 1; number <= 9; number++) {
      if (!cell.numberCandidates.Has(number)) {
        continue;
      }

      Board::FillNumberUndoContext undoContext;
      if (!board.FillNumber(cell, number, undoContext)) {
        continue;
      }

      bool noCellsVisited = true;
      bool foundSolution =
          board.ForEachFreeNeighborCell(cell, [this, &board, &noCellsVisited](Cell& currentCell) {
            noCellsVisited = false;
            return SolveCell(board, currentCell);
          });
      if (foundSolution || noCellsVisited) {
        return true;
      }

      board.UndoFillNumber(undoContext);
    }

    return false;
  }
};

} // namespace kakuro

#endif
