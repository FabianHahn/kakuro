#ifndef COMBINATIONS_H
#define COMBINATIONS_H

#include "numbers.h"
#include <array>
#include <vector>

namespace kakuro {

class Combinations {
public:
  struct CombinationsPerSizePerSum {
    std::vector<Numbers> numberCombinations;
    Numbers possibleNumbers;
    Numbers necessaryNumbers;
  };

  Combinations() {
    Numbers numbers;
    AddNumber(numbers, 1);

    for (int sum = 1; sum <= 45; sum++) {
      for (int size = 1; size <= 9; size++) {
        const auto& possibleCombinations = combinations[sum][size].numberCombinations;
        if (possibleCombinations.empty()) {
          continue;
        }

        Numbers& possibleNumbers = combinations[sum][size].possibleNumbers;
        Numbers& necessaryNumbers = combinations[sum][size].necessaryNumbers;
        possibleNumbers = Numbers{};
        necessaryNumbers.Fill();
        for (auto combination : possibleCombinations) {
          possibleNumbers.Or(combination);
          necessaryNumbers.And(combination);
        }
      }
    }
  }

  const CombinationsPerSizePerSum& PerSizePerSum(int sum, int size) const {
    assert(sum >= 0);
    assert(sum < 46);
    assert(size > 0);
    assert(size <= 9);
    return combinations[sum][size];
  }

private:
  void AddNumber(Numbers numbers, int number) {
    int count = numbers.Count();
    int sum = numbers.Sum();
    if (number > 9) {
      combinations[sum][count].numberCombinations.push_back(numbers);
    }

    if (number <= 9) {
      AddNumber(numbers, number + 1);

      numbers.Add(number);
      AddNumber(numbers, number + 1);
    }
  }


  std::array<std::array<CombinationsPerSizePerSum, 10>, 46> combinations;
};

} // namespace kakuro

#endif
