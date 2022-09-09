cmake_minimum_required(VERSION 3.19)


project(BCFTools
  VERSION 0.1.0
  DESCRIPTION "SFS Reader"
  HOMEPAGE_URL ""
  LANGUAGES CXX
  )
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


#-------------------------------------------------------------------------------
# unbcf executable
#-------------------------------------------------------------------------------
set(unbcf_sources
  ${BCFTools_SOURCE_DIR}/src/SFSReader.h
  ${BCFTools_SOURCE_DIR}/src/SFSReader.cpp

  ${BCFTools_SOURCE_DIR}/src/SFSNodeItem.h
  ${BCFTools_SOURCE_DIR}/src/SFSNodeItem.cpp

  ${BCFTools_SOURCE_DIR}/src/SFSUtils.hpp
  ${BCFTools_SOURCE_DIR}/src/unbcf.cpp
)

add_executable(unbcf ${unbcf_sources})
