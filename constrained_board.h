#ifndef CONSTRAINED_BOARD_H
#define CONSTRAINED_BOARD_H

#include "board.h"
#include "combinations.h"

namespace kakuro {

namespace {
const Combinations kCombinations;
}

struct CellConstraints {
  Numbers numberCandidates;
  Numbers rowBlockNumbers;
  Numbers columnBlockNumbers;
};

struct FillNumberUndoContext {
  int row;
  int column;
  std::vector<std::pair<int, int>> candidatesRemoved;
};

struct SetSumUndoContext {
  const Cell* cell;
  std::vector<class Numbers> numberCandidates;
  bool isRow;
};

class ConstrainedBoard {
public:
  ConstrainedBoard(Board& board)
      : board_{board}, cellConstraints_{static_cast<std::size_t>(board.Rows() * board.Columns())} {
    for (int row = 0; row < board_.Rows(); row++) {
      for (int column = 0; column < board.Columns(); column++) {
        const auto& cell = board_(row, column);
        auto& cellConstraints = Constraints(cell);
        if (!cell.isBlock) {
          cellConstraints.numberCandidates.Fill();
        }
      }
    }

    // TODO: The above assumes that all non-blocks are free, so we also need to fill constraints of
    // partially filled board.
  }

  Board& UnderlyingBoard() { return board_; }

  const std::unordered_map<const Cell*, int>& TrivialCells() const { return trivialCells_; }

  CellConstraints& Constraints(const Cell& cell) {
    return cellConstraints_[cell.row * board_.Columns() + cell.column];
  }

  std::optional<int> IsTrivialCell(const Cell& cell) {
    assert(!cell.isBlock);

    if (!cell.IsFree()) {
      return std::nullopt;
    }

    auto& constraints = Constraints(cell);

    // Check if cell is trivial because neighbor constraints say there is only one possible number.
    // We also count zero as trivial because it is a trivial contradiction.
    if (constraints.numberCandidates.Count() <= 1) {
      return constraints.numberCandidates.Sum();
    }

    // Check if cell is trivial because it is the only free number left in its row block.
    const auto& rowBlock = board_.RowBlock(cell);
    if (rowBlock.rowBlockSum > 0 && rowBlock.rowBlockFree == 1) {
      return rowBlock.rowBlockSum - Constraints(rowBlock).rowBlockNumbers.Sum();
    }

    // Check if cell is trivial because it is the only free number left in its column block.
    const Cell& columnBlock = board_.ColumnBlock(cell);
    if (columnBlock.columnBlockSum > 0 && columnBlock.columnBlockFree == 1) {
      return columnBlock.columnBlockSum - Constraints(columnBlock).columnBlockNumbers.Sum();
    }

    return std::nullopt;
  }

  bool FillNumber(const Cell& cell, int number, FillNumberUndoContext& undoContext) {
    if (number < 1 || number > 9) {
      return false;
    }

    if (cell.isBlock || cell.number != 0) {
      return false;
    }

    undoContext.row = cell.row;
    undoContext.column = cell.column;
    undoContext.candidatesRemoved.clear();

    auto& cellConstraints = Constraints(cell);
    const auto& rowBlock = board_.RowBlock(cell);
    auto& rowBlockConstraints = Constraints(rowBlock);
    const auto& columnBlock = board_.ColumnBlock(cell);
    auto& columnBlockConstraints = Constraints(columnBlock);

    bool canBeNumber = cellConstraints.numberCandidates.Has(number);
    bool rowBlockHasNumber = rowBlockConstraints.rowBlockNumbers.Has(number);
    bool columnBlockHasNumber = columnBlockConstraints.columnBlockNumbers.Has(number);

    if (!canBeNumber || rowBlockHasNumber || columnBlockHasNumber) {
      return false;
    }

    if (rowBlock.rowBlockSum > 0) {
      bool isLastRowBlockNumber =
          rowBlockConstraints.rowBlockNumbers.Count() + 1 == rowBlock.rowBlockSize;
      if (isLastRowBlockNumber &&
          rowBlockConstraints.rowBlockNumbers.Sum() + number != rowBlock.rowBlockSum) {
        return false;
      }
    }

    if (columnBlock.columnBlockSum > 0) {
      bool isLastColumnBlockNumber =
          columnBlockConstraints.columnBlockNumbers.Count() + 1 == columnBlock.columnBlockSize;
      if (isLastColumnBlockNumber &&
          columnBlockConstraints.columnBlockNumbers.Sum() + number != columnBlock.columnBlockSum) {
        return false;
      }
    }

    board_.SetNumber(cell, number);
    rowBlockConstraints.rowBlockNumbers.Add(number);
    columnBlockConstraints.columnBlockNumbers.Add(number);

    auto removeNumberCandidate = [this, number, &undoContext](const Cell& currentCell) {
      auto& currentCellConstraints = Constraints(currentCell);
      if (currentCellConstraints.numberCandidates.Has(number)) {
        undoContext.candidatesRemoved.emplace_back(currentCell.row, currentCell.column);
        currentCellConstraints.numberCandidates.Remove(number);
      }

      auto trivial = IsTrivialCell(currentCell);
      if (trivial) {
        trivialCells_[&currentCell] = *trivial;
      }
    };
    board_.ForEachColumnBlockCell(columnBlock, removeNumberCandidate);
    board_.ForEachRowBlockCell(rowBlock, removeNumberCandidate);

    // A filled cell cannot be trivial anymore
    trivialCells_.erase(&cell);

    return true;
  }

