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
#include "SFSReader.h"

#if(_MSC_VER >= 1)
#pragma warning(error : 4715) /* Not all control points return a value */
#pragma warning(error : 4258) /* Nested Variable Definitions are ignored */
                              //#ifdef SIMPLib_DISABLE_MSVC_WARNINGS
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#pragma warning(disable : 4305)
//#endif
#endif

#include <sys/stat.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "SFSNodeItem.h"
#include "SFSUtils.hpp"

namespace BCF
{
const char k_SFSMagic[8] = {'A', 'A', 'M', 'V', 'H', 'F', 'S', 'S'};
}

// -----------------------------------------------------------------------------
SFSReader::SFSReader() = default;

// -----------------------------------------------------------------------------
SFSReader::~SFSReader() = default;

// -----------------------------------------------------------------------------
float SFSReader::getVersion() const
{
  return m_Version;
}

// -----------------------------------------------------------------------------
uint32_t SFSReader::getChunkSize() const
{
  return m_ChunkSize;
}

// -----------------------------------------------------------------------------
uint32_t SFSReader::getUsableChunkSize() const
{
  return m_UsableChunkSize;
}

const std::string& SFSReader::getInputFile() const
{
  return m_FilePath;
}

// -----------------------------------------------------------------------------
SFSNodeItemPtr SFSReader::getRootNode() const
{
  return m_RootNode;
}

// -----------------------------------------------------------------------------
int SFSReader::parseFile(const std::string& filepath)
{
  m_FilePath = filepath;

  int32_t err = 0;
  FILE* fin = fopen(m_FilePath.c_str(), "rb");
  if(nullptr == fin)
  {
    std::cout << "Error opening file '" << filepath << "'" << std::endl;
    return -2;
  }

  std::array<uint8_t, 8> sfsmagic = {0, 0, 0, 0, 0, 0, 0, 0};

  size_t nread = fread(sfsmagic.data(), 1, 8, fin);
  if(nread != 8)
  {
    fclose(fin);
    return -3;
  }

  for(size_t c = 0; c < 8; c++)
  {
    if(BCF::k_SFSMagic[c] != sfsmagic[c])
    {
      fclose(fin);
      return -6;
    }
  }

  // Seek to the version number
  SFS_UTIL_FSEEK(fin, 0x124, SEEK_SET);

  m_Version = SFSUtils::readScalar<float>(fin, err);
  if(err == 1)
  {
    fclose(fin);
    std::cout << "Error reading version" << std::endl;
    return -4;
  }
  m_ChunkSize = SFSUtils::readScalar<uint32_t>(fin, err);
  if(err == 1)
  {
    fclose(fin);
    std::cout << "Error reading chunkSize" << std::endl;
    return -5;
  }

  m_UsableChunkSize = m_ChunkSize - 32;
  //  std::cout << "Version: " << m_Version << std::endl;
  //  std::cout << "chunkSize: " << m_ChunkSize << std::endl;
  //  std::cout << "usableChunkSize: " << m_UsableChunkSize << std::endl;

  // Read the SFS Tree Root Structure
  SFS_UTIL_FSEEK(fin, 320, SEEK_SET);
  err = 0;
  m_TreeAddress = SFSUtils::readScalar<uint32_t>(fin, err);
  if(err == -1)
  {
    m_IsValid = false;
  }
  m_NumTreeItems = SFSUtils::readScalar<uint32_t>(fin, err);
  if(err == -1)
  {
    m_IsValid = false;
  }
  m_NumChunks = SFSUtils::readScalar<uint32_t>(fin, err);
  if(err == -1)
  {
    m_IsValid = false;
  }
  //  std::cout << "TreeAddress: " << m_TreeAddress << std::endl;
  //  std::cout << "m_NumTreeItems: " << m_NumTreeItems << std::endl;
  //  std::cout << "m_NumChunks: " << m_NumChunks << std::endl;

  // Create all the headers to convert into SFSNodeItems
  int32_t fileTreeChunks = static_cast<int32_t>(std::ceil((m_NumTreeItems * 512.0f) / (m_ChunkSize - 32.0f)));
  //  std::cout << "fileTreeChunks: " << fileTreeChunks << std::endl;
  std::vector<uint8_t> rawTreeBuffer;
  if(fileTreeChunks == 1)
  {
    // file tree does not exceed one chunk in bcf:
    size_t offset = m_ChunkSize * m_TreeAddress + 312;
    SFS_UTIL_FSEEK(fin, offset, SEEK_SET);
    size_t rawTreeCount = 512 * m_NumTreeItems;
    rawTreeBuffer.resize(rawTreeCount * sizeof(int32_t));

    nread = fread(rawTreeBuffer.data(), 4, rawTreeCount, fin);
    if(nread != rawTreeCount)
    {
      std::cout << "sfsReader::parseFile(" << __LINE__ << ") error reading bytes: " << ferror(fin) << std::endl;
      // std::cout << "sfsReader::parseFile(" << __LINE__ << ") EOF Value: " << feof(fin) << std::endl;
      perror("sfsReader::parseFile Error");
      err = -1;
    }
  }
  else
  {
    uint32_t treeAddress = m_TreeAddress;
    int32_t numTreeItemsInChunk = static_cast<int32_t>(std::floor((m_ChunkSize - 32.0f) / 512.0f));
    size_t bytesToRead = numTreeItemsInChunk * 512;
    rawTreeBuffer.resize(fileTreeChunks * numTreeItemsInChunk * 512); // Allocate all the space for the header
    uint8_t* rawTreeBufferPtr = rawTreeBuffer.data();
    for(int32_t i = 0; i < fileTreeChunks; i++)
    {
      SFS_UTIL_FSEEK(fin, m_ChunkSize * treeAddress + 280, SEEK_SET);
      treeAddress = SFSUtils::readScalar<uint32_t>(fin, err); // strct_unp('<I', fn.read(4))[0]
      SFS_UTIL_FSEEK(fin, 28, SEEK_CUR);
      if(fread(rawTreeBufferPtr, 1, bytesToRead, fin) != bytesToRead)
      {
        m_IsValid = false;
        err = -1;
      }
      rawTreeBufferPtr += bytesToRead;
    }
  }

  // Create SFSNodeItems
  std::map<int32_t, SFSNodeItemPtr> indexNodeMap;
  m_RootNode = std::make_shared<SFSNodeItem>();
  // SFSNodeItemPtr(new SFSNodeItem);

  // Read all the 512 byte headers for each tree item
  for(uint32_t i = 0; i < m_NumTreeItems; i++)
  {
    // std::cout << "---------------  SFSTreeItem  -------------------" << std::endl;
    SFSNodeItemPtr item = std::make_shared<SFSNodeItem>(rawTreeBuffer.data() + (i * 512), fin, this);
    indexNodeMap[i] = item;
    if(item->getParentItemIndex() == -1)
    {
      m_RootNode->addChildNode(item);
    }
    else
    {
      SFSNodeItemPtr parent = indexNodeMap[item->getParentItemIndex()];
      parent->addChildNode(item);
    }
  }

  // m_RootNode->printTree(std::cout, 0);

  fclose(fin);
  fin = nullptr;

  return err;
}

