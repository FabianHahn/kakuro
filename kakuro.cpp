#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>

#include "board.h"
#include "board_generator.h"
#include "critical_path_finder.h"
#include "numbers.h"

void ClearNumbers(std::array<bool, 10>& numbers) {
	for (int i = 0; i <= 9; i++) {
		numbers[i] = false;
	}
};

int NumNumbers(std::array<bool, 10>& numbers) {
	int num = 0;
	for (int i = 1; i <= 9; i++) {
		if (numbers[i]) {
			num++;
		}
	}
	return num;
};

int SumNumbers(std::array<bool, 10>& numbers) {
	int sum = 0;
	for (int i = 1; i <= 9; i++) {
		if (numbers[i]) {
			sum += i;
		}
	}
	return sum;
};

int ChooseOneOfNumber(std::array<bool, 10>& numbers, std::mt19937 random) {
	int numNumbers = NumNumbers(numbers);
	if (numNumbers == 0) {
		return 0;
	}

	std::uniform_int_distribution<> mustBeOneOfDistribution{ 1, numNumbers };

	int whichOneOf = mustBeOneOfDistribution(random);
	int currentOneOf = 0;
	for (int i = 1; i <= 9; i++) {
		if (numbers[i]) {
			currentOneOf++;
		}

		if (currentOneOf == whichOneOf) {
			return i;
		}
	}

	return 0;
}

struct Cell {
	int row;
	int column;
	bool isBlock;
	bool isMarked;
	int number;
	Cell *rowBlock;
	Cell *columnBlock;
	std::array<bool, 10> rowNumbers;
	std::array<bool, 10> columnNumbers;
	int rowBlockSize;
	int columnBlockSize;
	int sumRow;
	int sumColumn;
	std::array<bool, 10> cannotBe;
	std::array<bool, 10> mustBeOneOf;

	void ClearRowNumbers() {
		ClearNumbers(rowNumbers);
	}

	void ClearColumnNumbers() {
		ClearNumbers(columnNumbers);
	}

	int NumRowNumbers() {
		return NumNumbers(rowNumbers);
	}

	int NumColumnNumbers() {
		return NumNumbers(columnNumbers);
	}

	int RowBlockDistance() {
		return column - rowBlock->column;
	}

	int ColumnBlockDistance() {
		return row - columnBlock->row;
	}

	void Sum() {
		sumRow = SumNumbers(rowNumbers);
		sumColumn = SumNumbers(columnNumbers);
	}
};

class Board {
public:
	Board(int numRows, int numColumns)
		: numRows_{ numRows }, numColumns_{ numColumns } {
		cells_ = new Cell[numRows_ * numColumns_];
		numNumbers_ = numRows_ * numColumns_;
		for (int row = 0; row < numRows_; row++) {
			for (int column = 0; column < numColumns_; column++) {
				auto& cell = CellAt(row, column);
				cell.row = row;
				cell.column = column;

				if (row == 0 || column == 0) {
					cell.isBlock = true;
					cell.rowBlock = &cell;
					cell.columnBlock = &cell;
					numNumbers_--;

					cell.rowBlockSize = 0;
					cell.columnBlockSize = 0;
					if (row == 0 && column != 0) {
						cell.columnBlockSize = numRows_ - 1;
					}

					if (column == 0 && row != 0) {
						cell.rowBlockSize = numColumns_ - 1;
					}
				}
				else {
					cell.isBlock = false;
					cell.rowBlock = &CellAt(row, 0);
					cell.columnBlock = &CellAt(0, column);
				}

				cell.isMarked = false;
				cell.number = 0;
				cell.sumColumn = 0;
				cell.sumRow = 0;

				cell.ClearColumnNumbers();
				cell.ClearRowNumbers();

				ClearNumbers(cell.cannotBe);
				ClearNumbers(cell.mustBeOneOf);
			}
		}
	}

	~Board() {
		delete[] cells_;
	}

	int Rows() {
		return numRows_;
	}

	int Columns() {
		return numColumns_;
	}

	Cell& CellAt(int row, int column) {
		return cells_[row * numColumns_ + column];
	}

