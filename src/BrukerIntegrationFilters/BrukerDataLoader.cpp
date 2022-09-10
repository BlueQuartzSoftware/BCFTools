/* ============================================================================
 * Copyright (c) 2013 Michael A. Jackson (BlueQuartz Software)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of Michael A. Jackson,
 * BlueQuartz Software nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  This code was written under United States Air Force Contract number
 *                           FA8650-07-D-5800
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "BrukerDataLoader.h"

#include "EbsdLib/Core/EbsdLibConstants.h"

#include "BrukerIntegration/BrukerIntegrationConstants.h"
#include "BrukerIntegration/BrukerIntegrationStructs.h"

#include "StringUtilities.hpp"

#include <cstdio>
#include <iostream>
#include <filesystem>
#include <cmath>
#include <fstream>
#include <sstream>

#ifdef SIMPL_USE_GHC_FILESYSTEM
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846 /* pi */
#endif


//using namespace BrukerEbsdXml;

#if defined(__GNUC__) && !defined(__APPLE__)
#define BDL_POS(var) var.__pos
#else
#define BDL_POS(var) var
#endif

// -----------------------------------------------------------------------------
BrukerDataLoader::BrukerDataLoader() = default;

// -----------------------------------------------------------------------------
BrukerDataLoader::~BrukerDataLoader() = default;

