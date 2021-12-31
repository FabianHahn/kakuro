#ifndef BOARD_H
#define BOARD_H

#include <cassert>
#include <functional>
#include <vector>

namespace kakuro {

struct Cell {
  int row;
  int column;
  int number;
  bool isBlock;
  int rowBlockRow;
  int rowBlockColumn;
  int rowBlockSize;
  int rowBlockSum;
  int columnBlockRow;
  int columnBlockColumn;
  int columnBlockSize;
  int columnBlockSum;

  int RowBlockDistance() {
    if (isBlock) {
      return 0;
    }
    return column - rowBlockColumn;
  }

  int ColumnBlockDistance() {
    if (isBlock) {
      return 0;
    }
    return row - columnBlockRow;
  }
};

class Board {
public:
  Board(int rows, int columns)
      : rows_{rows}
      , columns_{columns}
      , numbers_{(rows - 1) * (columns - 1)}
      , cells_{static_cast<std::size_t>(rows * columns)} {
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
        } else {
          cell.rowBlockRow = row;
          cell.rowBlockColumn = 0;
          cell.rowBlockSize = 0;
        }

        if (row == 0) {
          cell.isBlock = true;
          cell.columnBlockRow = 0;
          cell.columnBlockColumn = 0;
          cell.columnBlockSize = rows_ - 1;
        } else {
          cell.columnBlockRow = 0;
          cell.columnBlockColumn = column;
          cell.columnBlockSize = 0;
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

  void MakeBlock(Cell& cell) {
    if (cell.isBlock) {
      return;
    }

    auto& oldRowBlock = RowBlock(cell);
    auto& oldColumnBlock = ColumnBlock(cell);
    oldRowBlock.rowBlockSize = cell.RowBlockDistance() - 1;
    oldColumnBlock.columnBlockSize = cell.ColumnBlockDistance() - 1;
    oldRowBlock.rowBlockSum -= cell.number;
    oldColumnBlock.columnBlockSum -= cell.number;

    numbers_--;
    cell.number = 0;
    cell.isBlock = true;
    cell.columnBlockSize = 0;
    cell.columnBlockSum = 0;
    ForEachColumnBlockCell(
        cell, [&cell, &oldColumnBlock](Cell& currentCell) {
          currentCell.columnBlockRow = cell.row;
          currentCell.columnBlockColumn = cell.column;
          cell.columnBlockSize++;
          oldColumnBlock.columnBlockSum -= cell.number;
          cell.columnBlockSum += cell.number;
        });

    cell.rowBlockSize = 0;
    cell.rowBlockSum = 0;
    ForEachRowBlockCell(
        cell, [&cell, &oldRowBlock](Cell& currentCell) {
          currentCell.rowBlockRow = cell.row;
          currentCell.rowBlockColumn = cell.column;
          cell.rowBlockSize++;
          oldRowBlock.rowBlockSum -= cell.number;
          cell.rowBlockSum += cell.number;
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
          output << "\t\t\t\t\t<td style=\"text-align:right;\">" << ifNonZero(cell.rowBlockSum) << "</td>"
              << std::endl;
          output << "\t\t\t\t</tr>" << std::endl;
          output << "\t\t\t\t<tr>" << std::endl;
          output << "\t\t\t\t\t<td style=\"text-align:left;\">" << ifNonZero(cell.columnBlockSum) << "</td>"
              << std::endl;
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

private:
  int rows_;
  int columns_;
  int numbers_;
  std::vector<Cell> cells_;
};

}

#endif