	Cell& FindFreeCell(std::mt19937 random) {
		std::uniform_int_distribution<> numberDistribution{ 0, numRows_ * numColumns_ - 1 };

		int cellIndex = numberDistribution(random);
		int startingIndex = cellIndex;
		while (true) {
			Cell& cell = cells_[cellIndex];

			if (!cell.isBlock) {
				if (cell.number == 0) {
					if (cell.rowBlock->sumRow == 0 || cell.columnBlock->sumColumn == 0) {
						break;
					}
				}
			}

			cellIndex = (cellIndex + 1) % (numRows_ * numColumns_);

			if (cellIndex == startingIndex) {
				break;
			}
		}

		return cells_[cellIndex];
	}

	void ClearMarks() {
		for (int row = 0; row < numRows_; row++) {
			for (int column = 0; column < numColumns_; column++) {
				CellAt(row, column).isMarked = false;
			}
		}
	}

	int CountReachableCells(Cell& cell) {
		if (cell.isBlock || cell.isMarked) {
			return 0;
		}

		int numReachableUnmarked = 1;
		cell.isMarked = true;

		numReachableUnmarked += CountReachableCells(CellAt(cell.row - 1, cell.column));
		numReachableUnmarked += CountReachableCells(CellAt(cell.row, cell.column - 1));

		if (cell.row + 1 < numRows_) {
			numReachableUnmarked += CountReachableCells(CellAt(cell.row + 1, cell.column));
		}
		if (cell.column + 1 < numColumns_) {
			numReachableUnmarked += CountReachableCells(CellAt(cell.row, cell.column + 1));
		}

		return numReachableUnmarked;
	}

	bool IsCriticalPath(Cell& cell)
	{
		if (cell.isBlock) {
			return false;
		}

		Cell &topCell = CellAt(cell.row - 1, cell.column);
		if (!topCell.isBlock) {
			ClearMarks();
			cell.isMarked = true;
			int numReachableTop = CountReachableCells(topCell);
			if (numReachableTop != numNumbers_ - 1) {
				return true;
			}
		}

		Cell &leftCell = CellAt(cell.row, cell.column - 1);
		if (!leftCell.isBlock) {
			ClearMarks();
			cell.isMarked = true;
			int numReachableLeft = CountReachableCells(leftCell);
			if (numReachableLeft != numNumbers_ - 1) {
				return true;
			}
		}

		if (cell.row + 1 < numRows_) {
			Cell& bottomCell = CellAt(cell.row + 1, cell.column);
			if (!bottomCell.isBlock) {
				ClearMarks();
				cell.isMarked = true;
				int numReachableBottom = CountReachableCells(bottomCell);
				if (numReachableBottom != numNumbers_ - 1) {
					return true;
				}
			}
		}

		if (cell.column + 1 < numColumns_) {
			Cell& rightCell = CellAt(cell.row, cell.column + 1);
			if (!rightCell.isBlock) {
				ClearMarks();
				cell.isMarked = true;
				int numReachableRight = CountReachableCells(rightCell);
				if (numReachableRight != numNumbers_ - 1) {
					return true;
				}
			}
		}

		return false;
	}

	void MakeBlock(Cell& cell) {
		if (cell.isBlock) {
			return;
		}

		cell.rowBlock->rowBlockSize = cell.RowBlockDistance() - 1;
		cell.columnBlock->columnBlockSize = cell.ColumnBlockDistance() - 1;

		numNumbers_--;
		cell.isBlock = true;
		cell.number = 0;
		cell.columnBlockSize = 0;
		for (int row = cell.row + 1; row < numRows_; row++) {
			Cell& currentCell = CellAt(row, cell.column);

			if (currentCell.isBlock) {
				break;
			}

			currentCell.columnBlock = &cell;
			cell.columnBlockSize++;
		}

		cell.rowBlockSize = 0;
		for (int column = cell.column + 1; column < numColumns_; column++) {
			Cell& currentCell = CellAt(cell.row, column);

			if (currentCell.isBlock) {
				break;
			}

			currentCell.rowBlock = &cell;
			cell.rowBlockSize++;
		}
	}

