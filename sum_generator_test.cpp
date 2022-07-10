#include "sum_generator.h"

#include <gtest/gtest.h>

#include "board.h"
#include <fstream>

using namespace kakuro;

TEST(SumGeneratorTest, GenerateStar) {
  Board board{4, 4};
  board.MakeBlock(board(1, 1));
  board.MakeBlock(board(1, 3));
  board.MakeBlock(board(3, 1));
  board.MakeBlock(board(3, 3));
  ConstrainedBoard constrainedBoard{board};

  SumGenerator sumGenerator;
  bool result = sumGenerator.GenerateSums(constrainedBoard);
  ASSERT_TRUE(result);
}
