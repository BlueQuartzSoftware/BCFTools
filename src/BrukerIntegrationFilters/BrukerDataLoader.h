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

#pragma once


#include "EbsdPatterns.h"
#include "EbsdLib/Core/EbsdDataArray.hpp"

using namespace EbsdLib;

#include <memory>
#include <string>



class BrukerDataLoader
{
  public:
    using Self = BrukerDataLoader;
    using Pointer = std::shared_ptr<Self>;
    using ConstPointer = std::shared_ptr<const Self>;
    using WeakPointer = std::weak_ptr<Self>;
    using ConstWeakPointer = std::weak_ptr<const Self>;
    static Pointer NullPointer();

    static Pointer New();

    /**
     * @brief Returns the name of the class for BrukerDataLoader
     */
    virtual std::string getNameOfClass() const;
    /**
     * @brief Returns the name of the class for BrukerDataLoader
     */
    static std::string ClassName();

    virtual ~BrukerDataLoader();

    /**
     * @brief loadDataFromFile
     * @param descFileName
     * @param dataFile
     * @return
     */
    static EbsdPatterns::Pointer LoadPatternData(const std::string& descFileName, const std::string& dataFile);

    /**
     * @brief LoadIndexingResults
     * @param descFile
     * @param dataFile
     * @param positions
     * @param eulers
     * @param patQual
     * @param detBands
     * @param phase
     * @param indexedBands
     * @param bmm
     * @param mapWidth
     * @param mapHeight
     * @return
     */
    static int LoadIndexingResults(const std::string& descFile, const std::string& dataFile, const UInt16Array::Pointer& positions, const FloatArray::Pointer& eulers,
                                   const FloatArray::Pointer& patQual, const UInt16Array::Pointer& detectedBands, const Int16Array::Pointer& phase,
                                   const UInt16Array::Pointer& indexedBands, const FloatArray::Pointer& bmm, int32_t& mapWidth, int32_t& mapHeight, std::vector<uint16_t>& roi,
                                   bool reorder = false);

    /**
     * @brief ReadScanSizes
     * @param mapWidth
     * @param mapHeight
     * @param ebspWidth
     * @param ebspHeight
     * @return
     */
    static int ReadScanSizes(const std::string &path, int32_t &mapWidth, int32_t &mapHeight, int32_t &ebspWidth, int32_t &ebspHeight);

    /**
     * @brief VerifyRequiredFiles
     * @param path
     * @return
     */
    static std::string VerifyRequiredFiles(const std::string &path);

    /**
     * @brief ReadPhaseInformation
     * @param path
     */
    static int ReadCrystalSymmetries(const std::string& path, const UInt32Array::Pointer& xtal);

  protected:
    BrukerDataLoader();

  public:
    BrukerDataLoader(const BrukerDataLoader&) = delete; // Copy Constructor Not Implemented
    BrukerDataLoader(BrukerDataLoader&&) = delete;      // Move Constructor Not Implemented
    BrukerDataLoader& operator=(const BrukerDataLoader&) = delete; // Copy Assignment Not Implemented
    BrukerDataLoader& operator=(BrukerDataLoader&&) = delete;      // Move Assignment Not Implemented

  private:
};




