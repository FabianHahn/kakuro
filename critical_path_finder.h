#ifndef CRITICAL_PATH_FINDER_H
#define CRITICAL_PATH_FINDER_H

namespace kakuro {

class CriticalPathFinder {
public:
  CriticalPathFinder(const Board& board)
      : board_{board}, visited_(static_cast<std::size_t>(board.Rows() * board.Columns())) {}

  bool IsCriticalPath(const Cell& cell) {
    if (cell.isBlock) {
      return false;
    }

    const Cell& topCell = board_(cell.row - 1, cell.column);
    if (!topCell.isBlock) {
      ClearMarked();
      Mark(cell);
      int numReachableTop = CountReachableCells(topCell);
      if (numReachableTop != board_.Numbers() - 1) {
        return true;
      }
    }

    const Cell& leftCell = board_(cell.row, cell.column - 1);
    if (!leftCell.isBlock) {
      ClearMarked();
      Mark(cell);
      int numReachableLeft = CountReachableCells(leftCell);
      if (numReachableLeft != board_.Numbers() - 1) {
        return true;
      }
    }

    if (cell.row + 1 < board_.Rows()) {
      const Cell& bottomCell = board_(cell.row + 1, cell.column);
      if (!bottomCell.isBlock) {
        ClearMarked();
        Mark(cell);
        int numReachableBottom = CountReachableCells(bottomCell);
        if (numReachableBottom != board_.Numbers() - 1) {
          return true;
        }
      }
    }

    if (cell.column + 1 < board_.Columns()) {
      const Cell& rightCell = board_(cell.row, cell.column + 1);
      if (!rightCell.isBlock) {
        ClearMarked();
        Mark(cell);
        int numReachableRight = CountReachableCells(rightCell);
        if (numReachableRight != board_.Numbers() - 1) {
          return true;
        }
      }
    }

    return false;
  }

private:
  void ClearMarked() {
    visited_.clear();
    visited_.resize(board_.Rows() * board_.Columns(), false);
  }

  void Mark(const Cell& cell) { visited_[cell.row * board_.Columns() + cell.column] = true; }

  bool IsMarked(const Cell& cell) const {
    return visited_[cell.row * board_.Columns() + cell.column];
  }

  int CountReachableCells(const Cell& cell) {
    if (cell.isBlock || IsMarked(cell)) {
      return 0;
    }

    int numReachableUnmarked = 1;
    Mark(cell);

    numReachableUnmarked += CountReachableCells(board_(cell.row - 1, cell.column));
    numReachableUnmarked += CountReachableCells(board_(cell.row, cell.column - 1));

    if (cell.row + 1 < board_.Rows()) {
      numReachableUnmarked += CountReachableCells(board_(cell.row + 1, cell.column));
    }
    if (cell.column + 1 < board_.Columns()) {
      numReachableUnmarked += CountReachableCells(board_(cell.row, cell.column + 1));
    }

    return numReachableUnmarked;
  }

  const Board& board_;
  std::vector<bool> visited_;
};

} // namespace kakuro

#endif