#if 0
// -----------------------------------------------------------------------------
EbsdPatterns::Pointer BrukerDataLoader::LoadPatternData(const std::string& descFile, const std::string& dataFile)
{
  QFileInfo descFileInfo(descFile);
  if(!descFileInfo.exists())
  {
    std::cout << "The FrameDescription File does not exist: '" << descFile << "'";
    return EbsdPatterns::NullPointer();
  }

  QFileInfo dataFileInfo(dataFile);
  if(!dataFileInfo.exists())
  {
    std::cout << "The FrameData File does not exist: '" << dataFile << "'";
    return EbsdPatterns::NullPointer();
  }

  FILE* desc = fopen(descFile.c_str(), "rb");
  if (nullptr == desc)
  {
    std::cout << "Could not open the FrameDescription File";
    return EbsdPatterns::NullPointer();
  }

  qint64 filesize = dataFileInfo.size();

  // Get the total number of patterns from the FrameData file
  FrameDescriptionHeader_t descHeader;
  size_t nRead = fread(&descHeader, sizeof(int32_t), 3, desc);
  if (nRead != 3)
  {
    std::cout << "Could not read the header values. Only " << nRead  << " values were parsed";
    fclose(desc);
    return EbsdPatterns::NullPointer();
  }
  fclose(desc);
  desc = nullptr;

  int32_t xStart = 0;
  int32_t yStart = 0;
  int32_t xEnd = descHeader.width;
  int32_t yEnd = descHeader.height;

  // Open the Frame Data File
  FILE* f = fopen(dataFile.c_str(), "rb");
  if (nullptr == f)
  {
    std::cout << "Could not open the FrameData File";
    return EbsdPatterns::NullPointer();
  }

  //Read the first pattern header which will give us the height & width of the actual pattern data.
  FrameDataHeader_t patternHeader;
  nRead = fread(&patternHeader, 1, 25, f);
  if (nRead != 25)
  {
    std::cout << "Could not read the Frame Data Header values. Only " << nRead  << " values were parsed";
    fclose(f);
    return EbsdPatterns::NullPointer();
  }

  // Put the file pointer back to the start of the file
  rewind(f);

  UInt8Array::Pointer patternDataPtr = UInt8ArrayType::NullPointer();

  int32_t patternDataByteCount = patternHeader.width * patternHeader.height;

  // Loop through all the patterns loading them into memory. The Bruker Pattern data is NOT guaranteed to be in the
  // correct order. So while we are looping over the number of patterns in the x and y directions we are going to
  // read the pattern header to definitively get the proper XY index of the pattern, calculate the proper offset
  // into the array and read the data into that position
  QFileInfo fi(dataFile);
  std::vector<size_t> cDims(2);
  cDims[1] = patternHeader.height;
  cDims[0] = patternHeader.width;
  patternDataPtr = UInt8ArrayType::CreateArray(descHeader.width * descHeader.height, cDims, "EBSPData", true);
  patternDataPtr->initializeWithValue(0x00);
  //  size_t increment = patternHeader.width * patternHeader.height + 25;
  // size_t offset = 0;
  float progress = 0.0f;
  float currentProgress = 0.0f;
  size_t numPatterns = yEnd * xEnd;
  size_t i = 0;
  for(int32_t y = yStart; y < yEnd; y++)
  {
    for(int32_t x = xStart; x < xEnd; x++)
    {

      currentProgress = static_cast<float>(i) / static_cast<float>(numPatterns);
      if(currentProgress > progress)
      {
        progress = progress + 0.01f;
        std::cout << fi.fileName().toStdString() << " [" << static_cast<int>(currentProgress * 100.0f) << "%]\r";
        std::cout.flush();
      }
      i++;

      fpos_t pos;
      fgetpos(f, &pos);

      // Read the Header for the next pattern
      nRead = fread(&patternHeader, 1, 25, f);
      // We hit the end of the file, which is bad...
      if(feof(f) != 0)
      {
        std::cout << "End of file hit early. Not all expected patterns were extracted." << std::endl;
        y = yEnd;
        x = xEnd;
        break;
      }

      size_t tupleIndex = (descHeader.width * patternHeader.yIndex) + patternHeader.xIndex;
      // Read the Data associated with the header that was read.
      fgetpos(f, &pos);
      nRead = fread(patternDataPtr->getTuplePointer(tupleIndex), 1, patternDataByteCount, f);

      if(feof(f) != 0)
      {
        std::cout << "Unexpected End of File (EOF) was encountered. Details follow" << std::endl;
        std::cout << "File Size: " << filesize << std::endl;
        std::cout << "ferror(f) = " << ferror(f) << "  feof(f) = " << feof(f) << std::endl;
        //std::cout << "File Pos When Reading: " << static_cast<size_t>(pos) << std::endl;
        printf("File Pos When Reading: %llu\n", static_cast<unsigned long long int>(BDL_POS(pos)));
        fgetpos(f, &pos);
        std::cout << "error reading data file: nRead=" << nRead << " but needed: " << patternDataByteCount << std::endl;
        printf(" Current File Position: %llu\n", static_cast<unsigned long long int>(BDL_POS(pos)));
        std::cout << "X,Y Position from Pattern Header: " << patternHeader.xIndex << " , " << patternHeader.yIndex << std::endl;
        y = yEnd;
        x = xEnd;
        break;
      }
    }
    if(patternDataPtr.get() == nullptr)
    {
      break;
    }
  }
  fclose(f);


  EbsdPatterns::Pointer patterns = EbsdPatterns::New();
  patterns->setImageWidth(descHeader.width);
  patterns->setImageHeight(descHeader.height);
  patterns->setPatternWidth(patternHeader.width);
  patterns->setPatternHeight(patternHeader.height);
  patterns->setPatternData(patternDataPtr);
  patterns->setIsValid(patternDataPtr.get() != nullptr);

  return patterns;
}

#endif

