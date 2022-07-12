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

struct TrivialityChange {
  const Cell* cell;
  std::optional<int> previousTriviality;
};

struct FillNumberUndoContext {
  const Cell* cell;
  Numbers previousNumberCandidates;
  std::vector<const Cell*> candidatesRemoved;
  std::vector<TrivialityChange> trivialityChanges;
};

struct SetSumUndoContext {
  const Cell* cell;
  std::vector<class Numbers> numberCandidates;
  std::vector<TrivialityChange> trivialityChanges;
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
    // Since trivial computation depends on block numbers, we need to fill those first.
    auto filledCells = board_.FindFilledCells();
    for (const auto* cellPointer : filledCells) {
      const auto& cell = *cellPointer;
      Constraints(board_.RowBlock(cell)).rowBlockNumbers.Add(cell.number);
      Constraints(board_.ColumnBlock(cell)).columnBlockNumbers.Add(cell.number);
    }
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
      int leftover = rowBlock.rowBlockSum - Constraints(rowBlock).rowBlockNumbers.Sum();
      if (leftover < 1 || leftover > 9) {
        return 0; // contradiction
      }
      return leftover;
    }

    // Check if cell is trivial because it is the only free number left in its column block.
    const Cell& columnBlock = board_.ColumnBlock(cell);
    if (columnBlock.columnBlockSum > 0 && columnBlock.columnBlockFree == 1) {
      int leftover = columnBlock.columnBlockSum - Constraints(columnBlock).columnBlockNumbers.Sum();
      if (leftover < 1 || leftover > 9) {
        return 0; // contradiction
      }
      return leftover;
    }

    return std::nullopt;
  }

  TrivialityChange ChangeTriviality(const Cell& cell, std::optional<int> trivial) {
    TrivialityChange change;
    change.cell = &cell;

    auto trivialityQuery = trivialCells_.find(&cell);
    if (trivialityQuery != trivialCells_.end()) {
      change.previousTriviality = trivialityQuery->second;
    } else {
      change.previousTriviality = std::nullopt;
    }

    if (trivial) {
      if (change.previousTriviality && change.previousTriviality != *trivial) {
        // If it was already trivial before but is now trivial different, this is a contradiction,
        // which we'll mark as trivially zero.
        trivialCells_[&cell] = 0;
      } else {
        trivialCells_[&cell] = *trivial;
      }
    } else {
      trivialCells_.erase(&cell);
    }

    return change;
  }

  bool FillNumber(const Cell& cell, int number, FillNumberUndoContext& undo) {
    assert(cell.IsFree());

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
    assert(numberCandidatesRemoved_.empty());
    CellConstraints& constraints = Constraints(cell);

    FillNumberUndoContext undo;
    undo.cell = &cell;
    undo.previousNumberCandidates = constraints.numberCandidates;

    // Remove all number candidates.
    constraints.numberCandidates.Clear();

    const auto& rowBlock = board_.RowBlock(cell);
    const auto& columnBlock = board_.ColumnBlock(cell);

    Constraints(rowBlock).rowBlockNumbers.Add(cell.number);
    Constraints(columnBlock).columnBlockNumbers.Add(cell.number);

    auto removeNumberCandidate = [&](const Cell& currentCell) {
      if (currentCell.IsFilled()) {
        return;
      }

      auto& currentCellConstraints = Constraints(currentCell);
      if (currentCellConstraints.numberCandidates.Has(cell.number)) {
        undo.candidatesRemoved.emplace_back(&currentCell);
        currentCellConstraints.numberCandidates.Remove(cell.number);
        numberCandidatesRemoved_[&currentCell].Add(cell.number);
      }

      auto trivial = IsTrivialCell(currentCell);
      if (trivial) {
        undo.trivialityChanges.emplace_back(ChangeTriviality(currentCell, trivial));
      }
    };
    board_.ForEachBlockCell(columnBlock, /* isRow */ false, removeNumberCandidate);
    board_.ForEachBlockCell(rowBlock, /* isRow */ true, removeNumberCandidate);

    // A filled cell cannot be trivial anymore
    undo.trivialityChanges.emplace_back(ChangeTriviality(cell, std::nullopt));

    auto numberCandidatesRemovedTrivialityChanges = UpdateNumberCandidatesRemovedConstraints();
    undo.trivialityChanges.insert(
        undo.trivialityChanges.end(),
        numberCandidatesRemovedTrivialityChanges.begin(),
        numberCandidatesRemovedTrivialityChanges.end());

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
    }

    // Restore previous number candidates for cell itself.
    Constraints(cell).numberCandidates = undo.previousNumberCandidates;

    for (auto iter = undo.trivialityChanges.rbegin(); iter != undo.trivialityChanges.rend();
         ++iter) {
      const Cell& currentCell = *iter->cell;
      if (iter->previousTriviality) {
        trivialCells_[&currentCell] = *iter->previousTriviality;
      } else {
        trivialCells_.erase(&currentCell);
      }
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
    assert(numberCandidatesRemoved_.empty());

    SetSumUndoContext undo;
    undo.cell = &cell;
    undo.isRow = isRow;

    const auto& combinations =
        kCombinations.PerSizePerSum(cell.BlockSum(isRow), cell.BlockSize(isRow));
    std::array<int, 10> numberCandidateCounts{};
    std::array<const Cell*, 10> lastNumberCandidate{};
    board_.ForEachBlockCell(cell, isRow, [&](const Cell& currentCell) {
      if (currentCell.IsFilled()) {
        numberCandidateCounts[currentCell.number]++;
        lastNumberCandidate[currentCell.number] = &currentCell;
        return;
      }

      auto& currentCellConstraints = Constraints(currentCell);
      undo.numberCandidates.push_back(currentCellConstraints.numberCandidates);
      currentCellConstraints.numberCandidates.And(combinations.possibleNumbers);

      // Count how many cells in this block provide each number candidate so we can check if one
      // became trivial because of this sum set below.
      currentCellConstraints.numberCandidates.ForEachTrue(
          [&currentCell, &numberCandidateCounts, &lastNumberCandidate](int number) {
            numberCandidateCounts[number]++;
            lastNumberCandidate[number] = &currentCell;
          });

      // Check if any number candidates were removed and add to set of cells to update constraints
      // for below.
      Numbers removedNumberCandidates{undo.numberCandidates.back()};
      removedNumberCandidates.Xor(currentCellConstraints.numberCandidates);
      if (removedNumberCandidates.Count()) {
        numberCandidatesRemoved_[&currentCell].Or(removedNumberCandidates);
      }

      // Check if cell became trivial because we set the block sum.
      auto trivial = IsTrivialCell(currentCell);
      if (trivial) {
        undo.trivialityChanges.emplace_back(ChangeTriviality(currentCell, trivial));
      }
    });

    // Check if any necessary numbers for this block are now only provided once because of this sum
    // set.
    combinations.necessaryNumbers.ForEachTrue([&](int number) {
      if (numberCandidateCounts[number] == 1) {
        const Cell& currentCell = *lastNumberCandidate[number];
        if (currentCell.IsFree()) {
          // We know we need this number but there is only one candidate for it, so it must be here!
          undo.trivialityChanges.emplace_back(ChangeTriviality(currentCell, number));
        }
      }
    });

    auto numberCandidatesRemovedTrivialityChanges = UpdateNumberCandidatesRemovedConstraints();
    undo.trivialityChanges.insert(
        undo.trivialityChanges.end(),
        numberCandidatesRemovedTrivialityChanges.begin(),
        numberCandidatesRemovedTrivialityChanges.end());

    return undo;
  }

  void UndoSetSum(const SetSumUndoContext& undo) {
    auto& cell = *undo.cell;
    int i = 0;

    board_.SetBlockSum(cell, undo.isRow, 0);
    board_.ForEachBlockCell(cell, undo.isRow, [this, &i, &undo](const Cell& currentCell) {
      Constraints(currentCell).numberCandidates = undo.numberCandidates[i];
      i++;
    });

    for (auto iter = undo.trivialityChanges.rbegin(); iter != undo.trivialityChanges.rend();
         ++iter) {
      const Cell& currentCell = *iter->cell;
      if (iter->previousTriviality) {
        trivialCells_[&currentCell] = *iter->previousTriviality;
      } else {
        trivialCells_.erase(&currentCell);
      }
    }
  }

  // Returns a list of cells made trivial by the constraint updates.
  std::vector<TrivialityChange> UpdateNumberCandidatesRemovedConstraints() {
    std::vector<TrivialityChange> trivialityChanges;

    // First, gather all affected blocks and compute which numbers got removed from cells in the
    // block.
    std::unordered_map<const Cell*, Numbers> rowBlockNumberCandidatesRemoved;
    std::unordered_map<const Cell*, Numbers> columnBlockNumberCandidatesRemoved;
    for (auto entry : numberCandidatesRemoved_) {
      const auto& cell = *entry.first;
      const auto& numberCandidatesRemoved = entry.second;
      rowBlockNumberCandidatesRemoved[&board_.RowBlock(cell)].Or(numberCandidatesRemoved);
      columnBlockNumberCandidatesRemoved[&board_.ColumnBlock(cell)].Or(numberCandidatesRemoved);
    }

    // Second, for each affected block check if the removed number is now only a candidate in one
    // cell, which would make that cell trivial.
    auto updateBlockNumberCandidatesRemovedConstraints =
        [&](const std::unordered_map<const Cell*, Numbers>& blockNumberCandidatesRemoved,
            bool isRow) {
          for (auto entry : blockNumberCandidatesRemoved) {
            const auto& cell = *entry.first;
            const auto& numberCandidatesRemoved = entry.second;

            // Check if any of the removed number candidates were necessary.
            const auto& combinations =
                kCombinations.PerSizePerSum(cell.BlockSum(isRow), cell.BlockSize(isRow));
            Numbers necessaryRemovedCandidates{numberCandidatesRemoved};
            necessaryRemovedCandidates.And(combinations.necessaryNumbers);

            // Check if any of the numbers that were removed from cells in this block but are
            // necessary are now only possible for a single cell.
            necessaryRemovedCandidates.ForEachTrue([&](int number) {
              const Cell* lastCell = nullptr;
              int numCells = 0;
              board_.ForEachBlockCell(cell, isRow, [&](const Cell& currentCell) {
                if (currentCell.number == number ||
                    Constraints(currentCell).numberCandidates.Has(number)) {
                  lastCell = &currentCell;
                  numCells++;
                }
              });

              if (numCells == 1) {
                if (lastCell->IsFree()) {
                  // We know we need this number but there is only one candidate for it, so it must
                  // be here!
                  trivialityChanges.emplace_back(ChangeTriviality(*lastCell, number));
                }
              } else if (numCells == 0) {
                // This is a contradiction, we need this number but there is no available cell for
                // it.
                board_.ForEachBlockCell(cell, isRow, [&](const Cell& currentCell) {
                  if (currentCell.IsFree()) {
                    trivialityChanges.emplace_back(ChangeTriviality(currentCell, 0));
                  }
                });
              }
            });
          }
        };
    updateBlockNumberCandidatesRemovedConstraints(
        rowBlockNumberCandidatesRemoved, /* isRow */ true);
    updateBlockNumberCandidatesRemovedConstraints(
        columnBlockNumberCandidatesRemoved, /* isRow */ false);

    numberCandidatesRemoved_.clear();
    return trivialityChanges;
  }

  void Dump(std::string prefix, int index) const {
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
  void AssertValidity() const {
#ifdef NDEBUG
    return;
#endif
    Dump("validity", 0);

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

  Board& board_;
  std::vector<CellConstraints> cellConstraints_;
  std::unordered_map<const Cell*, int> trivialCells_;
  std::unordered_map<const Cell*, Numbers> numberCandidatesRemoved_;
};

} // namespace kakuro

#endif