	void FillNumber(Cell& cell, int number) {
		if (cell.isBlock) {
			return;
		}

		cell.number = number;
		cell.rowBlock->rowNumbers[number] = true;
		cell.columnBlock->columnNumbers[number] = true;

		for (int row = cell.columnBlock->row + 1; row < numRows_; row++) {
			Cell& currentCell = CellAt(row, cell.column);
			if (currentCell.isBlock) {
				break;
			}

			currentCell.cannotBe[cell.number] = true;
		}
		for (int column = cell.rowBlock->column + 1; column < numColumns_; column++) {
			Cell& currentCell = CellAt(cell.row, column);
			if (currentCell.isBlock) {
				break;
			}

			currentCell.cannotBe[cell.number] = true;
		}
	}

	void FillThinNeighbors(Cell& cell) {
		if (cell.isBlock) {
			return;
		}

		int rowBlockDistance = cell.RowBlockDistance();
		int columnBlockDistance = cell.ColumnBlockDistance();

		bool isNextRowFree = false;
		if (cell.row + 1 < numRows_ && !CellAt(cell.row + 1, cell.column).isBlock) {
			isNextRowFree = true;
		}

		bool isNextColumnFree = false;
		if (cell.column + 1 < numColumns_ && !CellAt(cell.row, cell.column + 1).isBlock) {
			isNextColumnFree = true;
		}

		bool isLockedInRows = columnBlockDistance == 1 && !isNextRowFree;
		bool isLockedInColumns = rowBlockDistance == 1 && !isNextColumnFree;
		if (isLockedInRows || isLockedInColumns) {
			MakeBlock(cell);
			FillThinNeighbors(CellAt(cell.row - 1, cell.column));
			FillThinNeighbors(CellAt(cell.row, cell.column - 1));

			if (cell.row + 1 < numRows_) {
				FillThinNeighbors(CellAt(cell.row + 1, cell.column));
			}
			if (cell.column + 1 < numColumns_) {
				FillThinNeighbors(CellAt(cell.row, cell.column + 1));
			}
		}
	}

