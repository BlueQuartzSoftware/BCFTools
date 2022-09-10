#include "BcfHdf5Convertor.h"

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

int main(int argc, char* argv[])
{
  std::string version("1.0.0");
  const size_t k_InputFileIndex = 0;
  const size_t k_OutputFileIndex = 1;
  const size_t k_FlipPatter = 2;
  const size_t k_Reorder = 3;
  const size_t k_HelpIndex = 4;

  using ArgEntry = std::vector<std::string>;
  using ArgEntries = std::vector<ArgEntry>;

  ArgEntries args;

  args.push_back({"-b", "--bcf", "The input file to process"});
  args.push_back({"-o", "--output", "The output file to write"});
  args.push_back({"-r", "--reorder", "Reorder Data inside of HDF5 file. This can increase final file size significantly. true or false."});
  args.push_back({"-f", "--flip", "Flip the patterns across the X Axis (Vertical Flip). true or false."});
  args.push_back({"-h", "--help", "Show help for this program"});

  std::string inputFile;
  std::string outputFile;
  std::string reorder;
  std::string flipPatterns;
  bool header = false;

  for(int32_t i = 0; i < argc; i++)
  {
    if(argv[i] == args[k_InputFileIndex][0] || argv[i] == args[k_InputFileIndex][1])
    {
      inputFile = argv[++i];
    }
    if(argv[i] == args[k_OutputFileIndex][0] || argv[i] == args[k_OutputFileIndex][1])
    {
      outputFile = argv[++i];
    }
    if(argv[i] == args[k_FlipPatter][0] || argv[i] == args[k_FlipPatter][1])
    {
      flipPatterns = argv[++i];
    }
    if(argv[i] == args[k_Reorder][0] || argv[i] == args[k_Reorder][1])
    {
      reorder = argv[++i];
    }

    if(argv[i] == args[k_HelpIndex][0] || argv[i] == args[k_HelpIndex][1])
    {
      std::cout << "This program has the following arguments:" << std::endl;
      for(const auto& input : args)
      {
        std::cout << input[0] << ", " << input[1] << ": " << input[2] << std::endl;
      }
      return 0;
    }
  }


  if(argc != 9)
  {
    std::cout << "7 Arguments are required. Use --help for more information." << std::endl;
    return EXIT_FAILURE;
  }


  BcfHdf5Convertor convertor(inputFile, outputFile);
  convertor.setReorder(reorder == "true");
  convertor.setFlipPatterns(flipPatterns == "true");
  convertor.execute();
  int32_t err = convertor.getErrorCode();
  if(err < 0)
  {
    std::cout << convertor.getErrorMessage() << ": " << convertor.getErrorCode() << std::endl;
  }

  std::cout << "Complete" << std::endl;

  return err;
}
