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
  const Cell* cell;
  std::vector<const Cell*> candidatesRemoved;
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

    // Go over all filled cells and update constraints as if the cell was just filled.
    auto filledCells = board_.FindFilledCells();
    for (const auto* cellPointer : filledCells) {
      const auto& cell = *cellPointer;
      UpdateCellFilledConstraints(cell);
    }

    // Go over all nonempty blocks and update sum constraints as if the sum was just filled.
    auto nonemptyBlocks = board_.FindNonemptyBlockCells();
    for (const auto* cellPointer : nonemptyBlocks) {
      const auto& cell = *cellPointer;
      if (cell.IsRowBlock() && cell.rowBlockSum > 0) {
        UpdateBlockSumSetConstraints(cell, /* isRow */ true);
      }
      if (cell.IsColumnBlock() && cell.columnBlockSum > 0) {
        UpdateBlockSumSetConstraints(cell, /* isRow */ false);
      }
    }
  }

  Board& UnderlyingBoard() { return board_; }

  const std::unordered_map<const Cell*, int>& TrivialCells() const { return trivialCells_; }

  CellConstraints& Constraints(const Cell& cell) {
    return cellConstraints_[cell.row * board_.Columns() + cell.column];
  }

  const CellConstraints& Constraints(const Cell& cell) const {
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

  bool FillNumber(const Cell& cell, int number, FillNumberUndoContext& undo) {
    if (number < 1 || number > 9) {
      return false;
    }

    if (cell.isBlock || cell.number != 0) {
      return false;
    }

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
    undo = UpdateCellFilledConstraints(cell);
    return true;
  }

  FillNumberUndoContext UpdateCellFilledConstraints(const Cell& cell) {
    FillNumberUndoContext undo;
    undo.cell = &cell;

    const auto& rowBlock = board_.RowBlock(cell);
    const auto& columnBlock = board_.ColumnBlock(cell);

    Constraints(rowBlock).rowBlockNumbers.Add(cell.number);
    Constraints(columnBlock).columnBlockNumbers.Add(cell.number);

    auto removeNumberCandidate = [&](const Cell& currentCell) {
      auto& currentCellConstraints = Constraints(currentCell);
      if (currentCellConstraints.numberCandidates.Has(cell.number)) {
        undo.candidatesRemoved.emplace_back(&currentCell);
        currentCellConstraints.numberCandidates.Remove(cell.number);
      }

      auto trivial = IsTrivialCell(currentCell);
      if (trivial) {
        trivialCells_[&currentCell] = *trivial;
      }
    };
    board_.ForEachBlockCell(columnBlock, /* isRow */ false, removeNumberCandidate);
    board_.ForEachBlockCell(rowBlock, /* isRow */ true, removeNumberCandidate);

    // A filled cell cannot be trivial anymore
    trivialCells_.erase(&cell);

    return undo;
  }

  void UndoFillNumber(const FillNumberUndoContext& undo) {
    const Cell& cell = *undo.cell;

    const Cell& rowBlock = board_.RowBlock(cell);
    const Cell& columnBlock = board_.ColumnBlock(cell);
    Constraints(rowBlock).rowBlockNumbers.Remove(cell.number);
    Constraints(columnBlock).columnBlockNumbers.Remove(cell.number);

    int number = cell.number;
    board_.SetNumber(cell, /* number */ 0);

    for (const auto* currentCellPointer : undo.candidatesRemoved) {
      const Cell& currentCell = *currentCellPointer;
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
  bool SetBlockSum(const Cell& cell, bool isRow, int sum, SetSumUndoContext& undo) {
    assert(cell.isBlock);
    assert(cell.BlockSum(isRow) == 0);
    assert(sum >= 0);
    assert(sum < 46);

    const auto& combinations = kCombinations.PerSizePerSum(sum, cell.BlockSize(isRow));
    if (combinations.numberCombinations.empty()) {
      return false;
    }

    undo.cell = &cell;
    undo.numberCandidates.clear();
    undo.isRow = isRow;

    board_.SetBlockSum(cell, isRow, sum);
    undo = UpdateBlockSumSetConstraints(cell, isRow);
    return true;
  }

  SetSumUndoContext UpdateBlockSumSetConstraints(const Cell& cell, bool isRow) {
    SetSumUndoContext undo;
    undo.cell = &cell;
    undo.numberCandidates.clear();
    undo.isRow = isRow;

    const auto& combinations =
        kCombinations.PerSizePerSum(cell.BlockSum(isRow), cell.BlockSize(isRow));
    std::array<int, 10> numberCandidateCounts{};
    std::array<const Cell*, 10> lastNumberCandidate{};
    board_.ForEachBlockCell(cell, isRow, [&](const Cell& currentCell) {
      auto& currentCellConstraints = Constraints(currentCell);
      undo.numberCandidates.push_back(currentCellConstraints.numberCandidates);
      currentCellConstraints.numberCandidates.And(combinations.possibleNumbers);
      currentCellConstraints.numberCandidates.ForEachTrue(
          [&currentCell, &numberCandidateCounts, &lastNumberCandidate](int number) {
            numberCandidateCounts[number]++;
            lastNumberCandidate[number] = &currentCell;
          });

      auto trivial = IsTrivialCell(currentCell);
      if (trivial) {
        trivialCells_[&currentCell] = *trivial;
      } else {
        trivialCells_.erase(&currentCell);
      }
    });

    combinations.necessaryNumbers.ForEachTrue([&](int number) {
      if (numberCandidateCounts[number] == 1) {
        // We know we need this number but there is only one candidate for it, so it must be here!
        const Cell& currentCell = *lastNumberCandidate[number];
        trivialCells_[&currentCell] = number;
      }
    });

    return undo;
  }

  void UndoSetSum(const SetSumUndoContext& undo) {
    auto& cell = *undo.cell;
    int i = 0;

    board_.SetBlockSum(cell, undo.isRow, 0);
    board_.ForEachBlockCell(cell, undo.isRow, [this, &i, &undo](const Cell& currentCell) {
      Constraints(currentCell).numberCandidates = undo.numberCandidates[i];
      i++;

      // If the cell is no longer trivial now, we need to remove it from the trivial cells.
      auto trivial = IsTrivialCell(currentCell);
      if (!trivial) {
        trivialCells_.erase(&currentCell);
      }
    });
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

  void AssertValidity() const {
    ConstrainedBoard other{board_};

    for (int row = 0; row < board_.Rows(); row++) {
      for (int column = 0; column < board_.Columns(); column++) {
        const auto& cell = board_(row, column);
        const auto& constraints = Constraints(cell);
        const auto& otherConstraints = other.Constraints(cell);
        assert(constraints.numberCandidates == otherConstraints.numberCandidates);
        assert(constraints.rowBlockNumbers == otherConstraints.rowBlockNumbers);
        assert(constraints.columnBlockNumbers == otherConstraints.columnBlockNumbers);
      }
    }

    assert(trivialCells_ == other.trivialCells_);
  }

private:
  Board& board_;
  std::vector<CellConstraints> cellConstraints_;
  std::unordered_map<const Cell*, int> trivialCells_;
};

} // namespace kakuro

#endif
