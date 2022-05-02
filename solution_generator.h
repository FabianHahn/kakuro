#ifndef SOLUTION_GENERATOR_H
#define SOLUTION_GENERATOR_H

#include "board.h"
#include "combinations.h"
#include <iostream>
#include <random>

namespace kakuro {

class SolutionGenerator {
public:
  SolutionGenerator(std::mt19937& random) : random_{random} {}

  bool Generate(Board& board) {
    while (true) {
      Cell& cell = FindFreeCell(board);
      if (cell.isBlock || cell.number > 0) {
        break;
      }

      Cell& rowBlock = board.RowBlock(cell);
      Cell& columnBlock = board.ColumnBlock(cell);

      bool hasRowSum = rowBlock.rowBlockSum > 0;
      bool hasColumnSum = columnBlock.columnBlockSum > 0;
      if (hasRowSum && hasColumnSum) {
        break;
      }

      bool fillRowSum = !hasRowSum;
      if (!hasRowSum && !hasColumnSum) {
        fillRowSum = coinFlipDistribution_(random);
      }

      if (fillRowSum) {
        std::cout << "\tFinding row sum for (" << cell.row << ", " << cell.column << ") "
                  << std::endl;
        auto rowBlockCandidates = FindRowBlockCandidates(board, *cell.rowBlock);
        if (rowBlockCandidates.empty()) {
          break;
        }

        auto minDifficultyIndex = findMinDifficultyCandidate(rowBlockCandidates);
        auto chosenCandidate = rowBlockCandidates[minDifficultyIndex];
        int candidateNum = NumNumbers(chosenCandidate);
        int candidateSum = SumNumbers(chosenCandidate);
        cell.rowBlock->sumRow = candidateSum;

        board.ApplyRowConstraints(combinations, *cell.rowBlock, candidateNum);

        std::cout << "\tAdded row sum for (" << cell.row << ", " << cell.column
                  << "): " << candidateSum << " (candidates: " << rowBlockCandidates.size() << ")"
                  << std::endl;
      } else {
        std::cout << "\tFinding column sum for (" << cell.row << ", " << cell.column << ") "
                  << std::endl;
        auto columnBlockCandidates =
            board.FindColumnBlockCandidates(combinations, *cell.columnBlock);
        if (columnBlockCandidates.empty()) {
          break;
        }

        auto minDifficultyIndex = findMinDifficultyCandidate(columnBlockCandidates);
        auto chosenCandidate = columnBlockCandidates[minDifficultyIndex];
        int candidateNum = NumNumbers(chosenCandidate);
        int candidateSum = SumNumbers(chosenCandidate);
        cell.columnBlock->sumColumn = candidateSum;

        board.ApplyColumnConstraints(combinations, *cell.columnBlock, candidateNum);

        std::cout << "\tAdded column sum for (" << cell.row << ", " << cell.column
                  << "): " << candidateSum << " (candidates: " << columnBlockCandidates.size()
                  << ")" << std::endl;
      }
    }
  }

private:
  Cell& FindFreeCell(Board& board) {
    int boardSize = board.Rows() * board.Columns();
    std::uniform_int_distribution<> numberDistribution{0, boardSize - 1};

    int cellIndex = numberDistribution(random);
    int startingIndex = cellIndex;
    while (true) {
      Cell& cell = board[cellIndex];

      if (!cell.isBlock) {
        if (cell.number == 0) {
          if (board.RowBlock(cell).rowBlockSum == 0 ||
              board.ColumnBlock(cell).columnBlockSum == 0) {
            break;
          }
        }
      }

      cellIndex = (cellIndex + 1) % boardSize;

      if (cellIndex == startingIndex) {
        break;
      }
    }

    return board[cellIndex];
  }

  std::vector<std::array<bool, 10>> FindRowBlockCandidates(Board& board, Cell& cell) {
    assert(cell.isBlock);
    assert(cell.rowBlockSum == 0);

    std::size_t minDifficulty = 9;
    std::vector<std::array<bool, 10>> candidates;
    std::unordered_map<int, bool> sumSolvable;
    std::function<void(std::array<bool, 10>, int)> pick;
    pick = [this, &board, &cell, &pick, &minDifficulty, &candidates, &sumSolvable](
               std::array<bool, 10> numbers, int column) {
      auto checkPick = [&](Numbers numbers) {
        int num = numbers.Count();
        int sum = numbers.Sum();

        if (num == 0) {
          return false;
        }

        auto difficulty = combinations_[sum][num].size();
        if (difficulty <= minDifficulty) {
          auto query = sumSolvable.find(sum);
          if (query == sumSolvable.end()) {
            cell.rowBlockSum = sum;
            std::cerr << "\t\tSolving for sum " << sum << std::endl;
            query = sumSolvable.insert({sum, CheckSolvable()}).first;
            cell.rowBlockSum = 0;
          }
          bool isSolvable = query->second;

          if (isSolvable) {
            minDifficulty = difficulty;
          }

          return isSolvable;
        }

        return false;
      };

      if (column >= board.Columns()) {
        if (checkPick(numbers)) {
          candidates.push_back(numbers);
        }
        return;
      }

      Cell& currentCell = CellAt(cell.row, column);

      if (currentCell.isBlock) {
        if (checkPick(numbers)) {
          candidates.push_back(numbers);
        }
        return;
      }

      int numMustBeOneOf = NumNumbers(currentCell.mustBeOneOf);
      for (int i = 1; i <= 9; i++) {
        if (numbers[i]) {
          continue;
        }

        if (numMustBeOneOf > 0 && !currentCell.mustBeOneOf[i]) {
          continue;
        }

        if (currentCell.cannotBe[i]) {
          continue;
        }

        std::array<bool, 10> newNumbers = numbers;
        newNumbers[i] = true;
        pick(newNumbers, column + 1);
      }
    };

    std::array<bool, 10> numbers;
    ClearNumbers(numbers);
    pick(numbers, cell.column + 1);

    return candidates;
  }

  std::mt19937& random_;
  std::bernoulli_distribution coinFlipDistribution_{0.5};
  Combinations combinations_;
};

} // namespace kakuro

#endif
