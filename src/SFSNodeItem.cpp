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
#include "SFSNodeItem.h"

#if(_MSC_VER >= 1)
#pragma warning(error : 4715) /* Not all control points return a value */
#pragma warning(error : 4258) /* Nested Variable Definitions are ignored */
                              //#ifdef SIMPLib_DISABLE_MSVC_WARNINGS
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#pragma warning(disable : 4305)
//#endif
#endif

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "SFSReader.h"
#include "SFSUtils.hpp"

// -----------------------------------------------------------------------------
SFSNodeItem::SFSNodeItem(const uint8_t* ptr, FILE* fin, SFSReader* reader)
: m_Reader(reader)
{
  ::memcpy(&m_PointerTableInit, ptr, 4);
  ::memcpy(&m_FileSize, ptr + 4, 8);
  ::memcpy(&m_FileCreationTime, ptr + 12, 8);
  ::memcpy(&m_FileModificationTime, ptr + 20, 8);
  ::memcpy(&m_LastAccessTime, ptr + 28, 8);
  ::memcpy(&m_Permissions, ptr + 36, 4);
  ::memcpy(&m_ParentItemIndex, ptr + 40, 4);

  m_String176 = std::string(reinterpret_cast<const char*>(ptr + 44), 176);
  ::memcpy(&m_Directory, ptr + 220, 1);
  m_String3 = std::string(reinterpret_cast<const char*>(ptr + 221));
  m_FileName = std::string(reinterpret_cast<const char*>(ptr + 224));
  m_String32 = std::string(reinterpret_cast<const char*>(ptr + 480));

  m_ChunkCount = getPointerTableEntryCount();

  if(!m_Directory)
  {
    generateFilePointerTable(fin);
  }
  m_IsValid = true;
}

// -----------------------------------------------------------------------------
SFSNodeItem::~SFSNodeItem() = default;

// -----------------------------------------------------------------------------
int32_t SFSNodeItem::getPointerTableEntryCount() const
{
  int32_t chunkCount = static_cast<int32_t>(std::ceil(static_cast<float>(m_FileSize) / static_cast<float>(m_Reader->getUsableChunkSize())));
  return chunkCount;
}

#if 0
namespace neolib
{
template <class Elem, class Traits>
inline void hex_dump(const void* aData, std::size_t aLength, std::basic_ostream<Elem, Traits>& aStream, std::size_t aWidth = 16)
{
  const char* const start = static_cast<const char*>(aData);
  const char* const end = start + aLength;
  const char* line = start;
  while(line != end)
  {
    aStream.width(4);
    aStream.fill('0');
    aStream << std::hex << line - start << " : ";
    std::size_t lineLength = std::min(aWidth, static_cast<std::size_t>(end - line));
    for(std::size_t pass = 1; pass <= 2; ++pass)
    {
      for(const char* next = line; next != end && next != line + aWidth; ++next)
      {
        char ch = *next;
        switch(pass)
        {
        case 1:
          aStream << (ch < 32 ? '.' : ch);
          break;
        case 2:
          if(next != line)
            aStream << " ";
          aStream.width(2);
          aStream.fill('0');
          aStream << std::hex << std::uppercase << static_cast<int>(static_cast<unsigned char>(ch));
          break;
        }
      }
      if(pass == 1 && lineLength != aWidth)
        aStream << std::string(aWidth - lineLength, ' ');
      aStream << " ";
    }
    aStream << std::endl;
    line = line + lineLength;
    aStream << std::dec;
  }
}
} // namespace neolib
#endif

