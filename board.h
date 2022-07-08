#ifndef BOARD_H
#define BOARD_H

#include "numbers.h"
#include <cassert>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace kakuro {

struct Cell {
  int row;
  int column;
  int number;
  Numbers numberCandidates;
  bool isBlock;
  int rowBlockRow;
  int rowBlockColumn;
  int rowBlockSize;
  int rowBlockFree;
  int rowBlockSum;
  Numbers rowBlockNumbers;
  int columnBlockRow;
  int columnBlockColumn;
  int columnBlockSize;
  int columnBlockFree;
  int columnBlockSum;
  Numbers columnBlockNumbers;

  int RowBlockDistance() const {
    if (isBlock) {
      return 0;
    }
    return column - rowBlockColumn;
  }

  int ColumnBlockDistance() const {
    if (isBlock) {
      return 0;
    }
    return row - columnBlockRow;
  }

  bool IsFree() const { return !isBlock && number == 0; }
};

class Board {
public:
  Board(int rows, int columns)
      : rows_{rows},
        columns_{columns},
        numbers_{(rows - 1) * (columns - 1)},
        cells_{static_cast<std::size_t>(rows * columns)} {
    for (int row = 0; row < rows_; row++) {
      for (int column = 0; column < columns_; column++) {
        auto& cell = (*this)(row, column);
        cell.row = row;
        cell.column = column;
        cell.number = 0;
        cell.numberCandidates.Fill();
        cell.isBlock = false;

        if (column == 0) {
          cell.isBlock = true;
          cell.rowBlockRow = 0;
          cell.rowBlockColumn = 0;
          cell.rowBlockSize = columns_ - 1;
          cell.rowBlockFree = columns_ - 1;
        } else {
          cell.rowBlockRow = row;
          cell.rowBlockColumn = 0;
          cell.rowBlockSize = 0;
          cell.rowBlockFree = 0;
        }

        if (row == 0) {
          cell.isBlock = true;
          cell.columnBlockRow = 0;
          cell.columnBlockColumn = 0;
          cell.columnBlockSize = rows_ - 1;
          cell.columnBlockFree = rows_ - 1;
        } else {
          cell.columnBlockRow = 0;
          cell.columnBlockColumn = column;
          cell.columnBlockSize = 0;
          cell.columnBlockFree = 0;
        }

        cell.rowBlockSum = 0;
        cell.columnBlockSum = 0;
      }
    }
  }

  int Rows() const { return rows_; }

  int Columns() const { return columns_; }

  int Numbers() const { return numbers_; }

  Cell& operator()(int row, int column) {
    assert(row >= 0);
    assert(row < rows_);
    assert(column >= 0);
    assert(column < columns_);
    return cells_[row * columns_ + column];
  }

  Cell& operator[](int index) {
    assert(index >= 0);
    assert(index < rows_ * columns_);
    return cells_[index];
  }

  Cell& RowBlock(Cell& cell) {
    if (cell.isBlock) {
      return cell;
    }

    return (*this)(cell.rowBlockRow, cell.rowBlockColumn);
  }

  Cell& ColumnBlock(Cell& cell) {
    if (cell.isBlock) {
      return cell;
    }

    return (*this)(cell.columnBlockRow, cell.columnBlockColumn);
  }

  std::optional<int> IsTrivialCell(Cell& cell) {
    assert(!cell.isBlock);

    if (!cell.IsFree()) {
      return std::nullopt;
    }

    // Check if cell is trivial because neighbor constraints say there is only one possible number.
    if (cell.numberCandidates.Count() == 1) {
      return cell.numberCandidates.Sum();
    }

    // Check if cell is trivial because it is the only free number left in its row block.
    Cell& rowBlock = RowBlock(cell);
    if (rowBlock.rowBlockSum > 0 && rowBlock.rowBlockFree == 1) {
      return rowBlock.rowBlockSum - rowBlock.rowBlockNumbers.Sum();
    }

    // Check if cell is trivial because it is the only free number left in its column block.
    Cell& columnBlock = ColumnBlock(cell);
    if (columnBlock.columnBlockSum > 0 && columnBlock.columnBlockFree == 1) {
      return columnBlock.columnBlockSum - columnBlock.columnBlockNumbers.Sum();
    }

    return std::nullopt;
  }