// -----------------------------------------------------------------------------
int BrukerDataLoader::LoadIndexingResults(const std::string& descFile, const std::string& dataFile, const UInt16Array::Pointer& positions, const FloatArray::Pointer& eulers,
                                          const FloatArray::Pointer& patQual, const UInt16Array::Pointer& detectedBands, const Int16Array::Pointer& phase,
                                          const UInt16Array::Pointer& indexedBands, const FloatArray::Pointer& bmm, int32_t& mapWidth, int32_t& mapHeight, std::vector<uint16_t>& roi,
                                          bool reorder)
{

  fs::path descFileInfo(descFile);
  if(!fs::exists(descFileInfo))
  {
    std::cout << "The FrameDescription File does not exist: '" << descFile << "'";
    return -1000;
  }

  fs::path dataFileInfo(dataFile);
  if(!fs::exists(dataFileInfo))
  {
    std::cout << "The Indexing Result File does not exist: '" << dataFile << "'";
    return -1001;
  }

  FILE* desc = fopen(descFile.c_str(), "rb");
  if (nullptr == desc)
  {
    std::cout << "Could not open the FrameDescription File";
    return -1003;
  }

  // Get the total number of scan points from the FrameData file
  FrameDescriptionHeader_t descHeader;
  size_t nRead = fread(&descHeader, sizeof(int32_t), 3, desc);
  if (nRead != 3)
  {
    std::cout << "Could not read the header values. Only " << nRead  << " values were parsed";
    fclose(desc);
    return -1004;
  }
  fclose(desc);
  desc = nullptr;

  mapWidth = descHeader.width;
  mapHeight = descHeader.height;

  // Now get all the array pointers. The arrays were pre-allocated already
  uint16_t* posPtr = positions->getPointer(0);
  positions->initializeWithZeros();

  float* euPtr = eulers->getPointer(0);
  eulers->initializeWithZeros();

  float* pq = patQual->getPointer(0);
  patQual->initializeWithZeros();

  uint16_t* detBnds = detectedBands->getPointer(0);
  detectedBands->initializeWithZeros();

  int16_t* ph = phase->getPointer(0);
  phase->initializeWithZeros();

  uint16_t* idxBnds = indexedBands->getPointer(0);
  indexedBands->initializeWithZeros();

  float* bmmPtr = bmm->getPointer(0);
  bmm->initializeWithZeros();

  int64_t filesize = static_cast<int64_t>(fs::file_size(dataFileInfo));

  FILE* f = fopen(dataFile.c_str(), "rb");
  if(nullptr == f)
  {
    std::cout << "Could not open Indexing Results file at " << dataFile;
    return -1005;
  }

  nRead = 0;

  uint8_t data[30];
  IndexResult_t* record = reinterpret_cast<IndexResult_t*>(data);
  size_t index = 0;
  uint16_t minX = std::numeric_limits<uint16_t>::max();
  uint16_t maxX = std::numeric_limits<uint16_t>::min();
  uint16_t minY = std::numeric_limits<uint16_t>::max();
  uint16_t maxY = std::numeric_limits<uint16_t>::min();
  size_t scannedPointCount = 0;
  fpos_t pos;
  fgetpos(f, &pos);
#if defined(Q_OS_LINUX)
  while(pos.__pos < filesize)
#else
  while(pos < filesize)
#endif
  {
    ::memset(data, 0, 30); // Splat zeros across the structure.
    nRead = fread(&data, 1, 30, f);
    if(feof(f) != 0)
    {
      std::cout << "BrukerDataLoader: "
                << "Unexpected End of File (EOF) was encountered. Details follow" << std::endl;
      std::cout << "  ferror(f) = " << ferror(f) << "  feof(f) = " << feof(f) << std::endl;
      // std::cout << "File Pos When Reading: " << static_cast<size_t>(pos) << std::endl;
      printf("  File Pos When Reading: %llu\n", static_cast<unsigned long long int>(BDL_POS(pos)));
      fgetpos(f, &pos);
      printf("  Current File Position: %llu\n", static_cast<unsigned long long int>(BDL_POS(pos)));
      break;
    }
    scannedPointCount++;
    // Calculate the proper index to place the data as the data is out of order in the file, but only if requested. FALSE by default
    if(reorder)
    {
      index = (mapWidth * record->yIndex) + record->xIndex;
    }

    if(record->xIndex > maxX)
    {
      maxX = record->xIndex;
    }
    if(record->xIndex < minX)
    {
      minX = record->xIndex;
    }

    if(record->yIndex > maxY)
    {
      maxY = record->yIndex;
    }
    if(record->yIndex < minY)
    {
      minY = record->yIndex;
    }

    posPtr[index * 2] = record->xIndex;
    posPtr[index * 2 + 1] = record->yIndex;

    euPtr[index * 3] = (M_PI - record->euler3);
    euPtr[index * 3 + 1] = record->euler2;
    euPtr[index * 3 + 2] = (M_PI - record->euler1);

    pq[index] = record->radonQuality;
    detBnds[index] = record->detectedBands;
    ph[index] = record->phase;
    idxBnds[index] = record->indexedBands;
    bmmPtr[index] = record->bmm;
    index++;
    fgetpos(f, &pos);

    //      if((y != record->yIndex || x != record->xIndex) && debug)
    //      {
    //        debug = false;
    //        std::cout << "Index Value(s) Do(es) not match: " << x << "," << y << " vs " << record->xIndex << "," << record->yIndex << std::endl;
    //        std::cout << "Data: " << record->xIndex << "\t" << record->yIndex << "\t" << record->euler1 << "\t" << record->euler2 << "\t" << record->euler3 << "\t" << record->radonQuality << "\t"
    //                  << record->detectedBands << "\t" << record->phase << "\t" << record->indexedBands << "\t" << record->bmm << std::endl;
    //      }
  }

  roi.resize(4);
  roi[0] = minX;
  roi[1] = minY;
  roi[2] = maxX;
  roi[3] = maxY;
  std::cout << "ROI: (" << minX << ", " << minY << ") -> (" << maxX << ", " << maxY << ")" << std::endl;
  std::cout << "Total Measured Points: " << scannedPointCount << std::endl;
  fclose(f);
  return 0;
}

