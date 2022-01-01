#ifndef COMBINATIONS_H
#define COMBINATIONS_H

#include <array>
#include <vector>

#include "numbers.h"

namespace kakuro {

class Combinations {
public:
  Combinations() {
    Numbers numbers;
    AddNumber(numbers, 1);
  }

private:
  void AddNumber(Numbers numbers, int number) {
    int count = numbers.Count();
    int sum = numbers.Sum();
    if (numbers.Has(number - 1)) {
      combinations[sum][count].push_back(numbers);
    }

    if (number <= 9) {
      AddNumber(numbers, number + 1);

      numbers.Add(number);
      AddNumber(numbers, number + 1);
    }
  }

  std::array<std::array<std::vector<Numbers>, 10>, 46> combinations;
};

}

#endif