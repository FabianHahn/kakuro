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

    return board;
  }

private:
  std::mt19937& random_;
  std::bernoulli_distribution blockDistribution_;
};

}

#endif