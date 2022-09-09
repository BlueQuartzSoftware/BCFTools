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

#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>

class SFSNodeItem;
using SFSNodeItemPtr = std::shared_ptr<SFSNodeItem>;

/**
 * @brief The SFSReader class
 */
class SFSReader
{
public:
  SFSReader();
  ~SFSReader();

  SFSReader(const SFSReader&) = delete;            // Copy Constructor Not Implemented
  SFSReader(SFSReader&&) = delete;                 // Move Constructor Not Implemented
  SFSReader& operator=(const SFSReader&) = delete; // Copy Assignment Not Implemented
  SFSReader& operator=(SFSReader&&) = delete;      // Move Assignment Not Implemented

  /**
   * @brief openFile
   * @param filepath
   * @return
   */
  int parseFile(const std::string& filepath);

  /**
   * @brief getVersion
   * @return
   */
  float getVersion() const;

  /**
   * @brief getChunkSize
   * @return
   */
  uint32_t getChunkSize() const;

  /**
   * @brief getUsableChunkSize
   * @return
   */
  uint32_t getUsableChunkSize() const;

  /**
   * @brief getInputFile
   * @return
   */
  const std::string& getInputFile() const;

  /**
   * @brief getRootNode
   * @return
   */
  SFSNodeItemPtr getRootNode() const;

  /**
   * @brief extractAll This will extract all files within the SFS file into a designated folder.
   * @param outputPath
   */
  void extractAll(const std::string& outputPath) const;

  /**
   * @brief extractFile This will extract a specific file within the SFS File
   * @param outputPath
   * @param sfsPath
   * @return Error code
   */
  int32_t extractFile(const std::string& outputPath, const std::string& sfsPath) const;

  /**
   * @brief Checks if the given path exists in the BCF archive
   * @param sfsPath The path to check
   * @return
   */
  bool fileExists(const std::string& sfsPath) const;

private:
  std::string m_FilePath;
  bool m_IsValid = false;

  float m_Version = 0.0f;
  uint32_t m_ChunkSize = 0;
  uint32_t m_UsableChunkSize = 0;

  uint32_t m_TreeAddress = 0;
  uint32_t m_NumTreeItems = 0;
  uint32_t m_NumChunks = 0;

  SFSNodeItemPtr m_RootNode;
};
