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
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>


#if defined (_MSC_VER)
#include <direct.h>
#define UNLINK _unlink
#define SFS_UTIL_PATH_MAX MAX_PATH
#define SFS_UTIL_GET_CWD _getcwd
#else
#define UNLINK ::unlink
#include <dirent.h>
#define SFS_UTIL_PATH_MAX PATH_MAX
#define SFS_UTIL_GET_CWD ::getcwd
#endif


#include <sys/stat.h>

#if defined (_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#define SFS_UTIL_STATBUF struct _stati64       // non-ANSI defs
#define SFS_UTIL_STATBUF4TSTAT struct _stati64 // non-ANSI defs
#define SFS_UTIL_STAT _stati64
#define SFS_UTIL_FSTAT _fstati64
#define SFS_UTIL_FSEEK _fseeki64
#define SFS_UTIL_STAT_REG _S_IFREG
#define SFS_UTIL_STAT_DIR _S_IFDIR
#define SFS_UTIL_STAT_MASK _S_IFMT
#if defined(_S_IFLNK)
#define SFS_UTIL_STAT_LNK _S_IFLNK
#endif

#elif defined (__APPLE__)

#define SFS_UTIL_STATBUF struct stat
#define SFS_UTIL_STATBUF4TSTAT struct stat
#define SFS_UTIL_STAT stat
#define SFS_UTIL_FSTAT fstat
#define SFS_UTIL_FSEEK fseek
#define SFS_UTIL_STAT_REG S_IFREG
#define SFS_UTIL_STAT_DIR S_IFDIR
#define SFS_UTIL_STAT_MASK S_IFMT
#define SFS_UTIL_STAT_LNK S_IFLNK

#else
#define SFS_UTIL_STATBUF struct stat
#define SFS_UTIL_STATBUF4TSTAT struct stat
#define SFS_UTIL_STAT stat
#define SFS_UTIL_FSTAT fstat
#define SFS_UTIL_FSEEK fseek
#define SFS_UTIL_STAT_REG S_IFREG
#define SFS_UTIL_STAT_DIR S_IFDIR
#define SFS_UTIL_STAT_MASK S_IFMT
#define SFS_UTIL_STAT_LNK S_IFLNK
#endif


/**
 * @brief The SFSUtils class
 */
class SFSUtils 
{

  public:


    template <typename T>
    static T readScalar(FILE *f, int32_t &err)
    {
      T value = 0x00;
      size_t nread = fread(&value, sizeof(T), 1, f);
      if(nread != 1)
      {
        err = -1;
      }
      return value;
    }


    static void readArray(FILE* f, size_t byteCount, void* data, int32_t &err)
    {
      size_t nread = fread(data, 1, byteCount, f);
      if(nread != byteCount)
      {
        std::cout << "readArray(" << __LINE__ << ") error reading bytes: " << ferror(f) << std::endl;
        std::cout << "readArray(" << __LINE__ << ") EOF Value: " << feof(f) << std::endl;
        perror("SFSUtils::readArray Error");
        err = -1;
      }
    }


#if defined (WIN32)
    static  const char Separator = '\\';
#else
    static  const char Separator = '/';
#endif
    static  const char UnixSeparator = '/';
    static  const char Dot = '.';

    // -----------------------------------------------------------------------------
    static std::string fromNativeSeparators(const std::string  &fsPath)
    {
      std::string path(fsPath);
#if defined (WIN32)
      for (int i=0; i<(int)path.length(); i++) {
        if (path[i] ==  Separator )
          path[i] =  UnixSeparator;
      }
#endif
      return path;
    }

    // -----------------------------------------------------------------------------
    static std::string toNativeSeparators(const std::string &fsPath)
    {
      std::string path(fsPath);
#if defined (WIN32)
      for (int i=0; i<(int)path.length(); i++) {
        if (path[i] ==  UnixSeparator )
          path[i] =  Separator;
      }
#endif
      return path;
    }

#if defined (WIN32)
  // -----------------------------------------------------------------------------
  static bool isDirPath(const std::string &folderPath, bool *existed)
  {
      std::string fsPath = folderPath;
      if (fsPath.length() == 2 &&fsPath.at(1) == ':')
          fsPath += Separator;

      DWORD fileAttrib = INVALID_FILE_ATTRIBUTES;
      fileAttrib = ::GetFileAttributesA(fsPath.c_str() );

      if (existed)
          *existed = fileAttrib != INVALID_FILE_ATTRIBUTES;

      if (fileAttrib == INVALID_FILE_ATTRIBUTES)
          return false;

      return (fileAttrib & FILE_ATTRIBUTE_DIRECTORY) ? true : false;
  }
#endif




    // -----------------------------------------------------------------------------
    static bool exists(const std::string &fsPath)
    {
      int error;
      std::string dirName(fsPath);
      // Both windows and OS X both don't like trailing slashes so just get rid of them
      // for all Operating Systems.
      if (dirName[dirName.length() - 1] == Separator) {
        dirName = dirName.substr(0, dirName.length() - 1);
      }
      SFS_UTIL_STATBUF st;
      error = SFS_UTIL_STAT(dirName.c_str(), &st);
      return (error == 0);
    }