  void ForEachRowBlockCell(Cell& cell, const std::function<void(Cell&)>& callback) {
    assert(cell.isBlock);

    for (int column = cell.column + 1; column < columns_; column++) {
      Cell& currentCell = (*this)(cell.row, column);

      if (currentCell.isBlock) {
        break;
      }

      callback(currentCell);
    }
  }

  void ForEachColumnBlockCell(Cell& cell, const std::function<void(Cell&)>& callback) {
    assert(cell.isBlock);

    for (int row = cell.row + 1; row < rows_; row++) {
      Cell& currentCell = (*this)(row, cell.column);

      if (currentCell.isBlock) {
        break;
      }

      callback(currentCell);
    }
  }

  int ForEachNeighborCell(Cell& cell, const std::function<bool(Cell&)>& callback) {
    bool isLeftBorder = cell.column == 0;
    bool isTopBorder = cell.row == 0;
    bool isRightBorder = cell.column == Columns() - 1;
    bool isBottomBorder = cell.row == Rows() - 1;

    if (!isLeftBorder) {
      Cell& leftCell = (*this)(cell.row, cell.column - 1);
      if (!leftCell.isBlock) {
        if (!callback(leftCell)) {
          return false;
        }
      }
    }

    if (!isTopBorder) {
      Cell& topCell = (*this)(cell.row - 1, cell.column);
      if (!topCell.isBlock) {
        if (!callback(topCell)) {
          return false;
        }
      }
    }

    if (!isRightBorder) {
      Cell& rightCell = (*this)(cell.row, cell.column + 1);
      if (!rightCell.isBlock) {
        if (!callback(rightCell)) {
          return false;
        }
      }
    }

    if (!isBottomBorder) {
      Cell& bottomCell = (*this)(cell.row + 1, cell.column);
      if (!bottomCell.isBlock) {
        if (!callback(bottomCell)) {
          return false;
        }
      }
    }

    return true;
  }

  std::unordered_set<Cell*> FindFreeCells() {
    std::unordered_set<Cell*> freeCells;

    for (int row = 0; row < Rows(); row++) {
      for (int column = 0; column < Columns(); column++) {
        Cell& cell = (*this)(row, column);
        if (cell.IsFree()) {
          freeCells.insert(&cell);
        }
      }
    }

    return freeCells;
  }

