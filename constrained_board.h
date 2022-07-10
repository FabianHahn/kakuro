#ifndef CONSTRAINED_BOARD_H
#define CONSTRAINED_BOARD_H

#include "board.h"

namespace kakuro {

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
  Cell* cell;
  std::vector<class Numbers> numberCandidates;
  bool isRow;
};

class ConstrainedBoard {
public:
  ConstrainedBoard(Board& board)
      : board_{board}, cellConstraints_{static_cast<std::size_t>(board.Rows() * board.Columns())} {
    for (int row = 0; row < board_.Rows(); row++) {
      for (int column = 0; column < board.Columns(); column++) {
        auto& cell = board_(row, column);
        auto& cellConstraints = Constraints(cell);
        if (!cell.isBlock) {
          cellConstraints.numberCandidates.Fill();
        }
      }
    }

    // TODO: The above assumes that all non-blocks are free, so we also need to fill constraints of
    // partially filled board.
  }

  Board& GetBoard() { return board_; }

  CellConstraints& Constraints(Cell& cell) {
    return cellConstraints_[cell.row * board_.Columns() + cell.column];
  }

  std::optional<int> IsTrivialCell(Cell& cell) {
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
    auto& rowBlock = board_.RowBlock(cell);
    if (rowBlock.rowBlockSum > 0 && rowBlock.rowBlockFree == 1) {
      return rowBlock.rowBlockSum - Constraints(rowBlock).rowBlockNumbers.Sum();
    }

    // Check if cell is trivial because it is the only free number left in its column block.
    Cell& columnBlock = board_.ColumnBlock(cell);
    if (columnBlock.columnBlockSum > 0 && columnBlock.columnBlockFree == 1) {
      return columnBlock.columnBlockSum - Constraints(columnBlock).columnBlockNumbers.Sum();
    }

    return std::nullopt;
  }

  bool FillNumber(Cell& cell, int number, FillNumberUndoContext& undoContext) {
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
    auto& rowBlock = board_.RowBlock(cell);
    auto& rowBlockConstraints = Constraints(rowBlock);
    auto& columnBlock = board_.ColumnBlock(cell);
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

    cell.number = number;
    rowBlockConstraints.rowBlockNumbers.Add(number);
    rowBlock.rowBlockFree--;
    columnBlockConstraints.columnBlockNumbers.Add(number);
    columnBlock.columnBlockFree--;

    auto removeNumberCandidate = [this, number, &undoContext](Cell& currentCell) {
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
    Cell& cell = board_(undoContext.row, undoContext.column);

    Cell& rowBlock = board_.RowBlock(cell);
    Cell& columnBlock = board_.ColumnBlock(cell);
    Constraints(rowBlock).rowBlockNumbers.Remove(cell.number);
    rowBlock.rowBlockFree++;
    Constraints(columnBlock).columnBlockNumbers.Remove(cell.number);
    columnBlock.columnBlockFree++;

    for (auto& currentCellCoordinates : undoContext.candidatesRemoved) {
      Cell& currentCell = board_(currentCellCoordinates.first, currentCellCoordinates.second);
      Constraints(currentCell).numberCandidates.Add(cell.number);

      // Check if currentCell is no longer trivial because we undid the fill of cell.
      if (!IsTrivialCell(currentCell)) {
        trivialCells_.erase(&currentCell);
      }
    }

    cell.number = 0;

    // Check if cell is trivial now that we undid its fill.
    auto trivial = IsTrivialCell(cell);
    if (trivial) {
      trivialCells_[&cell] = *trivial;
    }
  }

  // Doesn't currently check if the sum is at all possible for this block in terms of combinations.
  bool SetRowBlockSum(Cell& cell, int sum, SetSumUndoContext& undo) {
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

    cell.rowBlockSum = sum;

    class Numbers possibleNumbers;
    for (auto combination : possibleCombinations) {
      possibleNumbers.Or(combination);
    }

    board_.ForEachRowBlockCell(cell, [this, &undo, &possibleNumbers](Cell& currentCell) {
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
  bool SetColumnBlockSum(Cell& cell, int sum, SetSumUndoContext& undo) {
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

    cell.columnBlockSum = sum;

    class Numbers possibleNumbers;
    for (auto combination : possibleCombinations) {
      possibleNumbers.Or(combination);
    }

    board_.ForEachColumnBlockCell(cell, [this, &undo, &possibleNumbers](Cell& currentCell) {
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
      cell.rowBlockSum = 0;
      board_.ForEachRowBlockCell(cell, [this, &i, &undo](Cell& currentCell) {
        Constraints(currentCell).numberCandidates = undo.numberCandidates[i];
        i++;

        // If the cell is no longer trivial now, we need to remove it from the trivial cells.
        auto trivial = IsTrivialCell(currentCell);
        if (!trivial) {
          trivialCells_.erase(&currentCell);
        }
      });
    } else {
      cell.columnBlockSum = 0;
      board_.ForEachColumnBlockCell(cell, [this, &i, &undo](Cell& currentCell) {
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

  const std::unordered_map<Cell*, int>& TrivialCells() const { return trivialCells_; }

private:
  Board& board_;
  std::vector<CellConstraints> cellConstraints_;
  std::unordered_map<Cell*, int> trivialCells_;
};

} // namespace kakuro

#endif
