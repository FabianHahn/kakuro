#include <iostream>
#include <fstream>

#include "board.h"
#include "board_generator.h"
#include "critical_path_finder.h"
#include "numbers.h"

using namespace kakuro;

int main(int argc, char** argv)
{
  if (argc != 5) {
    std::cerr << "Usage: kakuro [rows] [columns] [block probability] [output file]" << std::endl;
    std::cerr << "Example: kakuro 20 32 0.3 kakuro.html" << std::endl;
    std::cerr << "Then open the resulting kakuro.html file in your browser." << std::endl;
    std::cerr << "The cells contain the solution as background color, select the text to see it." << std::endl;
    return EXIT_FAILURE;
  }

  int rows = std::atoi(argv[1]);
  int columns = std::atoi(argv[2]);
  double blockProbability = std::atof(argv[3]);
  std::string outputFilename{argv[4]};

  std::random_device randomDevice;
  std::mt19937 random;
  random.seed(3);

  std::cout << "Generating board..." << std::endl;
  BoardGenerator boardGenerator{random, blockProbability};
  auto board = boardGenerator.Generate(rows, columns);

  std::ofstream outputFile{outputFilename};
  if (!outputFile) {
    std::cout << "Failed to open output file" << std::endl;
  }

  board.RenderHtml(outputFile);

  return EXIT_SUCCESS;
}
