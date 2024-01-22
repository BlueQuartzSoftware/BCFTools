cmake_minimum_required(VERSION 3.19)


project(BCFTools
  VERSION 0.3.0
  DESCRIPTION "Tools to Decompress and Convert .bcf files"
  HOMEPAGE_URL ""
  LANGUAGES CXX
  )
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BUILD_SHARED_LIBS ON)

# ---------- Setup output Directories -------------------------
if(NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
      ${BCFTools_BINARY_DIR}/Bin
      CACHE PATH
      "Single Directory for all Libraries"
      )
endif()

# --------- Setup the Executable output Directory -------------
if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
      ${BCFTools_BINARY_DIR}/Bin
      CACHE PATH
      "Single Directory for all Executables."
      )
endif()

# --------- Setup the Executable output Directory -------------
if(NOT DEFINED CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
      ${BCFTools_BINARY_DIR}/Bin
      CACHE PATH
      "Single Directory for all static libraries."
      )
endif()



#-------------------------------------------------------------------------------
# PUGIXML Library
#-------------------------------------------------------------------------------
add_subdirectory(${BCFTools_SOURCE_DIR}/3rdparty/pugixml)

#-------------------------------------------------------------------------------
# HDF5 Library
#-------------------------------------------------------------------------------
set(HDF5_BUILD_WITH_INSTALL_NAME ON CACHE BOOL "" FORCE)
set(HDF5_BUILD_CPP_LIB OFF CACHE BOOL "" FORCE)
set(HDF5_BUILD_HL_LIB OFF CACHE BOOL "" FORCE)
set(HDF5_BUILD_HL_TOOLS OFF CACHE BOOL "" FORCE)
set(HDF5_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(HDF5_BUILD_UTILS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(HDF_PACKAGE_NAMESPACE "hdf5::")
set(HDF5_ALLOW_EXTERNAL_SUPPORT ON CACHE BOOL "" FORCE)
set(HDF5_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(${BCFTools_SOURCE_DIR}/3rdparty/hdf5)

#-------------------------------------------------------------------------------
# H5Support Library
#-------------------------------------------------------------------------------
#set(H5Support_INCLUDE_QT_API OFF)
#set(H5Support_INSTALL_HDF5 OFF)
#add_subdirectory(${BCFTools_SOURCE_DIR}/3rdparty/H5Support)


#-------------------------------------------------------------------------------
# unbcf executable
#-------------------------------------------------------------------------------
set(unbcf_sources
  ${BCFTools_SOURCE_DIR}/src/SFSReader.h
  ${BCFTools_SOURCE_DIR}/src/SFSReader.cpp

  ${BCFTools_SOURCE_DIR}/src/SFSNodeItem.h
  ${BCFTools_SOURCE_DIR}/src/SFSNodeItem.cpp

  ${BCFTools_SOURCE_DIR}/src/SFSUtils.hpp

)

add_executable(unbcf ${unbcf_sources} ${BCFTools_SOURCE_DIR}/src/unbcf.cpp)
target_compile_definitions(unbcf PRIVATE "-DBCFTools_VERSION=\"${BCFTools_VERSION}\"")

#-------------------------------------------------------------------------------
# bcftohdf5 executable
#-------------------------------------------------------------------------------
set(bcf2hdf5_sources
    ${BCFTools_SOURCE_DIR}/src/bcf2hdf5.cpp
    ${BCFTools_SOURCE_DIR}/src/BcfHdf5Convertor.h
    ${BCFTools_SOURCE_DIR}/src/BcfHdf5Convertor.cpp

    ${BCFTools_SOURCE_DIR}/src/BrukerIntegration/BrukerIntegrationConstants.h
    ${BCFTools_SOURCE_DIR}/src/BrukerIntegration/BrukerIntegrationStructs.h

    ${BCFTools_SOURCE_DIR}/src/BrukerIntegrationFilters/BrukerDataLoader.h
    ${BCFTools_SOURCE_DIR}/src/BrukerIntegrationFilters/BrukerDataLoader.cpp

    ${BCFTools_SOURCE_DIR}/src/BrukerIntegrationFilters/EbsdPatterns.h
    ${BCFTools_SOURCE_DIR}/src/BrukerIntegrationFilters/EbsdPatterns.cpp

    ${BCFTools_SOURCE_DIR}/src/EbsdLib/Core/EbsdDataArray.hpp
    ${BCFTools_SOURCE_DIR}/src/EbsdLib/Core/EbsdDataArray.cpp
    ${BCFTools_SOURCE_DIR}/src/EbsdLib/Core/EbsdLibConstants.h
    ${BCFTools_SOURCE_DIR}/src/EbsdLib/Core/EbsdMacros.h
)

#add_executable(bcf2hdf5 ${unbcf_sources} ${BCFTools_SOURCE_DIR}/src/unbcf.cpp)

add_executable(bcf2hdf5 ${unbcf_sources} ${bcf2hdf5_sources})
target_link_libraries(bcf2hdf5 hdf5-shared pugixml)
target_include_directories(bcf2hdf5 PUBLIC
                           ${BCFTools_SOURCE_DIR}/src
                           ${BCFTools_SOURCE_DIR}/3rdparty/H5Support/Source
                           ${BCFTools_SOURCE_DIR}/3rdparty/hdf5/src
                           ${BCFTools_BINARY_DIR}/3rdparty/hdf5/src
                           ${BCFTools_SOURCE_DIR}/3rdparty/pugixml/src
                           )
target_compile_definitions(bcf2hdf5 PRIVATE "-DBCFTools_VERSION=\"${BCFTools_VERSION}\"")
