#ifndef BOARD_GENERATOR_H
#define BOARD_GENERATOR_H

#include <random>
#include "critical_path_finder.h"

namespace kakuro {

class BoardGenerator {
public:
  BoardGenerator(std::mt19937& random, double blockProbability)
      : random_{random}
      , blockDistribution_{blockProbability} {}

  Board Generate(int rows, int columns) {
    Board board{rows, columns};
    CriticalPathFinder criticalPathFinder{board};

    for (int row = 1; row < rows; row++) {
      for (int column = 1; column < columns; column++) {
        auto& cell = board(row, column);

        int rowBlockDistance = cell.RowBlockDistance();
        int columnBlockDistance = cell.ColumnBlockDistance();

        int maxBlockDistance = rowBlockDistance;
        if (columnBlockDistance > rowBlockDistance) {
          maxBlockDistance = columnBlockDistance;
        }

        if (maxBlockDistance == 10) {
          board.MakeBlock(cell);
          continue;
        }

        if (rowBlockDistance == 2 || columnBlockDistance == 2) {
          continue;
        }

        if (criticalPathFinder.IsCriticalPath(cell)) {
          continue;
        }

        for (int i = 2; i < maxBlockDistance; i++) {
          if (blockDistribution_(random_)) {
            board.MakeBlock(cell);
          }
        }
      }
    }

    for (int row = 1; row < board.Rows(); row++) {
      FillThinNeighbors(board, board(row, board.Columns() - 1));
    }

    for (int column = 1; column < board.Columns(); column++) {
      FillThinNeighbors(board, board(board.Rows() - 1, column));
    }

    return board;
  }

private:
  void FillThinNeighbors(Board& board, Cell& cell) {
    if (cell.isBlock) {
      return;
    }

    int rowBlockDistance = cell.RowBlockDistance();
    int columnBlockDistance = cell.ColumnBlockDistance();

    bool isNextRowFree = false;
    if (cell.row + 1 < board.Rows() && !board(cell.row + 1, cell.column).isBlock) {
      isNextRowFree = true;
    }

    bool isNextColumnFree = false;
    if (cell.column + 1 < board.Columns() && !board(cell.row, cell.column + 1).isBlock) {
      isNextColumnFree = true;
    }

    bool isLockedInRows = columnBlockDistance == 1 && !isNextRowFree;
    bool isLockedInColumns = rowBlockDistance == 1 && !isNextColumnFree;
    if (isLockedInRows || isLockedInColumns) {
      board.MakeBlock(cell);
      FillThinNeighbors(board, board(cell.row - 1, cell.column));
      FillThinNeighbors(board, board(cell.row, cell.column - 1));

      if (cell.row + 1 < board.Rows()) {
        FillThinNeighbors(board, board(cell.row + 1, cell.column));
      }
      if (cell.column + 1 < board.Columns()) {
        FillThinNeighbors(board, board(cell.row, cell.column + 1));
      }
    }
  }

  std::mt19937& random_;
  std::bernoulli_distribution blockDistribution_;
};

}

#endif