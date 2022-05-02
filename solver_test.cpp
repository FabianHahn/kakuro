#include "solver.h"

#include <gtest/gtest.h>

#include "board.h"
#include <fstream>

using namespace kakuro;

TEST(SolverTest, SolveEmpty) {
  Board board{3, 4};
  Solver solver;
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

TEST(SolverTest, SolveConstrained) {
  Board board{3, 4};
  board(1, 0).rowBlockSum = 17;
  Solver solver;
  bool result = solver.Solve(board);
  ASSERT_TRUE(result);

  ASSERT_EQ(board(1, 1).number + board(1, 2).number + board(1, 3).number, 17);
}

TEST(SolverTest, SolveUnique) {
  Board board{3, 4};
  board(1, 0).rowBlockSum = 7;
  board(2, 0).rowBlockSum = 24;
  board(0, 1).columnBlockSum = 10;
  board(0, 2).columnBlockSum = 13;
  board(0, 3).columnBlockSum = 8;
  Solver solver;
  bool result = solver.Solve(board);
  ASSERT_TRUE(result);

  ASSERT_EQ(board(1, 1).number, 2);
  ASSERT_EQ(board(1, 2).number, 4);
  ASSERT_EQ(board(1, 3).number, 1);
  ASSERT_EQ(board(2, 1).number, 8);
  ASSERT_EQ(board(2, 2).number, 9);
  ASSERT_EQ(board(2, 3).number, 7);
}

TEST(SolverTest, SolveImpossible) {
  Board board{3, 4};
  board(1, 0).rowBlockSum = 45;
  Solver solver;
  bool result = solver.Solve(board);

  ASSERT_FALSE(result);
}
