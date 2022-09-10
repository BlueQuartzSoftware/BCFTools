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

#include "BrukerIntegration/BrukerIntegrationStructs.h"

#include "EbsdLib/Core/EbsdDataArray.hpp"

#include <memory>
#include <string>

/**
 * @brief The EbsdPatterns class
 */
class EbsdPatterns
{
  public:
    using Self = EbsdPatterns;
    using Pointer = std::shared_ptr<Self>;
    using ConstPointer = std::shared_ptr<const Self>;
    using WeakPointer = std::weak_ptr<Self>;
    using ConstWeakPointer = std::weak_ptr<const Self>;
    static Pointer NullPointer();

    static Pointer New();

    /**
     * @brief Returns the name of the class for EbsdPatterns
     */
    virtual std::string getNameOfClass() const;
    /**
     * @brief Returns the name of the class for EbsdPatterns
     */
    static std::string ClassName();

    virtual ~EbsdPatterns();

    EbsdPatterns::Pointer createSameSizePatterns();

    /**
     * @brief Setter property for ImageWidth
     */
    void setImageWidth(int32_t value);
    /**
     * @brief Getter property for ImageWidth
     * @return Value of ImageWidth
     */
    int32_t getImageWidth() const;

    /**
     * @brief Setter property for ImageHeight
     */
    void setImageHeight(int32_t value);
    /**
     * @brief Getter property for ImageHeight
     * @return Value of ImageHeight
     */
    int32_t getImageHeight() const;

    /**
     * @brief Setter property for PatternWidth
     */
    void setPatternWidth(int32_t value);
    /**
     * @brief Getter property for PatternWidth
     * @return Value of PatternWidth
     */
    int32_t getPatternWidth() const;

    /**
     * @brief Setter property for PatternHeight
     */
    void setPatternHeight(int32_t value);
    /**
     * @brief Getter property for PatternHeight
     * @return Value of PatternHeight
     */
    int32_t getPatternHeight() const;

    /**
     * @brief Setter property for PatternData
     */
    void setPatternData(const EbsdLib::UInt8Array::Pointer& value);
    /**
     * @brief Getter property for PatternData
     * @return Value of PatternData
     */
    EbsdLib::UInt8Array::Pointer getPatternData() const;

    /**
     * @brief Setter property for IsValid
     */
    void setIsValid(bool value);
    /**
     * @brief Getter property for IsValid
     * @return Value of IsValid
     */
    bool getIsValid() const;

    /**
     * @brief getPatternCount Returns the number of patterns
     * @return
     */
    int getPatternCount();

    /**
     * @brief getPatternElementCount Returns the number of elements in each pattern
     * @return
     */
    int getPerPatternElementCount();

    uint8_t* getPatternPointer(int32_t x, int32_t y);
    uint8_t* getPatternPointer(size_t index);

    EbsdLib::FloatArray::Pointer computeMeanPattern();
    EbsdLib::UInt64Array::Pointer computeDotProducts();

    void subtractMeanPattern(EbsdLib::FloatArray::Pointer meanPtr, EbsdLib::FloatArray::Pointer results);

  protected:
    EbsdPatterns();

  public:
    EbsdPatterns(const EbsdPatterns&) = delete;   // Copy Constructor Not Implemented
    EbsdPatterns(EbsdPatterns&&) = delete;        // Move Constructor Not Implemented
    EbsdPatterns& operator=(const EbsdPatterns&) = delete; // Copy Assignment Not Implemented
    EbsdPatterns& operator=(EbsdPatterns&&) = delete;      // Move Assignment Not Implemented

  private:
    int32_t m_ImageWidth = {-1};
    int32_t m_ImageHeight = {-1};
    int32_t m_PatternWidth = {-1};
    int32_t m_PatternHeight = {-1};
    EbsdLib::UInt8Array::Pointer m_PatternData = {};
    bool m_IsValid = {false};
};


