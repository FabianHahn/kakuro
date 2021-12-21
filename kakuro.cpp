#include <array>
#include <cassert>
#include <cstdlib>
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
	int number;
	std::array<bool, 10> rowNumbers;
	std::array<bool, 10> columnNumbers;
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
		for (int row = 0; row < numRows_; row++) {
			for (int column = 0; column < numColumns_; column++) {
				auto& cell = CellAt(row, column);
				cell.row = row;
				cell.column = column;

				if (row == 0 || column == 0) {
					cell.isBlock = true;
				}
				else {
					cell.isBlock = false;
				}

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

	Cell& FindRowBlock(Cell& cell) {
		if (cell.isBlock) {
			return cell;
		}

		assert(cell.column > 0);
		return FindRowBlock(CellAt(cell.row, cell.column - 1));
	}

	Cell& FindColumnBlock(Cell& cell) {
		if (cell.isBlock) {
			return cell;
		}

		assert(cell.row > 0);
		return FindColumnBlock(CellAt(cell.row - 1, cell.column));
	}

	void ComputeSums() {
		for (int row = 0; row < numRows_; row++) {
			Cell *previousBlock = &CellAt(row, 0);
			int currentSum = 0;
			for (int column = 1; column < numColumns_; column++) {
				auto& cell = CellAt(row, column);

				if (cell.isBlock) {
					previousBlock->sumRow = currentSum;
					previousBlock = &cell;
					currentSum = 0;
				}
				else {
					currentSum += cell.number;
				}
			}

			previousBlock->sumRow = currentSum;
		}

		for (int column = 0; column < numColumns_; column++) {
			Cell *previousBlock = &CellAt(0, column);
			int currentSum = 0;
			for (int row = 1; row < numRows_; row++) {
				auto& cell = CellAt(row, column);

				if (cell.isBlock) {
					previousBlock->sumColumn = currentSum;
					previousBlock = &cell;
					currentSum = 0;
				}
				else {
					currentSum += cell.number;
				}
			}

			previousBlock->sumColumn = currentSum;
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

	Board board{ numRows, numColumns };

	for (int row = 1; row < board.Rows(); row++) {
		Cell* currentRowBlock = &board.CellAt(row, 0);
		for (int column = 1; column < board.Columns(); column++) {
			auto& cell = board.CellAt(row, column);
			auto& currentColumnBlock = board.FindColumnBlock(cell);

			int numRowBlockNumbers = currentRowBlock->NumRowNumbers();
			int numColumnBlockNumbers = currentColumnBlock.NumColumnNumbers();

			int largerBlockNumbers = numRowBlockNumbers;
			if (numColumnBlockNumbers > numRowBlockNumbers) {
				largerBlockNumbers = numColumnBlockNumbers;
			}

			int numAttempts = 0;
			if (numRowBlockNumbers == 1 || numColumnBlockNumbers == 1) {
				numAttempts = 9;
			}

			bool isBlock = false;
			for (int i = 1; i < largerBlockNumbers; i++) {
				if (blockDistribution(random)) {
					isBlock = true;
					break;
				}
			}

			if (numAttempts == 0 && isBlock) {
				cell.isBlock = true;
				cell.number = 0;
				currentRowBlock = &cell;
				continue;
			}

			int attempt = 0;
			cell.number = numberDistribution(random);
			while (attempt < numAttempts && (currentRowBlock->rowNumbers[cell.number] || currentColumnBlock.columnNumbers[cell.number])) {
				cell.number = (cell.number % 9) + 1;
				attempt++;
			}

			if (row == board.Rows() - 1 && numColumnBlockNumbers == 0) {
				cell.isBlock = true;
				cell.number = 0;
				currentRowBlock = &cell;
			}
			else if (column == board.Columns() - 1 && numRowBlockNumbers == 0) {
				cell.isBlock = true;
				cell.number = 0;
				currentRowBlock = &cell;
			}
			else if (currentRowBlock->rowNumbers[cell.number] || currentColumnBlock.columnNumbers[cell.number]) {
				cell.isBlock = true;
				cell.number = 0;
				currentRowBlock = &cell;
			}
			else {
				cell.isBlock = false;
				currentRowBlock->rowNumbers[cell.number] = true;
				currentColumnBlock.columnNumbers[cell.number] = true;
			}
		}
	}

	board.ComputeSums();
	board.Print();

	return EXIT_SUCCESS;
}
