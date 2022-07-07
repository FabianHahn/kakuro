#include "board.h"

#include <gtest/gtest.h>

#include <fstream>

using namespace kakuro;

// Test board:
//   *********
//   ********8
//   *12345679
//
// We first assert that the 8 is trivial if we provide a row block sum, then that 1 is trivial if we
// provide a column block sum. Finally, we fill in numbers 1-7 and assert that 9 must be trivial
// too.
TEST(BoardTest, Trivial) {
  Board board{3, 9};

  for (int column = 1; column <= 7; column++) {
    board.MakeBlock(board(1, column));
  }

  board(1, 7).rowBlockSum = 8;
  board(1, 1).columnBlockSum = 1;

  Board::FillNumberUndoContext undo;
  ASSERT_EQ(board.IsTrivialCell(board(1, 8)), 8)
      << "cell should be trivial because it is the only cell in its row block";
  board.FillNumber(board(1, 8), 8, undo);

  ASSERT_EQ(board.IsTrivialCell(board(2, 1)), 1)
      << "cell should be trivial because it is the only cell in its column block";
  ASSERT_EQ(board.IsTrivialCell(board(2, 8)), 0) << "cell should not be trivial yet";

  for (int column = 1; column <= 7; column++) {
    board.FillNumber(board(2, column), column, undo);
  }

  ASSERT_EQ(board.IsTrivialCell(board(2, 8)), 9) << "cell should now be trivial";
}