	std::vector<std::array<bool, 10>> FindRowBlockCandidates(const std::array<std::array<std::vector<std::array<bool, 10>>, 10>, 46>& combinations, Cell& cell) {
		assert(cell.isBlock);
		assert(cell.sumRow == 0);

		std::size_t minDifficulty = 9;
		std::vector<std::array<bool, 10>> candidates;
		std::unordered_map<int, bool> sumSolvable;
		std::function<void(std::array<bool, 10>, int)> pick;
		pick = [this, &combinations, &cell, &pick, &minDifficulty, &candidates, &sumSolvable](std::array<bool, 10> numbers, int column) {
			auto checkPick = [&](std::array<bool, 10> numbers) {
				int num = NumNumbers(numbers);
				int sum = SumNumbers(numbers);

				if (num == 0) {
					return false;
				}

				auto difficulty = combinations[sum][num].size();
				if (difficulty <= minDifficulty) {
					auto query = sumSolvable.find(sum);
					if (query == sumSolvable.end()) {
						cell.sumRow = sum;
						std::cerr << "\t\tSolving for sum " << sum << std::endl;
						query = sumSolvable.insert({ sum, CheckSolvable() }).first;
						cell.sumRow = 0;
					}
					bool isSolvable = query->second;

					if (isSolvable) {
						minDifficulty = difficulty;
					}

					return isSolvable;
				}

				return false;
			};

			if (column >= numColumns_) {
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

	std::vector<std::array<bool, 10>> FindColumnBlockCandidates(const std::array<std::array<std::vector<std::array<bool, 10>>, 10>, 46>& combinations, Cell& cell) {
		assert(cell.isBlock);
		assert(cell.sumColumn == 0);

		std::size_t minDifficulty = 9;
		std::vector<std::array<bool, 10>> candidates;
		std::unordered_map<int, bool> sumSolvable;
		std::function<void(std::array<bool, 10>, int)> pick;
		pick = [this, &combinations, &cell, &pick, &minDifficulty, &candidates, &sumSolvable](std::array<bool, 10> numbers, int row) {
			auto checkPick = [&](std::array<bool, 10> numbers) {
				int num = NumNumbers(numbers);
				int sum = SumNumbers(numbers);

				if (num == 0) {
					return false;
				}

				auto difficulty = combinations[sum][num].size();
				if (difficulty <= minDifficulty) {
					auto query = sumSolvable.find(sum);
					if (query == sumSolvable.end()) {
						cell.sumColumn = sum;
						std::cerr << "\t\tSolving for sum " << sum << std::endl;
						query = sumSolvable.insert({ sum, CheckSolvable() }).first;
						cell.sumColumn = 0;
					}
					bool isSolvable = query->second;

					if (isSolvable) {
						minDifficulty = difficulty;
					}

					return isSolvable;
				}

				return false;
			};

			if (row >= numRows_) {
				if (checkPick(numbers)) {
					candidates.push_back(numbers);
				}
				return;
			}

			Cell& currentCell = CellAt(row, cell.column);

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
				pick(newNumbers, row + 1);
			}
		};

		std::array<bool, 10> numbers;
		ClearNumbers(numbers);
		pick(numbers, cell.row + 1);

		return candidates;
	}

	void ApplyRowConstraints(std::array<std::array<std::vector<std::array<bool, 10>>, 10>, 46> combinations, Cell& cell, int rowBlockSize) {
		if (!cell.isBlock || cell.sumRow == 0) {
			return;
		}

		auto sumNumCombinations = combinations[cell.sumRow][rowBlockSize];
		std::vector<std::vector<int>> sequences;

		for (std::size_t i = 0; i < sumNumCombinations.size(); i++) {
			auto combination = sumNumCombinations[i];

			std::function<void(std::vector<int>, std::array<bool, 10>, int)> pick;
			pick = [this, &cell, &pick, &sequences](std::vector<int> sequence, std::array<bool, 10> remainingNumbers, int column) {
				if (NumNumbers(remainingNumbers) == 0) {
					sequences.push_back(sequence);
					return;
				}

				Cell& currentCell = CellAt(cell.row, column);
				assert(!currentCell.isBlock);

				for (int i = 1; i <= 9; i++) {
					if (!remainingNumbers[i]) {
						continue;
					}

					if (currentCell.cannotBe[i]) {
						continue;
					}

					if (NumNumbers(currentCell.mustBeOneOf) > 0 && !currentCell.mustBeOneOf[i]) {
						continue;
					}

					std::vector<int> newSequence = sequence;
					newSequence.push_back(i);
					std::array<bool, 10> newRemainingNumbers = remainingNumbers;
					newRemainingNumbers[i] = false;
					pick(newSequence, newRemainingNumbers, column + 1);
				}
			};
			
			std::vector<int> sequence;
			pick(sequence, combination, cell.column + 1);
		}

		for (int i = 0; i < rowBlockSize; i++) {
			int column = cell.column + i + 1;
			assert(column < numColumns_);
			Cell& currentCell = CellAt(cell.row, column);
			assert(!currentCell.isBlock);
			
			std::array<bool, 10> usedNumbers;
			ClearNumbers(usedNumbers);
			for (auto sequence : sequences) {
				usedNumbers[sequence[i]] = true;
			}

			if (NumNumbers(currentCell.mustBeOneOf) > 0) {
				for (int j = 1; j <= 9; j++) {
					if (currentCell.mustBeOneOf[j] && !usedNumbers[j]) {
						currentCell.mustBeOneOf[j] = false;
					}
				}
			}
			else {
				for (int j = 1; j <= 9; j++) {
					currentCell.mustBeOneOf[j] = usedNumbers[j];
				}
			}

			for (int j = 1; j <= 9; j++) {
				if (!usedNumbers[j]) {
					currentCell.cannotBe[j] = true;
				}
			}

			if (NumNumbers(currentCell.mustBeOneOf) == 1) {
				for (int j = 1; j <= 9; j++) {
					if (currentCell.mustBeOneOf[j]) {
						currentCell.number = j;
					}
				}
			}
		}
	}

	void ApplyColumnConstraints(std::array<std::array<std::vector<std::array<bool, 10>>, 10>, 46> combinations, Cell& cell, int columnBlockSize) {
		if (!cell.isBlock || cell.sumColumn == 0) {
			return;
		}

		auto sumNumCombinations = combinations[cell.sumColumn][columnBlockSize];
		std::vector<std::vector<int>> sequences;

		for (std::size_t i = 0; i < sumNumCombinations.size(); i++) {
			auto combination = sumNumCombinations[i];

			std::function<void(std::vector<int>, std::array<bool, 10>, int)> pick;
			pick = [this, &cell, &pick, &sequences](std::vector<int> sequence, std::array<bool, 10> remainingNumbers, int row) {
				if (NumNumbers(remainingNumbers) == 0) {
					sequences.push_back(sequence);
					return;
				}

				Cell& currentCell = CellAt(row, cell.column);
				assert(!currentCell.isBlock);

				for (int i = 1; i <= 9; i++) {
					if (!remainingNumbers[i]) {
						continue;
					}

					if (currentCell.cannotBe[i]) {
						continue;
					}

					if (NumNumbers(currentCell.mustBeOneOf) > 0 && !currentCell.mustBeOneOf[i]) {
						continue;
					}

					std::vector<int> newSequence = sequence;
					newSequence.push_back(i);
					std::array<bool, 10> newRemainingNumbers = remainingNumbers;
					newRemainingNumbers[i] = false;
					pick(newSequence, newRemainingNumbers, row + 1);
				}
			};

			std::vector<int> sequence;
			pick(sequence, combination, cell.row + 1);
		}

		for (int i = 0; i < columnBlockSize; i++) {
			int row = cell.row + i + 1;
			assert(row < numRows_);
			Cell& currentCell = CellAt(row, cell.column);
			assert(!currentCell.isBlock);

			std::array<bool, 10> usedNumbers;
			ClearNumbers(usedNumbers);
			for (auto sequence : sequences) {
				usedNumbers[sequence[i]] = true;
			}

			if (NumNumbers(currentCell.mustBeOneOf) > 0) {
				for (int j = 1; j <= 9; j++) {
					if (currentCell.mustBeOneOf[j] && !usedNumbers[j]) {
						currentCell.mustBeOneOf[j] = false;
					}
				}
			}
			else {
				for (int j = 1; j <= 9; j++) {
					currentCell.mustBeOneOf[j] = usedNumbers[j];
				}
			}

			for (int j = 1; j <= 9; j++) {
				if (!usedNumbers[j]) {
					currentCell.cannotBe[j] = true;
				}
			}

			if (NumNumbers(currentCell.mustBeOneOf) == 1) {
				for (int j = 1; j <= 9; j++) {
					if (currentCell.mustBeOneOf[j]) {
						currentCell.number = j;
					}
				}
			}
		}
	}

	bool CheckSolvable() {
		auto *numbers = new int[numColumns_ * numRows_];
		memset(numbers, 0, numColumns_ * numRows_ * sizeof(int));

		auto checkRowValid = [this, numbers](Cell& rowBlockCell) {
			assert(rowBlockCell.isBlock);

			int currentSum = 0;
			std::array<bool, 10> rowNumbers;
			ClearNumbers(rowNumbers);
			bool isFull = true;

			for (int column = rowBlockCell.column + 1; column < numColumns_; column++) {
				Cell& currentCell = CellAt(rowBlockCell.row, column);
				if (currentCell.isBlock) {
					break;
				}

				int number = numbers[currentCell.row * numColumns_ + currentCell.column];
				if (number == 0) {
					isFull = false;
					continue;
				}

				currentSum += number;

				if (rowNumbers[number]) {
					return false;
				}

				rowNumbers[number] = true;
			}

			if (isFull && rowBlockCell.sumRow > 0 && currentSum != rowBlockCell.sumRow) {
				return false;
			}

			return true;
		};

		auto checkColumnValid = [this, numbers](Cell& columnBlockCell) {
			assert(columnBlockCell.isBlock);

			int currentSum = 0;
			std::array<bool, 10> columnNumbers;
			ClearNumbers(columnNumbers);
			bool isFull = true;

			for (int row = columnBlockCell.row + 1; row < numRows_; row++) {
				Cell& currentCell = CellAt(row, columnBlockCell.column);
				if (currentCell.isBlock) {
					break;
				}

				int number = numbers[currentCell.row * numColumns_ + currentCell.column];
				if (number == 0) {
					isFull = false;
					continue;
				}

				currentSum += number;

				if (columnNumbers[number]) {
					return false;
				}

				columnNumbers[number] = true;
			}

			if (isFull && columnBlockCell.sumColumn > 0 && currentSum != columnBlockCell.sumColumn) {
				return false;
			}

			return true;
		};

		std::function<bool(int)> solve;
		solve = [this, numbers, &solve, &checkRowValid, &checkColumnValid](int cellIndex) {
			if (cellIndex >= numColumns_ * numRows_) {
				return true;
			}

			Cell& cell = cells_[cellIndex];
			if (cell.isBlock) {
				return solve(cellIndex + 1);
			}

			if (cell.number > 0) {
				numbers[cellIndex] = cell.number;
				return solve(cellIndex + 1);
			}

			for (int i = 1; i <= 9; i++) {
				if (cell.cannotBe[i]) {
					continue;
				}

				if (NumNumbers(cell.mustBeOneOf) && !cell.mustBeOneOf[i]) {
					continue;
				}

				numbers[cellIndex] = i;
				bool isRowValid = checkRowValid(*cell.rowBlock);
				bool isColumnValid = checkColumnValid(*cell.columnBlock);
				if (isRowValid && isColumnValid) {
					if (solve(cellIndex + 1)) {
						return true;
					}
				}
				numbers[cellIndex] = 0;
			}

			return false;
		};

		bool result = solve(0);
		delete[] numbers;
		return result;
	}

	void ComputeSums() {
		for (int row = 0; row < numRows_; row++) {
			for (int column = 0; column < numColumns_; column++) {
				auto& cell = CellAt(row, column);
				cell.Sum();
				// cell.sumColumn = cell.columnBlockSize;
				// cell.sumRow = cell.rowBlockSize;
			}
		}
	}

	void Print() {
		std::cout << "<!doctype html>" << std::endl;
		std::cout << "<html>" << std::endl;
		std::cout << "<head>" << std::endl;
		std::cout << "<meta charset = \"utf-8\">" << std::endl;
		std::cout << "<title>Karuko</title>" << std::endl;
		std::cout << "<style type=\"text/css\">" << std::endl;
		std::cout << "table { border-collapse: collapse }" << std::endl;
		std::cout << "td { text-align: center; vertical-align: middle; color: white }" << std::endl;
		std::cout << "td.cell { width: 48px; height: 48px; border: 1px solid black }" << std::endl;
		std::cout << "</style>" << std::endl;
		std::cout << "</head>" << std::endl;
		std::cout << "<body>" << std::endl;
		std::cout << "<table>" << std::endl;

		for (int row = 0; row < numRows_; row++) {
			std::cout << "\t<tr>" << std::endl;
			for (int column = 0; column < numColumns_; column++) {
				const auto& cell = CellAt(row, column);

				if (cell.isBlock) {
					auto ifNonZero = [](int i) -> std::string {
						if (i > 0) {
							return std::to_string(i);
						}
						else {
							return "&nbsp;";
						}
					};

					std::cout << "\t\t<td class=\"cell\" style=\"background-color: black\">" << std::endl;
					std::cout << "\t\t\t<table style=\"width: 100%; height: 100%;\">" << std::endl;
					std::cout << "\t\t\t\t<tr>" << std::endl;
					std::cout << "\t\t\t\t\t<td></td>" << std::endl;
					std::cout << "\t\t\t\t\t<td style=\"text-align:right;\">" << ifNonZero(cell.sumRow) << "</td>" << std::endl;
					std::cout << "\t\t\t\t</tr>" << std::endl;
					std::cout << "\t\t\t\t<tr>" << std::endl;
					std::cout << "\t\t\t\t\t<td style=\"text-align:left;\">" << ifNonZero(cell.sumColumn) << "</td>" << std::endl;
					std::cout << "\t\t\t\t\t<td></td>" << std::endl;
					std::cout << "\t\t\t\t</tr>" << std::endl;
					std::cout << "\t\t\t</table>" << std::endl;
				}
				else {
					std::cout << "\t\t<td class=\"cell\">" << std::endl;
					std::cout << "\t\t\t" << cell.number << std::endl;
				}

				std::cout << "\t\t</td>" << std::endl;
			}
			std::cout << "\t</tr>" << std::endl;
		}

		std::cout << "</table>" << std::endl;
		std::cout << "</body>" << std::endl;
		std::cout << "</html>" << std::endl;
	}

private:
	int numRows_;
	int numColumns_;
	Cell *cells_;
	int numNumbers_;
};

int main(int argc, char** argv)
{
	if (argc != 4) {
		std::cerr << "Usage: kakuro [rows] [columns] [block probability]" << std::endl;
		std::cerr << "Example: kakuro 20 32 0.3 > kakuro.html" << std::endl;
		std::cerr << "Then open the resulting kakuro.html file in your browser." << std::endl;
		std::cerr << "The cells contain the solution as background color, select the text to see it." << std::endl;
		return EXIT_FAILURE;
	}

	int numRows = std::atoi(argv[1]);
	int numColumns = std::atoi(argv[2]);
	double blockProbability = std::atof(argv[3]);

	std::random_device randomDevice;
	std::mt19937 random;
	random.seed(3);
	std::uniform_int_distribution<> numberDistribution{ 1, 9 };
	std::bernoulli_distribution blockDistribution{ blockProbability };
	std::bernoulli_distribution coinFlipDistribution{ 0.5 };

	std::array<std::array<std::vector<std::array<bool, 10>>, 10>, 46> combinations;
	std::function<void(std::array<bool, 10>, int)> add_number;
	add_number = [&combinations, &add_number](std::array<bool, 10> numbers, int number) {
		int count = NumNumbers(numbers);
		int sum = SumNumbers(numbers);
		if (numbers[number - 1]) {
			combinations[sum][count].push_back(numbers);
		}

		if (number <= 9) {
			add_number(numbers, number + 1);

			numbers[number] = true;
			add_number(numbers, number + 1);
		}
	};
	std::array<bool, 10> numbers;
	ClearNumbers(numbers);
	add_number(numbers, 1);

	std::array<std::array<std::vector<std::array<bool, 10>>, 46>, 10> combinations_by_count;
	for (int i = 0; i < 46; i++) {
		for (int j = 0; j < 10; j++) {
			combinations_by_count[j][i] = combinations[i][j];
		}
	}

	Board board{ numRows, numColumns };

	std::cerr << "Generating board..." << std::endl;
	for (int row = 1; row < board.Rows(); row++) {
		for (int column = 1; column < board.Columns(); column++) {
			auto& cell = board.CellAt(row, column);

			int rowBlockDistance = cell.RowBlockDistance();
			int columnBlockDistance = cell.ColumnBlockDistance();

			int maxBlockDistance = rowBlockDistance;
			if (columnBlockDistance > rowBlockDistance) {
				maxBlockDistance = columnBlockDistance;
			}

			if (maxBlockDistance == 10) {
				board.MakeBlock(cell);
				continue;
			}

			if (rowBlockDistance == 2 || columnBlockDistance == 2) {
				continue;
			}

			bool isCriticalPath = board.IsCriticalPath(cell);

			if (isCriticalPath) {
				continue;
			}

			for (int i = 2; i < maxBlockDistance; i++) {
				if (blockDistribution(random)) {
					board.MakeBlock(cell);
				}
			}
		}
	}

	std::cerr << "Patching border..." << std::endl;
	for (int row = 1; row < board.Rows(); row++) {
		board.FillThinNeighbors(board.CellAt(row, board.Columns() - 1));
	}

	for (int column = 1; column < board.Columns(); column++) {
		board.FillThinNeighbors(board.CellAt(board.Rows() - 1, column));
	}

	auto findMinDifficultyCandidate = [&combinations](const std::vector<std::array<bool, 10>>& candidates) {
		std::size_t minDifficulty = 9;
		std::size_t minDifficultyIndex = 0;

		for (std::size_t i = 0; i < candidates.size(); i++) {
			auto candidate = candidates[i];
			int candidateNum = NumNumbers(candidate);
			int candidateSum = SumNumbers(candidate);

			auto difficulty = combinations[candidateSum][candidateNum].size();
			if (difficulty < minDifficulty) {
				minDifficultyIndex = i;
				minDifficulty = difficulty;
			}
		}

		return minDifficultyIndex;
	};

	std::cerr << "Generating sums..." << std::endl;
	while (true) {
		Cell& cell = board.FindFreeCell(random);
		if (cell.isBlock || cell.number > 0) {
			break;
		}

		bool hasRowSum = cell.rowBlock->sumRow > 0;
		bool hasColumnSum = cell.columnBlock->sumColumn > 0;
		if (hasRowSum && hasColumnSum) {
			break;
		}

		bool fillRowSum = !hasRowSum;
		if (!hasRowSum && !hasColumnSum) {
			fillRowSum = coinFlipDistribution(random);
		}

		if (fillRowSum) {
			std::cerr << "\tFinding row sum for (" << cell.row << ", " << cell.column << ") " << std::endl;
			auto rowBlockCandidates = board.FindRowBlockCandidates(combinations, *cell.rowBlock);
			if (rowBlockCandidates.empty()) {
				break;
			}

			auto minDifficultyIndex = findMinDifficultyCandidate(rowBlockCandidates);
			auto chosenCandidate = rowBlockCandidates[minDifficultyIndex];
			int candidateNum = NumNumbers(chosenCandidate);
			int candidateSum = SumNumbers(chosenCandidate);
			cell.rowBlock->sumRow = candidateSum;

			board.ApplyRowConstraints(combinations, *cell.rowBlock, candidateNum);

			std::cerr << "\tAdded row sum for (" << cell.row << ", " << cell.column << "): " << candidateSum
				<< " (candidates: " << rowBlockCandidates.size() << ")" << std::endl;
		}
		else
		{
			std::cerr << "\tFinding column sum for (" << cell.row << ", " << cell.column << ") " << std::endl;
			auto columnBlockCandidates = board.FindColumnBlockCandidates(combinations, *cell.columnBlock);
			if (columnBlockCandidates.empty()) {
				break;
			}

			auto minDifficultyIndex = findMinDifficultyCandidate(columnBlockCandidates);
			auto chosenCandidate = columnBlockCandidates[minDifficultyIndex];
			int candidateNum = NumNumbers(chosenCandidate);
			int candidateSum = SumNumbers(chosenCandidate);
			cell.columnBlock->sumColumn = candidateSum;

			board.ApplyColumnConstraints(combinations, *cell.columnBlock, candidateNum);

			std::cerr << "\tAdded column sum for (" << cell.row << ", " << cell.column << "): " << candidateSum
				<< " (candidates: " << columnBlockCandidates.size() << ")" << std::endl;
		}

		/*
		int chosenOneOf = ChooseOneOfNumber(cell.mustBeOneOf, random);
		if (chosenOneOf > 0) {
			board.FillNumber(cell, chosenOneOf);
		}

		int attempt = 0;
		int number = numberDistribution(random);
		while (attempt < 9 && cell.cannotBe[number]) {
			number = (number % 9) + 1;
			attempt++;
		}

		board.FillNumber(cell, number);
		*/
	}

	/*
	for (int row = 1; row < board.Rows(); row++) {
		for (int column = 1; column < board.Columns(); column++) {
			auto& cell = board.CellAt(row, column);

			if (cell.isBlock) {
				continue;
			}

			auto& rowBlock = *cell.rowBlock;
			auto& columnBlock = *cell.columnBlock;

			int attempt = 0;
			cell.number = numberDistribution(random);
			while (attempt < 9 && (rowBlock.rowNumbers[cell.number] || columnBlock.columnNumbers[cell.number])) {
				cell.number = (cell.number % 9) + 1;
				attempt++;
			}

			rowBlock.rowNumbers[cell.number] = true;
			columnBlock.columnNumbers[cell.number] = true;
		}
	}
	*/

	// board.ComputeSums();
	board.Print();

	return EXIT_SUCCESS;
}