// -----------------------------------------------------------------------------
void parseIntValue(const std::string& line, int& value)
{
  auto tokens = complex::StringUtilities::split_2(line, '=');
  value = stoi(tokens[1]);
}


// -----------------------------------------------------------------------------
int BrukerDataLoader::ReadScanSizes(const std::string& path, int32_t& mapWidth, int32_t& mapHeight,
                                    int32_t& ebspWidth, int32_t& ebspHeight)
{
  /* The file contents should be the following:
   * AcquisitionStep=1
    SEMImgWidth=512
    SEMImgHeight=384
    MapWidth=512
    MapHeight=384
    EBSPWidth=80
    EBSPHeight=60
    ChannelNameCount=1
    ChannelName0=SE
    MeasurementDuration=-1
    MaxRadonBandCount=12
  */

  std::string filePath = path + "/" + Bruker::Files::Auxiliarien;
  if(!fs::exists(filePath))
  {
    return -1;
  }

  std::string contents;
  {
    std::ifstream source(filePath);

    source.seekg(0, std::ios::end);
    contents.reserve(source.tellg());
    source.seekg(0, std::ios::beg);

    contents.assign((std::istreambuf_iterator<char>(source)), std::istreambuf_iterator<char>());
  }

  auto list = complex::StringUtilities::split_2(contents,  '\n');
  for(const auto& line : list)
  {
    if(complex::StringUtilities::contains(line, "MapWidth"))
    {
      parseIntValue(line, mapWidth);
    }
    else if(complex::StringUtilities::contains(line,"MapHeight"))
    {
      parseIntValue(line, mapHeight);
    }
    else if(complex::StringUtilities::contains(line,"EBSPWidth"))
    {
      parseIntValue(line, ebspWidth);
    }
    else if(complex::StringUtilities::contains(line,"EBSPHeight"))
    {
      parseIntValue(line, ebspHeight);
    }
  }

  return 1;
}



