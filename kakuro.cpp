#include <array>
#include <cassert>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <random>
#include <string>

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
	std::mt19937 random{ randomDevice() };
	std::uniform_int_distribution<> numberDistribution{ 1, 9 };
	std::bernoulli_distribution blockDistribution{ blockProbability };

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

	for (int row = 1; row < board.Rows(); row++) {
		board.FillThinNeighbors(board.CellAt(row, board.Columns() - 1));
	}

	for (int column = 1; column < board.Columns(); column++) {
		board.FillThinNeighbors(board.CellAt(board.Rows() - 1, column));
	}

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

	board.ComputeSums();
	board.Print();

	return EXIT_SUCCESS;
}