  // Finds a subboard around a given nonblock cell in BFS order.
  std::vector<Cell*> FindSubboard(Cell& cell) {
    assert(!cell.isBlock);

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

        ForEachNeighborCell(newCell, [&subboard, &visitedCells, &newCells](Cell& currentCell) {
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

  std::unordered_set<Cell*> FindSubboardBlocks(const std::vector<Cell*>& subboard) {
    std::unordered_set<Cell*> blocks;
    for (auto* cellPointer : subboard) {
      auto& cell = *cellPointer;
      blocks.insert(&RowBlock(cell));
      blocks.insert(&ColumnBlock(cell));
    }
    return blocks;
  }

  void MakeBlock(Cell& cell) {
    assert(cell.IsFree());

    auto& oldRowBlock = RowBlock(cell);
    auto& oldColumnBlock = ColumnBlock(cell);
    oldRowBlock.rowBlockSize = cell.RowBlockDistance() - 1;
    oldRowBlock.rowBlockFree--; // subtract one for this cell, more below
    oldColumnBlock.columnBlockSize = cell.ColumnBlockDistance() - 1;
    oldColumnBlock.columnBlockFree--; // subtract one for this cell, more below
    oldRowBlock.rowBlockSum -= cell.number;
    oldColumnBlock.columnBlockSum -= cell.number;

    numbers_--;
    cell.number = 0;
    cell.isBlock = true;
    cell.columnBlockSize = 0;
    cell.columnBlockFree = 0;
    cell.columnBlockSum = 0;
    ForEachColumnBlockCell(cell, [&cell, &oldColumnBlock](Cell& currentCell) {
      currentCell.columnBlockRow = cell.row;
      currentCell.columnBlockColumn = cell.column;
      cell.columnBlockSize++;
      oldColumnBlock.columnBlockSum -= cell.number;
      cell.columnBlockSum += cell.number;

      if (currentCell.IsFree()) {
        // remove a free cell from old column block, and add one to this
        oldColumnBlock.columnBlockFree--;
        cell.columnBlockFree++;
      }
    });

    cell.rowBlockSize = 0;
    cell.rowBlockFree = 0;
    cell.rowBlockSum = 0;
    ForEachRowBlockCell(cell, [&cell, &oldRowBlock](Cell& currentCell) {
      currentCell.rowBlockRow = cell.row;
      currentCell.rowBlockColumn = cell.column;
      cell.rowBlockSize++;
      oldRowBlock.rowBlockSum -= cell.number;
      cell.rowBlockSum += cell.number;

      if (currentCell.IsFree()) {
        // remove a free cell from old column block, and add one to this
        oldRowBlock.rowBlockFree--;
        cell.rowBlockFree++;
      }
    });
  }

  struct FillNumberUndoContext {
    int row;
    int column;
    std::vector<std::pair<int, int>> candidatesRemoved;
  };

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

    Cell& rowBlock = RowBlock(cell);
    Cell& columnBlock = ColumnBlock(cell);

    bool canBeNumber = cell.numberCandidates.Has(number);
    bool rowBlockHasNumber = rowBlock.rowBlockNumbers.Has(number);
    bool columnBlockHasNumber = columnBlock.columnBlockNumbers.Has(number);

    if (!canBeNumber || rowBlockHasNumber || columnBlockHasNumber) {
      return false;
    }

    if (rowBlock.rowBlockSum > 0) {
      bool isLastRowBlockNumber = rowBlock.rowBlockNumbers.Count() + 1 == rowBlock.rowBlockSize;
      if (isLastRowBlockNumber && rowBlock.rowBlockNumbers.Sum() + number != rowBlock.rowBlockSum) {
        return false;
      }
    }

    if (columnBlock.columnBlockSum > 0) {
      bool isLastColumnBlockNumber =
          columnBlock.columnBlockNumbers.Count() + 1 == columnBlock.columnBlockSize;
      if (isLastColumnBlockNumber &&
          columnBlock.columnBlockNumbers.Sum() + number != columnBlock.columnBlockSum) {
        return false;
      }
    }

    cell.number = number;
    rowBlock.rowBlockNumbers.Add(number);
    rowBlock.rowBlockFree--;
    columnBlock.columnBlockNumbers.Add(number);
    columnBlock.columnBlockFree--;

    auto removeNumberCandidate = [this, number, &undoContext](Cell& currentCell) {
      if (currentCell.numberCandidates.Has(number)) {
        undoContext.candidatesRemoved.emplace_back(currentCell.row, currentCell.column);
        currentCell.numberCandidates.Remove(number);
      }

      auto trivial = IsTrivialCell(currentCell);
      if (trivial) {
        trivialCells_[&currentCell] = *trivial;
      }
    };
    ForEachColumnBlockCell(columnBlock, removeNumberCandidate);
    ForEachRowBlockCell(rowBlock, removeNumberCandidate);

    // A filled cell cannot be trivial anymore
    trivialCells_.erase(&cell);

    return true;
  }

  void UndoFillNumber(const FillNumberUndoContext& undoContext) {
    Cell& cell = (*this)(undoContext.row, undoContext.column);

    Cell& rowBlock = RowBlock(cell);
    Cell& columnBlock = ColumnBlock(cell);
    rowBlock.rowBlockNumbers.Remove(cell.number);
    rowBlock.rowBlockFree++;
    columnBlock.columnBlockNumbers.Remove(cell.number);
    columnBlock.columnBlockFree++;

    for (auto& currentCellCoordinates : undoContext.candidatesRemoved) {
      Cell& currentCell = (*this)(currentCellCoordinates.first, currentCellCoordinates.second);
      currentCell.numberCandidates.Add(cell.number);

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
  void SetRowBlockSum(Cell& cell, int sum) {
    assert(cell.isBlock);
    assert(sum >= 0);
    assert(sum < 46);

    cell.rowBlockSum = sum;

    ForEachRowBlockCell(cell, [this](Cell& currentCell) {
      auto trivial = IsTrivialCell(currentCell);
      if (trivial) {
        trivialCells_[&currentCell] = *trivial;
      } else {
        trivialCells_.erase(&currentCell);
      }
    });
  }

  // Doesn't currently check if the sum is at all possible for this block in terms of combinations.
  void SetColumnBlockSum(Cell& cell, int sum) {
    assert(cell.isBlock);
    assert(sum >= 0);
    assert(sum < 46);

    cell.columnBlockSum = sum;

    ForEachColumnBlockCell(cell, [this](Cell& currentCell) {
      auto trivial = IsTrivialCell(currentCell);
      if (trivial) {
        trivialCells_[&currentCell] = *trivial;
      } else {
        trivialCells_.erase(&currentCell);
      }
    });
  }

  void RenderHtml(std::ostream& output) {
    output << "<!doctype html>" << std::endl;
    output << "<html>" << std::endl;
    output << "<head>" << std::endl;
    output << "<meta charset = \"utf-8\">" << std::endl;
    output << "<title>Karuko</title>" << std::endl;
    output << "<style type=\"text/css\">" << std::endl;
    output << "table { border-collapse: collapse }" << std::endl;
    output << "td { text-align: center; vertical-align: middle; color: white }" << std::endl;
    output << "td.cell { width: 48px; height: 48px; border: 1px solid black }" << std::endl;
    output << "</style>" << std::endl;
    output << "</head>" << std::endl;
    output << "<body>" << std::endl;
    output << "<table>" << std::endl;

    for (int row = 0; row < rows_; row++) {
      output << "\t<tr>" << std::endl;
      for (int column = 0; column < columns_; column++) {
        const auto& cell = (*this)(row, column);

        if (cell.isBlock) {
          auto ifNonZero = [](int i) -> std::string {
            if (i > 0) {
              return std::to_string(i);
            } else {
              return "&nbsp;";
            }
          };

          output << "\t\t<td class=\"cell\" style=\"background-color: black\">" << std::endl;
          output << "\t\t\t<table style=\"width: 100%; height: 100%;\">" << std::endl;
          output << "\t\t\t\t<tr>" << std::endl;
          output << "\t\t\t\t\t<td></td>" << std::endl;
          output << "\t\t\t\t\t<td style=\"text-align:right;\">" << ifNonZero(cell.rowBlockSum)
                 << "</td>" << std::endl;
          output << "\t\t\t\t</tr>" << std::endl;
          output << "\t\t\t\t<tr>" << std::endl;
          output << "\t\t\t\t\t<td style=\"text-align:left;\">" << ifNonZero(cell.columnBlockSum)
                 << "</td>" << std::endl;
          output << "\t\t\t\t\t<td></td>" << std::endl;
          output << "\t\t\t\t</tr>" << std::endl;
          output << "\t\t\t</table>" << std::endl;
        } else {
          output << "\t\t<td class=\"cell\">" << std::endl;
          output << "\t\t\t" << cell.number << std::endl;
        }

        output << "\t\t</td>" << std::endl;
      }
      output << "\t</tr>" << std::endl;
    }

    output << "</table>" << std::endl;
    output << "</body>" << std::endl;
    output << "</html>" << std::endl;
  }

  const std::unordered_map<Cell*, int>& TrivialCells() const { return trivialCells_; }

private:
  int rows_;
  int columns_;
  int numbers_;
  std::vector<Cell> cells_;
  std::unordered_map<Cell*, int> trivialCells_;
};

} // namespace kakuro

#endif
