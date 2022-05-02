#ifndef COMBINATIONS_H
#define COMBINATIONS_H

#include "numbers.h"
#include <array>
#include <vector>

namespace kakuro {

class Combinations {
public:
  Combinations() {
    Numbers numbers;
    AddNumber(numbers, 1);
  }

  const std::array<std::vector<Numbers>, 10>& operator[](int index) const {
    assert(index >= 0);
    assert(index < 46);
    return combinations[index];
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

} // namespace kakuro

#endif
