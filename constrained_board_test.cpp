#include "constrained_board.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fstream>

using namespace kakuro;
using testing::Contains;
using testing::Not;
using testing::UnorderedElementsAre;

// Test board:
//   *********
//   ********8
//   *12345679
//
// We first assert that the 8 is trivial if we provide a row block sum, then that 1 is trivial if we
// provide a column block sum. Finally, we fill in numbers 1-7 and assert that 9 must be trivial
// too.
TEST(ConstrainedBoardTest, Trivial) {
  Board board{3, 9};

  for (int column = 1; column <= 7; column++) {
    board.MakeBlock(board(1, column));
  }

  ConstrainedBoard constrainedBoard{board};
  SetSumUndoContext sumUndo;
  constrainedBoard.SetRowBlockSum(board(1, 7), 8, sumUndo);
  constrainedBoard.SetColumnBlockSum(board(1, 1), 1, sumUndo);

  ASSERT_THAT(
      constrainedBoard.TrivialCells(),
      UnorderedElementsAre(std::make_pair(&board(1, 8), 8), std::make_pair(&board(2, 1), 1)));

  FillNumberUndoContext undo;
  ASSERT_EQ(constrainedBoard.IsTrivialCell(board(1, 8)), 8)
      << "cell should be trivial because it is the only cell in its row block";
  constrainedBoard.FillNumber(board(1, 8), 8, undo);

  ASSERT_EQ(constrainedBoard.IsTrivialCell(board(2, 1)), 1)
      << "cell should be trivial because it is the only cell in its column block";
  ASSERT_FALSE(constrainedBoard.IsTrivialCell(board(2, 8))) << "cell should not be trivial yet";

  for (int column = 1; column <= 7; column++) {
    constrainedBoard.FillNumber(board(2, column), column, undo);
  }

  ASSERT_EQ(constrainedBoard.IsTrivialCell(board(2, 8)), 9) << "cell should now be trivial";
  ASSERT_THAT(constrainedBoard.TrivialCells(), UnorderedElementsAre(std::make_pair(&board(2, 8), 9)));
}

TEST(ConstrainedBoardTest, InvalidTrivial) {
  Board board{2, 4};

  ConstrainedBoard constrainedBoard{board};
  FillNumberUndoContext undo;
  constrainedBoard.FillNumber(board(1, 2), 1, undo);
  constrainedBoard.FillNumber(board(1, 3), 4, undo);
  SetSumUndoContext sumUndo;
  constrainedBoard.SetRowBlockSum(board(1, 0), 6, sumUndo);

  ASSERT_EQ(constrainedBoard.IsTrivialCell(board(1, 1)), 1);
  ASSERT_THAT(
      constrainedBoard.TrivialCells(),
      UnorderedElementsAre(std::make_pair(&board(1, 1), 1)));
}