  void UndoFillNumber(const FillNumberUndoContext& undoContext) {
    const Cell& cell = board_(undoContext.row, undoContext.column);

    const Cell& rowBlock = board_.RowBlock(cell);
    const Cell& columnBlock = board_.ColumnBlock(cell);
    Constraints(rowBlock).rowBlockNumbers.Remove(cell.number);
    Constraints(columnBlock).columnBlockNumbers.Remove(cell.number);

    int number = cell.number;
    board_.SetNumber(cell, /* number */ 0);

    for (auto& currentCellCoordinates : undoContext.candidatesRemoved) {
      const Cell& currentCell = board_(currentCellCoordinates.first, currentCellCoordinates.second);
      Constraints(currentCell).numberCandidates.Add(number);

      // Check if currentCell is no longer trivial because we undid the fill of cell.
      if (!IsTrivialCell(currentCell)) {
        trivialCells_.erase(&currentCell);
      }
    }

    // Check if cell is trivial now that we undid its fill.
    auto trivial = IsTrivialCell(cell);
    if (trivial) {
      trivialCells_[&cell] = *trivial;
    }
  }

  // Doesn't currently check if the sum is at all possible for this block in terms of combinations.
  bool SetRowBlockSum(const Cell& cell, int sum, SetSumUndoContext& undo) {
    assert(cell.isBlock);
    assert(cell.rowBlockSum == 0);
    assert(sum >= 0);
    assert(sum < 46);

    auto possibleCombinations = kCombinations[sum][cell.rowBlockSize];
    if (possibleCombinations.empty()) {
      return false;
    }

    undo.cell = &cell;
    undo.numberCandidates.clear();
    undo.isRow = true;

    board_.SetRowBlockSum(cell, sum);

    class Numbers possibleNumbers;
    for (auto combination : possibleCombinations) {
      possibleNumbers.Or(combination);
    }

    board_.ForEachRowBlockCell(cell, [this, &undo, &possibleNumbers](const Cell& currentCell) {
      auto& currentCellConstraints = Constraints(currentCell);
      undo.numberCandidates.push_back(currentCellConstraints.numberCandidates);
      currentCellConstraints.numberCandidates.And(possibleNumbers);

      auto trivial = IsTrivialCell(currentCell);
      if (trivial) {
        trivialCells_[&currentCell] = *trivial;
      } else {
        trivialCells_.erase(&currentCell);
      }
    });

    return true;
  }

  // Doesn't currently check if the sum is at all possible for this block in terms of combinations.
  bool SetColumnBlockSum(const Cell& cell, int sum, SetSumUndoContext& undo) {
    assert(cell.isBlock);
    assert(cell.columnBlockSum == 0);
    assert(sum >= 0);
    assert(sum < 46);

    auto possibleCombinations = kCombinations[sum][cell.columnBlockSize];
    if (possibleCombinations.empty()) {
      return false;
    }

    undo.cell = &cell;
    undo.numberCandidates.clear();
    undo.isRow = false;

    board_.SetColumnBlockSum(cell, sum);

    class Numbers possibleNumbers;
    for (auto combination : possibleCombinations) {
      possibleNumbers.Or(combination);
    }

    board_.ForEachColumnBlockCell(cell, [this, &undo, &possibleNumbers](const Cell& currentCell) {
      auto& currentCellConstraints = Constraints(currentCell);
      undo.numberCandidates.push_back(currentCellConstraints.numberCandidates);
      currentCellConstraints.numberCandidates.And(possibleNumbers);

      auto trivial = IsTrivialCell(currentCell);
      if (trivial) {
        trivialCells_[&currentCell] = *trivial;
      } else {
        trivialCells_.erase(&currentCell);
      }
    });

    return true;
  }

  void UndoSetSum(const SetSumUndoContext& undo) {
    auto& cell = *undo.cell;
    int i = 0;
    if (undo.isRow) {
      board_.SetRowBlockSum(cell, 0);
      board_.ForEachRowBlockCell(cell, [this, &i, &undo](const Cell& currentCell) {
        Constraints(currentCell).numberCandidates = undo.numberCandidates[i];
        i++;

        // If the cell is no longer trivial now, we need to remove it from the trivial cells.
        auto trivial = IsTrivialCell(currentCell);
        if (!trivial) {
          trivialCells_.erase(&currentCell);
        }
      });
    } else {
      board_.SetColumnBlockSum(cell, 0);
      board_.ForEachColumnBlockCell(cell, [this, &i, &undo](const Cell& currentCell) {
        Constraints(currentCell).numberCandidates = undo.numberCandidates[i];
        i++;

        // If the cell is no longer trivial now, we need to remove it from the trivial cells.
        auto trivial = IsTrivialCell(currentCell);
        if (!trivial) {
          trivialCells_.erase(&currentCell);
        }
      });
    }
  }

  void Dump(std::string prefix, int index) {
    std::ofstream outputFile{prefix + std::to_string(index) + ".html"};
    if (outputFile) {
      board_.RenderHtml(outputFile, [this](std::ostream& output, const Cell& cell) {
        if (cell.number > 0) {
          output << cell.number;
        } else {
          for (int i = 1; i <= 9; i++) {
            if (Constraints(cell).numberCandidates.Has(i)) {
              output << i << "?";
            }
          }
        }
      });
    }
  }

private:
  Board& board_;
  std::vector<CellConstraints> cellConstraints_;
  std::unordered_map<const Cell*, int> trivialCells_;
};

} // namespace kakuro

#endif