    // -----------------------------------------------------------------------------
    static std::string cleanPath(const std::string &fsPath)
    {
      if (fsPath.length() == 0)
      {return fsPath;}
      std::string path(fsPath);
      char slash = '/';
      char dot = '.';
      if (Separator != UnixSeparator)
      {
        path = fromNativeSeparators(path);
      }

      // Peel off any trailing slash
      if (path[path.length() -1 ] == slash)
      {
        path = path.substr(0, path.length() -1);
      }

      std::vector<std::string> stk;
      std::string::size_type pos = 0;
      std::string::size_type pos1 = 0;

      pos = path.find_first_of(slash, pos);
      pos1 = path.find_first_of(slash, pos + 1);
#if defined (WIN32)
      // Check for UNC style paths first
      if (pos == 0 && pos1 == 1)
      {
        pos1 = path.find_first_of(slash, pos1 + 1);
      } else
#endif
        if (pos != 0)
        {
          stk.push_back(path.substr(0, pos));
        }
      // check for a top level Unix Path:
      if (pos == 0 && pos1 == std::string::npos)
      {
        stk.push_back(path);
      }


      while (pos1 != std::string::npos)
      {
        if (pos1 - pos == 3 && path[pos+1] == dot && path[pos+2] == dot)
        {
          //  std::cout << "Popping back element" << std::endl;
          if (!stk.empty())
          {
            stk.pop_back();
          }
        }
        else if (pos1 - pos == 2 && path[pos+1] == dot )
        {

        }
        else if (pos + 1 == pos1) {

        }
        else {
          stk.push_back(path.substr(pos, pos1-pos));
        }
        pos = pos1;
        pos1 = path.find_first_of(slash, pos + 1);
        if (pos1 == std::string::npos)
        {
          stk.push_back(path.substr(pos, path.length() - pos));
        }
      }
      std::string ret;
      for(const auto& str : stk)
      {
        ret.append(str);
      }
      ret = toNativeSeparators(ret);
#if defined (WIN32)
      if (ret.length() > 2
          && isalpha(ret[0]) != 0
          && islower(ret[0]) != 0
          && ret[1] == ':' && ret[2] == '\\')
      {
        //we have a lower case drive letter which needs to be changed to upper case.
        ret[0] = toupper(ret[0]);
      }
#endif
      return ret;
    }

    // -----------------------------------------------------------------------------
    static bool mkdir(const std::string &name, bool createParentDirectories)
    {
#if defined (WIN32)
      std::string dirName = name;
      if (createParentDirectories) {
        dirName = toNativeSeparators(cleanPath(dirName));
        // We specifically search for / so \ would break it..
        std::string::size_type oldslash = std::string::npos;
        if (dirName[0] == '\\' && dirName[1] == '\\')
        {
          // Don't try to create the root fsPath of a UNC fsPath;
          // CreateDirectory() will just return ERROR_INVALID_NAME.
          for (unsigned int i = 0; i < dirName.size(); ++i) {
            if (dirName[i] != Separator)
            {
              oldslash = i;
              break;
            }
          }
          if (oldslash != std::string::npos) {
            oldslash = dirName.find(Separator, oldslash);
          }
        }
        for (std::string::size_type slash=0; slash != std::string::npos; oldslash = slash) {
          slash = dirName.find(Separator, oldslash+1);
          if (slash == std::string::npos) {
            if(oldslash == dirName.length())
              break;
            slash = dirName.length();
          }
          if (slash != std::string::npos) {
            std::string chunk = dirName.substr(0, slash);
            bool existed = false;
            if (!SFSUtils::isDirPath(chunk, &existed) && !existed)
            {
              if (!::CreateDirectoryA(chunk.c_str(), 0) ) { return false; }
            }
          }
        }
        return true;
      }
      return (!::CreateDirectoryA(dirName.c_str(), 0)  == 0);
#else

      std::string dirName = name;
      if (createParentDirectories)
      {
        dirName = SFSUtils::cleanPath(dirName);
        std::string::size_type slash = 0;
        for (std::string::size_type oldslash = 1; slash != std::string::npos; oldslash = slash)
        {
          slash = dirName.find(Separator, oldslash + 1);
          if (slash == std::string::npos)
          {
            if (oldslash == dirName.length()) {break;}
            slash = dirName.length();
          }
          if (slash != std::string::npos)
          {
            std::string chunk = dirName.substr(0, slash);
            if (!SFSUtils::exists(chunk))
            {
              SFS_UTIL_STATBUF st;
              if(SFS_UTIL_STAT(chunk.c_str(), &st) != -1)
              {
                if ((st.st_mode & S_IFMT) != S_IFDIR)
                {
                  return false;
                }
              }
              else if (::mkdir(chunk.c_str(), 0777) != 0)
              {
                return false;
              }
            }
          }
        }
        return true;
      }
#if defined(__APPLE__)  // Mac X doesn't support trailing /'s
      if (dirName[dirName.length() - 1] == '/')
      {
        dirName = dirName.substr(0, dirName.length() - 1);
      }
#endif
      return (::mkdir(dirName.c_str(), 0777) == 0);
#endif
    }

};
