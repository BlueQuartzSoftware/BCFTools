/* ============================================================================
 * Copyright (c) 2019 BlueQuartz Software, LLC
 * All rights reserved.
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with any project and source this library is coupled.
 * If not, see <https://www.gnu.org/licenses/#GPL>.
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include <iostream>

#include "SFSReader.h"
#include "SFSNodeItem.h"



/**
 * @brief This will upack all the files within an SFS file archive. This will NOT support
 * SFS files that have compression or encryption enabled.
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char const *argv[])
{
  if(argc != 3)
  {
    std::cout << "Need the input file name and output directory" << std::endl;
    return 1;
  }
  std::string inputFile(argv[1]);
  std::string outputDir(argv[2]);
  if(outputDir.back() != '/')
  {
    outputDir = outputDir + '/';
  }

  SFSReader sfsFile;
  sfsFile.parseFile(inputFile);

 
#ifdef _WIN32
  const char slash = '\\';
  #else
  const char slash = '/';
  #endif
  size_t slashIndex = inputFile.find_last_of(slash);
  std::string baseName = inputFile.substr(slashIndex + 1);
  size_t dotIndex = baseName.find_last_of('.');
  baseName = baseName.substr(0, dotIndex);

  std::string outputPath = outputDir + baseName;
  //sfsFile.extractFile(outputPath, "EBSDData/SEMImage");
  std::cout << "Extracting to " << outputPath << std::endl;
  sfsFile.extractAll(outputPath);

  std::cout << "Complete" << std::endl;

  /* code */
  return 0;
}