// -----------------------------------------------------------------------------
void SFSNodeItem::generateFilePointerTable(FILE* fin)
{
  //  bool debug = false;
  // m_FileName == "FrameData";
  //  if(debug)
  //    std::cout << "=============================================\n  " << m_FileName << std::endl;

  size_t readerChunkSize = static_cast<size_t>(m_Reader->getChunkSize());
  size_t readerUsableChunkSize = static_cast<size_t>(m_Reader->getUsableChunkSize());

  int32_t err = 0;
  float denom = std::floor(readerUsableChunkSize / 4.0f);
  float beforeCeil = m_ChunkCount / denom;
  auto chunkCount = static_cast<int32_t>(::ceil(beforeCeil));
  std::vector<uint8_t> buffer(chunkCount * readerUsableChunkSize, 0x00); // Allocate a complete buffer to read the table
  uint8_t* bufferPtr = buffer.data();
  auto ui32Ptr = reinterpret_cast<uint32_t*>(buffer.data());
  size_t totalBytesRead = 0;
  if(chunkCount > 1)
  {
    uint32_t nextChunk = m_PointerTableInit;
    for(int i = 0; i < chunkCount; i++)
    {
      size_t offset = readerChunkSize * nextChunk + 280;
      SFS_UTIL_FSEEK(fin, offset, SEEK_SET);
      nextChunk = SFSUtils::readScalar<uint32_t>(fin, err);
      SFS_UTIL_FSEEK(fin, 28, SEEK_CUR);

      if(fread(bufferPtr, 1, readerUsableChunkSize, fin) != readerUsableChunkSize)
      {
        m_IsValid = false;
        return;
      }
      bufferPtr += readerUsableChunkSize - 1;
      totalBytesRead += readerUsableChunkSize;
      //      if(debug)
      //      {
      //        std::cout << "  " << i << "  |  File Pos Start: " << offset + 32 << "\t"
      //                  << "File Pos End: " << offset + 32 + readerUsableChunkSize << "\t"
      //                  << "Num. UInt32s: " << totalBytesRead / 4 << std::endl;

      //        if(totalBytesRead / 4 > 230687)
      //        {
      //          std::cout << ui32Ptr[230687] * readerChunkSize + 312ULL << "\t" << ui32Ptr[230688] * readerChunkSize + 312ULL << std::endl;

      //          neolib::hex_dump(ui32Ptr + 230687, 8, std::cout);
      //        }
      //      }
    }
  }
  else
  {
    size_t offset = readerChunkSize * m_PointerTableInit + 312ULL;
    SFS_UTIL_FSEEK(fin, offset, SEEK_SET);
    if(fread(bufferPtr, 1, readerUsableChunkSize, fin) != readerUsableChunkSize)
    {
      m_IsValid = false;
      return;
    }
  }
  bufferPtr = buffer.data();
  m_FilePointerTable.resize(m_ChunkCount);
  //  size_t maxIndex = buffer.size() / 4;
  //  if(maxIndex < m_ChunkCount)
  //  {
  //    std::cout << "We will walk off the end of the array." << std::endl;
  //  }
  //  bool messedUp = false;
  for(size_t i = 0; i < m_ChunkCount; i++)
  {

    auto value = static_cast<size_t>(ui32Ptr[i]);
    m_FilePointerTable[i] = value * static_cast<size_t>(readerChunkSize) + 312ULL;
    //    if(m_FilePointerTable[i] > 18661284985 && !messedUp)
    //    {
    //      std::cout << "===========================" << std::endl;
    //      messedUp = true;
    //    }
    //    if(debug)
    //    {
    //      std::cout << "    m_FilePointerTable[" << i << "] = " << m_FilePointerTable[i] << std::endl;
    //    }
  }
}

