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
#pragma once

#include <cstdio>
#include <array>
#include <string>
#include <vector>
#include <map>
#include <memory>

class SFSReader;

/**
 * @brief The SFSNodeItem class holds all the information for an individual file within the SFS file
 */
class SFSNodeItem
{

public:
  SFSNodeItem() = default;
  explicit SFSNodeItem(const uint8_t* raw_string, FILE* fin, SFSReader* reader);
  ~SFSNodeItem();

  /**
   *
   */
  using Pointer = std::shared_ptr<SFSNodeItem>;


  SFSNodeItem(const SFSNodeItem&) = default; // Copy Constructor
  SFSNodeItem(SFSNodeItem&&) = default;      // Move Constructor
  SFSNodeItem& operator=(const SFSNodeItem&) = default; // Copy Assignment Not Implemented
  SFSNodeItem& operator=(SFSNodeItem&&) = default;      // Move Assignment Not Implemented

  /**
   * @brief getPointerTableInit
   * @return
   */
  int32_t getPointerTableInit() const;

  /**
   * @brief getFileSize
   * @return
   */
  uint64_t getFileSize() const;

  /**
   * @brief getFileCreationTime
   * @return
   */
  uint64_t getFileCreationTime() const;

  /**
   * @brief getFileModificationTime
   * @return
   */
  uint64_t getFileModificationTime() const;

  /**
   * @brief getFileLastAccessTime
   * @return
   */
  uint64_t getFileLastAccessTime() const;

  /**
   * @brief getPermissions
   * @return
   */
  uint32_t getPermissions() const;

  /**
   * @brief getParentItemIndex
   * @return
   */
  int32_t getParentItemIndex() const;

  /**
   * @brief isDirectory
   * @return
   */
  bool isDirectory() const;

  /**
   * @brief getFileName
   * @return
   */
  std::string getFileName() const;

  bool getIsValid() const;

  /**
   * @brief extractFile
   * @return
   */
  std::vector<uint8_t> extractFile() const;

  /**
   * @brief writeFile
   * @param outputfile
   * @return
   */
  int32_t writeFile(const std::string& outputfile) const;

  /**
   * @brief debug
   * @param out
   */
  void debug(std::ostream& out) const;

  /**
   * @brief printTree
   * @param out
   */
  void printTree(std::ostream& out, int32_t level) const;

  /**
   * @brief setParentNode
   * @param parent
   */
  void setParentNode(SFSNodeItem* parent);

  /**
   * @brief getParentNode
   * @return
   */
  SFSNodeItem* getParentNode();

  /**
   * @brief addChildNode
   * @param child
   */
  void addChildNode(const Pointer &child);

  /**
   * @brief removeChildNode
   * @param name
   */
  void removeChildNode(const std::string& name);

  /**
   * @brief childCount
   * @return
   */
  size_t childCount();

  /**
   * @brief clearChildren
   */
  void clearChildren();

  /**
   * @brief child
   * @param name
   * @return
   */
  Pointer child(const std::string& name);

  /**
   * @brief children
   * @return
   */
  std::map<std::string, Pointer>& children();


protected:
  /**
   * @brief getPointerTableEntryCount
   * @return
   */
  int32_t getPointerTableEntryCount() const;

  /**
   * @brief generateFilePointerTable
   */
  void generateFilePointerTable(FILE* fin);

private:
  bool m_IsValid = false;
  int32_t m_PointerTableInit = 0;
  uint64_t m_FileSize = 0;
  uint64_t m_FileCreationTime = 0;
  uint64_t m_FileModificationTime = 0;
  uint64_t m_LastAccessTime = 0;
  uint32_t m_Permissions = 0;
  int32_t m_ParentItemIndex = 0;
  std::string m_String176;
  bool m_Directory = true;
  std::string m_String3;
  std::string m_FileName = std::string("/");
  std::string m_String32;

  int32_t m_ChunkCount = 0;

  std::vector<size_t> m_FilePointerTable;

  SFSReader* m_Reader = nullptr;
  SFSNodeItem* m_ParentObject = nullptr;
  std::map<std::string, Pointer> m_Children;
};
