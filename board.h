#ifndef BOARD_H
#define BOARD_H

#include "combinations.h"
#include <cassert>
#include <fstream>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace kakuro {

namespace {
const Combinations kCombinations;
}

struct Cell {
  int row;
  int column;
  int number;
  bool isBlock;
  int rowBlockRow;
  int rowBlockColumn;
  int rowBlockSize;
  int rowBlockFree;
  int rowBlockSum;
  int columnBlockRow;
  int columnBlockColumn;
  int columnBlockSize;
  int columnBlockFree;
  int columnBlockSum;

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

  void RenderHtml(std::ostream& output) {
    return RenderHtml(output, [](std::ostream& output, Cell& cell) { output << cell.number; });
  }

  void RenderHtml(
      std::ostream& output, const std::function<void(std::ostream&, Cell&)>& cellPrinter) {
    output << "<!doctype html>" << std::endl;
    output << "<html>" << std::endl;
    output << "<head>" << std::endl;
    output << "<meta charset = \"utf-8\">" << std::endl;
    output << "<title>Karuko</title>" << std::endl;
    output << "<style type=\"text/css\">" << std::endl;
    output << "table { border-collapse: collapse }" << std::endl;
    output << "td { text-align: center; vertical-align: middle; color: black }" << std::endl;
    output << "td.cell { width: 48px; height: 48px; border: 1px solid black }" << std::endl;
    output << "</style>" << std::endl;
    output << "</head>" << std::endl;
    output << "<body>" << std::endl;
    output << "<table>" << std::endl;

    for (int row = 0; row < rows_; row++) {
      output << "\t<tr>" << std::endl;
      for (int column = 0; column < columns_; column++) {
        auto& cell = (*this)(row, column);

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
          output << "\t\t\t\t\t<td style=\"text-align:right;color:white\">"
                 << ifNonZero(cell.rowBlockSum) << "</td>" << std::endl;
          output << "\t\t\t\t</tr>" << std::endl;
          output << "\t\t\t\t<tr>" << std::endl;
          output << "\t\t\t\t\t<td style=\"text-align:left;color:white\">"
                 << ifNonZero(cell.columnBlockSum) << "</td>" << std::endl;
          output << "\t\t\t\t\t<td></td>" << std::endl;
          output << "\t\t\t\t</tr>" << std::endl;
          output << "\t\t\t</table>" << std::endl;
        } else {
          output << "\t\t<td class=\"cell\">" << std::endl;
          output << "\t\t\t";
          cellPrinter(output, cell);
          output << std::endl;
        }

        output << "\t\t</td>" << std::endl;
      }
      output << "\t</tr>" << std::endl;
    }

    output << "</table>" << std::endl;
    output << "</body>" << std::endl;
    output << "</html>" << std::endl;
  }

private:
  int rows_;
  int columns_;
  int numbers_;
  std::vector<Cell> cells_;
};

} // namespace kakuro

#endif
