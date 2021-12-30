#ifndef NUMBERS_H
#define NUMBERS_H

#include <bitset>
#include <cassert>

namespace kakuro {

class Numbers {
public:
  Numbers() : bitset_{} {

  }

  void Add(int number) {
    assert(number > 0);
    assert(number <= 9);
    bitset_.set(number - 1);
  }

  bool Has(int number) const {
    assert(number > 0);
    assert(number <= 9);
    return bitset_[number - 1];
  }

  int Count() const {
    return static_cast<int>(bitset_.count());
  }

  int Sum() const {
    int sum = 0;
    for (int number = 1; number <= 9; number++) {
      if (Has(number)) {
        sum += number;
      }
    }
    return sum;
  }

  void Clear() {
    bitset_.reset();
  }

private:
  std::bitset<9> bitset_;
};

}

#endif