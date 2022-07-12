#include "constrained_board.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fstream>

using namespace kakuro;
using testing::Contains;
using testing::IsEmpty;
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
  constrainedBoard.SetBlockSum(board(1, 7), /* isRow */ true, 8, sumUndo);
  constrainedBoard.SetBlockSum(board(1, 1), /* isRow */ false, 1, sumUndo);
  constrainedBoard.AssertValidity();

  ASSERT_THAT(
      constrainedBoard.TrivialCells(),
      UnorderedElementsAre(std::make_pair(&board(1, 8), 8), std::make_pair(&board(2, 1), 1)));

  FillNumberUndoContext undo;
  ASSERT_EQ(constrainedBoard.IsTrivialCell(board(1, 8)), 8)
      << "cell should be trivial because it is the only cell in its row block";
  constrainedBoard.FillNumber(board(1, 8), 8, undo);
  constrainedBoard.AssertValidity();

  ASSERT_EQ(constrainedBoard.IsTrivialCell(board(2, 1)), 1)
      << "cell should be trivial because it is the only cell in its column block";
  ASSERT_FALSE(constrainedBoard.IsTrivialCell(board(2, 8))) << "cell should not be trivial yet";

  for (int column = 1; column <= 7; column++) {
    constrainedBoard.FillNumber(board(2, column), column, undo);
  }
  constrainedBoard.AssertValidity();

  ASSERT_EQ(constrainedBoard.IsTrivialCell(board(2, 8)), 9) << "cell should now be trivial";
  ASSERT_THAT(
      constrainedBoard.TrivialCells(), UnorderedElementsAre(std::make_pair(&board(2, 8), 9)));
}

TEST(ConstrainedBoardTest, InvalidTrivial) {
  Board board{2, 4};

  ConstrainedBoard constrainedBoard{board};
  FillNumberUndoContext undo;
  constrainedBoard.FillNumber(board(1, 2), 1, undo);
  constrainedBoard.FillNumber(board(1, 3), 4, undo);
  SetSumUndoContext sumUndo;
  constrainedBoard.SetBlockSum(board(1, 0), /* isRow */ true, 6, sumUndo);
  constrainedBoard.AssertValidity();

  ASSERT_EQ(constrainedBoard.IsTrivialCell(board(1, 1)), 1);
  ASSERT_THAT(
      constrainedBoard.TrivialCells(), UnorderedElementsAre(std::make_pair(&board(1, 1), 1)));
}

TEST(ConstrainedBoardTest, TrivialAmbigous) {
  Board board{5, 4};
  board.MakeBlock(board(1, 1));
  ConstrainedBoard constrainedBoard{board};
  SetSumUndoContext sumUndo;
  constrainedBoard.SetBlockSum(board(0, 3), /* isRow */ false, 10, sumUndo);
  constrainedBoard.SetBlockSum(board(0, 2), /* isRow */ false, 10, sumUndo);
  constrainedBoard.SetBlockSum(board(4, 0), /* isRow */ true, 6, sumUndo);
  constrainedBoard.SetBlockSum(board(1, 1), /* isRow */ true, 3, sumUndo);
  constrainedBoard.SetBlockSum(board(1, 1), /* isRow */ false, 6, sumUndo);
  constrainedBoard.AssertValidity();

  ASSERT_THAT(constrainedBoard.TrivialCells(), IsEmpty());
}

TEST(ConstrainedBoardTest, TrivialNecessary) {
  Board board{5, 6};
  board.MakeBlock(board(3, 2));
  board.MakeBlock(board(4, 2));
  board.MakeBlock(board(1, 4));
  board.MakeBlock(board(2, 4));
  board.MakeBlock(board(1, 5));
  board.MakeBlock(board(2, 5));
  ConstrainedBoard constrainedBoard{board};
  SetSumUndoContext sumUndo;
  constrainedBoard.SetBlockSum(board(0, 1), /* isRow */ false, 10, sumUndo);
  constrainedBoard.SetBlockSum(board(0, 2), /* isRow */ false, 3, sumUndo);
  constrainedBoard.SetBlockSum(board(1, 0), /* isRow */ true, 6, sumUndo);
  constrainedBoard.SetBlockSum(board(2, 0), /* isRow */ true, 6, sumUndo);
  constrainedBoard.SetBlockSum(board(4, 2), /* isRow */ true, 6, sumUndo);
  constrainedBoard.SetBlockSum(board(0, 3), /* isRow */ false, 10, sumUndo);

  ASSERT_EQ(constrainedBoard.IsTrivialCell(board(3, 3)), std::nullopt);
  ASSERT_THAT(
      constrainedBoard.TrivialCells(), UnorderedElementsAre(std::make_pair(&board(3, 3), 4)));
}