// -----------------------------------------------------------------------------
std::vector<uint8_t> SFSNodeItem::extractFile() const
{
  std::vector<uint8_t> data(m_FileSize, 0);
  if(m_FileSize == 0)
  {
    return data;
  }
  if(m_Directory)
  {
    return data;
  }
  const std::string& inputFilePath = m_Reader->getInputFile();
  FILE* fn = fopen(inputFilePath.c_str(), "rb");
  if(nullptr == fn)
  {
    return data;
  }

  if(m_ChunkCount == 1)
  {
    SFS_UTIL_FSEEK(fn, m_FilePointerTable[0], SEEK_SET);
    if(fread(data.data(), 1, m_FileSize, fn) != m_FileSize)
    {
      return data;
    }
  }
  else
  {
    size_t dataSize = 0;
    size_t chunkSize = static_cast<size_t>(m_Reader->getUsableChunkSize());
    uint8_t* destPtr = data.data();
    for(size_t i = 0; i < m_ChunkCount; i++)
    {
      SFS_UTIL_FSEEK(fn, m_FilePointerTable[i], SEEK_SET);
      if(fread(destPtr, 1, chunkSize, fn) != chunkSize)
      {
        data.assign(m_FileSize, 0);
        return data;
      }
      destPtr += chunkSize;
      dataSize += chunkSize;
      if(m_FileSize - dataSize < chunkSize)
      {
        chunkSize = m_FileSize - dataSize;
      }
    }
  }
  fclose(fn);
  return data;
}

// -----------------------------------------------------------------------------
int32_t SFSNodeItem::writeFile(const std::string& outputfile) const
{
  if(m_FileSize == 0)
  {
    return -1;
  }
  if(m_Directory)
  {
    return -2;
  }

  FILE* out = fopen(outputfile.c_str(), "wb");
  if(out == nullptr)
  {
    return -3;
  }

  const std::string& inputFilePath = m_Reader->getInputFile();
  FILE* fn = fopen(inputFilePath.c_str(), "rb");
  if(nullptr == fn)
  {
    fclose(out);
    return -4;
  }
  if(m_ChunkCount == 1)
  {
    std::vector<uint8_t> data(m_FileSize, 0);
    SFS_UTIL_FSEEK(fn, m_FilePointerTable[0], SEEK_SET);
    if(fread(data.data(), 1, m_FileSize, fn) != m_FileSize)
    {
      fclose(out);
      return -5;
    }
    fwrite(data.data(), m_FileSize, 1, out);
    std::cout << m_FileName << std::endl;
  }
  else
  {
    uint64_t dataSize = 0;
    uint64_t chunkSize = static_cast<uint64_t>(m_Reader->getUsableChunkSize());
    std::vector<uint8_t> data(chunkSize, 0);
    uint8_t* destPtr = data.data();

    float progress = 0.0f;
    float currentProgress = 0.0f;

    // std::cout << m_FileName << std::endl;
    for(size_t i = 0; i < m_ChunkCount; i++)
    {
      currentProgress = static_cast<float>(i) / static_cast<float>(m_ChunkCount);
      if(currentProgress > progress)
      {
        progress = progress + 0.01f;
        std::cout << m_FileName << " " << m_FileSize << " [" << static_cast<int>(currentProgress * 100.0f) << "%]\r";
        std::cout.flush();
      }
      size_t filePointer = m_FilePointerTable[i];
      SFS_UTIL_FSEEK(fn, filePointer, SEEK_SET);
      size_t numBytesRead = fread(destPtr, 1, chunkSize, fn);
      if(numBytesRead != chunkSize)
      {
        std::cout << "Not Enough Bytes Read: " << m_FilePointerTable[i] << " Needed " << chunkSize << " Got " << numBytesRead << std::endl;
      }
      fwrite(destPtr, chunkSize, 1, out);
      dataSize += chunkSize;
      if(m_FileSize - dataSize < chunkSize)
      {
        chunkSize = m_FileSize - dataSize;
      }
    }
    std::cout << std::endl;
  }

  fclose(out);
  fclose(fn);
  return 0;
}

// -----------------------------------------------------------------------------
void SFSNodeItem::setParentNode(SFSNodeItem* parent)
{
  m_ParentObject = parent;
}

// -----------------------------------------------------------------------------
SFSNodeItem* SFSNodeItem::getParentNode()
{
  return m_ParentObject;
}

// -----------------------------------------------------------------------------
void SFSNodeItem::addChildNode(const SFSNodeItem::Pointer& child)
{
  m_Children[child->getFileName()] = child;
  child->setParentNode(this);
}