// -----------------------------------------------------------------------------
void saveFile(const std::string& outputDir, const SFSNodeItemPtr& node)
{
  int err = 0;

  std::map<std::string, SFSNodeItemPtr> children = node->children();
  for(const auto& child : children)
  {
    if(child.second->isDirectory())
    {
      std::string path = outputDir + "/" + child.second->getFileName();
      SFSUtils::mkdir(path, true);
      saveFile(path, child.second);
    }
    else
    {
      std::string outputPath = outputDir + "/" + child.second->getFileName();
      std::cout << "Saving File: " << outputPath << std::endl;
      err = child.second->writeFile(outputPath);
    }
  }
}

// -----------------------------------------------------------------------------
void SFSReader::extractAll(const std::string& outputPath) const
{
  SFSUtils::mkdir(outputPath, true);
  saveFile(outputPath, m_RootNode);
}

// -----------------------------------------------------------------------------
std::vector<std::string> split(const std::string& s, char delimiter)
{
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(s);
  while(std::getline(tokenStream, token, delimiter))
  {
    tokens.push_back(token);
  }
  return tokens;
}

// -----------------------------------------------------------------------------
int32_t SFSReader::extractFile(const std::string& outputPath, const std::string& sfsPath) const
{
  SFSUtils::mkdir(outputPath, true);

  std::vector<std::string> tokens = split(sfsPath, '/');

  SFSNodeItemPtr node = m_RootNode;
  for(const auto& token : tokens)
  {
    node = node->child(token);
    if(node.get() == nullptr)
    {
      break;
    }
    if(node->isDirectory())
    {
      std::string path = outputPath + "/" + node->getFileName();
      SFSUtils::mkdir(path, true);
    }
  }

  if(node.get() == nullptr)
  {
    std::cout << "Path does not exist in SFS file. '" << sfsPath << "'" << std::endl;
    return -10;
  }
  std::string fullpath = outputPath + "/" + sfsPath;
  return node->writeFile(fullpath);
}

// -----------------------------------------------------------------------------
bool SFSReader::fileExists(const std::string& sfsPath) const
{
  if(sfsPath.empty())
  {
    return false;
  }
  std::vector<std::string> tokens = split(sfsPath, '/');

  SFSNodeItemPtr node = m_RootNode;
  for(const auto& token : tokens)
  {
    node = node->child(token);
    if(nullptr == node.get())
    {
      return false;
    }
  }

  if(node == m_RootNode)
  {
    return false;
  }
  return node.get() != nullptr;
}