#if 0
// -----------------------------------------------------------------------------
std::string BrukerDataLoader::VerifyRequiredFiles(const std::string& path)
{
  std::string errString;

  std::stringList files;
  files << Bruker::Files::Auxiliarien << Bruker::Files::AuxIndexingOptions << Bruker::Files::Calibration
        << Bruker::Files::CameraConfiguration << Bruker::Files::CameraLifeConfig << Bruker::Files::FrameData
        << Bruker::Files::FrameDescription << Bruker::Files::ImgPreparation << Bruker::Files::IndexingResults
        << Bruker::Files::Miszellaneen << Bruker::Files::PhaseList << Bruker::Files::RadonLines
        << Bruker::Files::SEMImage << Bruker::Files::ShownPhaseList << Bruker::Files::Version;


  std::stringListIterator iter(files);
  while(iter.hasNext())
  {
    std::string file = iter.next();
    QFileInfo fi(path + "/" + file);
    if(!fi.exists())
    {
      std::string str = std::string("File path does not exist: '%1'\n").arg(fi.absoluteFilePath());
      errString.append(str);
    }
  }
  return errString;
}
#endif

#if 0
// -----------------------------------------------------------------------------
int BrukerDataLoader::ReadCrystalSymmetries(const std::string& path, const UInt32Array::Pointer& xtal)
{

  std::string filePath = path + "/" + Bruker::Files::PhaseList;


  QFileInfo fi(filePath);
  if(!fi.exists())
  {
    std::cout << "BrukerDataLoader::ReadPhaseInformation Input file does not exist" << std::endl;
    return -1000;
  }

  QFile xmlFile(filePath);

  QIODevice* device = &xmlFile;

  QDomDocument domDocument;
  std::string errorStr;
  int errorLine;
  int errorColumn;
  if (!domDocument.setContent(device, true, &errorStr, &errorLine, &errorColumn))
  {
    std::string errormessage("Parse error at line %1, column %2:\n%3");
    errormessage  = errormessage.arg(errorLine).arg(errorColumn).arg(errorStr);
    return 0;
  }
  QDomElement root = domDocument.documentElement();
  std::string nodeName = root.nodeName();
  QDomNodeList nodeList = root.elementsByTagName("ClassInstance");
  for(int i = 0; i < nodeList.size(); ++i)
  {
    QDomElement e = nodeList.at(i).toElement();
    std::string eType = e.attribute("Type");
    if (nodeName.compare(eType) == 0)
    {
      root = e;
      break;
    }
  }

  TEBSDExtPhaseEntryList topLevel;
  topLevel.parse(root);

  QVector<TEBSDExtPhaseEntry::Pointer> phases = topLevel.getTEBSDExtPhaseEntry_vec();

  xtal->resizeTuples(phases.size() + 1);
  xtal->setValue(0, EbsdLib::CrystalStructure::UnknownCrystalStructure);

  for(size_t i = 0; i < phases.size(); i++)
  {
    TEBSDPhaseEntry::Pointer phase = phases[i]->getTEBSDPhaseEntry();
    //    std::cout << "Phase List";
    //    std::cout << phase->getCC();
    //    std::cout << phase->getSG();
    // This could be put into the "Material Name" array
    //    std::cout << phase->getChem();
    Cell::Pointer cell = phase->getCell();
    // These could be put into the Lattice Constants Array
    //    std::cout << cell->getAngles();
    //    std::cout << cell->getDim();

    if(phase->getCC().compare("CUB") == 0)
    {
      xtal->setValue(i + 1, EbsdLib::CrystalStructure::Cubic_High); // This is a TOTAL GUESS!!!!
    }
    else
    {
      xtal->setValue(i + 1, EbsdLib::CrystalStructure::UnknownCrystalStructure); // This is a TOTAL GUESS!!!!
    }
  }

  return 0;
}
#endif

// -----------------------------------------------------------------------------
BrukerDataLoader::Pointer BrukerDataLoader::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}

// -----------------------------------------------------------------------------
BrukerDataLoader::Pointer BrukerDataLoader::New()
{
  Pointer sharedPtr(new(BrukerDataLoader));
  return sharedPtr;
}

// -----------------------------------------------------------------------------
std::string BrukerDataLoader::getNameOfClass() const
{
  return std::string("BrukerDataLoader");
}

// -----------------------------------------------------------------------------
std::string BrukerDataLoader::ClassName()
{
  return std::string("BrukerDataLoader");
}