// -----------------------------------------------------------------------------
void SFSNodeItem::removeChildNode(const std::string& name)
{
  if(m_Children.find(name) != m_Children.end())
  {
    SFSNodeItem::Pointer item = m_Children[name];
    item->setParentNode(nullptr);
    m_Children.erase(name);
  }
}

// -----------------------------------------------------------------------------
size_t SFSNodeItem::childCount()
{
  return m_Children.size();
}

// -----------------------------------------------------------------------------
void SFSNodeItem::clearChildren()
{

  for(const auto& child : m_Children)
  {
    child.second->setParentNode(nullptr);
  }

  m_Children.clear();
}

// -----------------------------------------------------------------------------
SFSNodeItem::Pointer SFSNodeItem::child(const std::string& name)
{
  if(m_Children.find(name) != m_Children.end())
  {
    return m_Children[name];
  }
  return SFSNodeItem::Pointer(nullptr);
}

// -----------------------------------------------------------------------------
std::map<std::string, SFSNodeItem::Pointer>& SFSNodeItem::children()
{
  return m_Children;
}

// -----------------------------------------------------------------------------
int32_t SFSNodeItem::getPointerTableInit() const
{
  return m_PointerTableInit;
}

// -----------------------------------------------------------------------------
uint64_t SFSNodeItem::getFileSize() const
{
  return m_FileSize;
}

// -----------------------------------------------------------------------------
uint64_t SFSNodeItem::getFileCreationTime() const
{
  return m_FileCreationTime;
}

// -----------------------------------------------------------------------------
uint64_t SFSNodeItem::getFileModificationTime() const
{
  return m_FileModificationTime;
}

// -----------------------------------------------------------------------------
uint64_t SFSNodeItem::getFileLastAccessTime() const
{
  return m_LastAccessTime;
}

// -----------------------------------------------------------------------------
uint32_t SFSNodeItem::getPermissions() const
{
  return m_Permissions;
}

// -----------------------------------------------------------------------------
int32_t SFSNodeItem::getParentItemIndex() const
{
  return m_ParentItemIndex;
}

// -----------------------------------------------------------------------------
bool SFSNodeItem::isDirectory() const
{
  return m_Directory;
}

// -----------------------------------------------------------------------------
std::string SFSNodeItem::getFileName() const
{
  return m_FileName;
}

bool SFSNodeItem::getIsValid() const
{
  return m_IsValid;
}

// -----------------------------------------------------------------------------
void SFSNodeItem::debug(std::ostream& out) const
{
  out << "--------------------------------------------------------------------" << std::endl;
  out << "m_FileName: " << m_FileName << std::endl;
  out << "m_PointerTableInit: " << m_PointerTableInit << std::endl;
  out << "m_FileSize: " << m_FileSize << std::endl;
  out << "m_ChunkCount: " << m_ChunkCount << std::endl;
  out << "m_FileCreationTime: " << m_FileCreationTime << std::endl;
  out << "m_FileModificationTime: " << m_FileModificationTime << std::endl;
  out << "m_LastAccessTime: " << m_LastAccessTime << std::endl;
  out << "m_Permissions: " << m_Permissions << std::endl;
  out << "m_ParentItemIndex: " << m_ParentItemIndex << std::endl;
  out << "m_String176: " << m_String176 << std::endl;
  out << "m_Directory: " << m_Directory << std::endl;
  out << "m_String3: " << m_String3 << std::endl;
  out << "m_String32: " << m_String32 << std::endl;
}

// -----------------------------------------------------------------------------
void SFSNodeItem::printTree(std::ostream& out, int32_t level) const
{
  std::string indent(level, ' ');
  out << indent << m_FileName << std::endl;
  for(const auto& item : m_Children)
  {
    if(item.second->isDirectory())
    {
      item.second->printTree(out, level + 2);
    }
    else
    {
      indent = std::string(level + 4, ' ');
      out << indent << item.second->getFileName() << ": " << item.second->getFileSize() << std::endl;
    }
  }
}
