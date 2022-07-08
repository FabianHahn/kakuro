#include "solver.h"

#include <gtest/gtest.h>

#include "board.h"
#include "board_generator.h"
#include <fstream>

using namespace kakuro;

class SolverTest : public ::testing::TestWithParam<bool> {};

TEST_P(SolverTest, SolveEmpty) {
  Board board{3, 4};
  Solver solver{GetParam()};
  bool result = solver.Solve(board);
  ASSERT_TRUE(result);

  for (int row = 1; row <= 2; row++) {
    for (int col = 1; col <= 3; col++) {
      ASSERT_NE(board(row, col).number, 0) << "field must be solved";
    }
  }

  // Make sure rows have different numbers for each column
  for (int row = 1; row <= 2; row++) {
    ASSERT_NE(board(row, 1).number, board(row, 2).number);
    ASSERT_NE(board(row, 1).number, board(row, 3).number);
  }

  // Make sure cols have different numbers for each row
  for (int col = 1; col <= 3; col++) {
    ASSERT_NE(board(1, col).number, board(2, col).number);
  }
}

TEST_P(SolverTest, SolveConstrained) {
  Board board{3, 4};
  board.SetRowBlockSum(board(1, 0), 17);
  Solver solver{GetParam()};
  bool result = solver.Solve(board);
  ASSERT_TRUE(result);

  ASSERT_EQ(board(1, 1).number + board(1, 2).number + board(1, 3).number, 17);
}

TEST_P(SolverTest, SolveUnique) {
  Board board{3, 4};
  board.SetRowBlockSum(board(1, 0), 7);
  board.SetRowBlockSum(board(2, 0), 24);
  board.SetColumnBlockSum(board(0, 1), 10);
  board.SetColumnBlockSum(board(0, 2), 13);
  board.SetColumnBlockSum(board(0, 3), 8);
  Solver solver{GetParam()};
  bool result = solver.Solve(board);
  ASSERT_TRUE(result);

  ASSERT_EQ(board(1, 1).number, 2);
  ASSERT_EQ(board(1, 2).number, 4);
  ASSERT_EQ(board(1, 3).number, 1);
  ASSERT_EQ(board(2, 1).number, 8);
  ASSERT_EQ(board(2, 2).number, 9);
  ASSERT_EQ(board(2, 3).number, 7);
}

TEST_P(SolverTest, SolveImpossible) {
  Board board{3, 4};
  board.SetRowBlockSum(board(1, 0), 45);
  Solver solver{GetParam()};
  bool result = solver.Solve(board);

  ASSERT_FALSE(result);
}

TEST_P(SolverTest, SolveStar) {
  Board board{4, 4};
  board.MakeBlock(board(1, 1));
  board.MakeBlock(board(1, 3));
  board.MakeBlock(board(3, 1));
  board.MakeBlock(board(3, 3));
  board.SetRowBlockSum(board(1, 1), 1);
  board.SetColumnBlockSum(board(1, 3), 1);

  Solver solver{GetParam()};
  bool result = solver.Solve(board);

  ASSERT_TRUE(result);
}

TEST_P(SolverTest, SolveGenerated) {
  std::random_device randomDevice;
  std::mt19937 random;
  random.seed(3);

  BoardGenerator boardGenerator{random, /* blockProbability */ 0.3};
  auto board = boardGenerator.Generate(/* rows */ 10, /* columns */ 20);

  Solver solver{GetParam()};
  bool result = solver.Solve(board);

  ASSERT_TRUE(result);
}

INSTANTIATE_TEST_SUITE_P(WithWithoutTrivial, SolverTest, testing::Values(false, true));
