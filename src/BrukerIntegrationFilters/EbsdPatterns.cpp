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

#include "EbsdPatterns.h"

#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/blocked_range2d.h>
#include <tbb/partitioner.h>
#endif

#include "BrukerIntegration/BrukerIntegrationStructs.h"

using namespace EbsdLib;

//#include "SIMPLib/FilterParameters/AbstractFilterParametersReader.h"
//#include "BrukerIntegration/BrukerIntegrationConfig.h"

#ifdef HAVE_SSE_EXTENSIONS
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
float dot_product_intrin(float* a, float* b, int size)
{
  //float arr[4];
  float total;
  int i;
  __m128 num1, num2, num3, num4;
  num4 = _mm_setzero_ps(); //sets sum to zero
  for(i = 0; i < size; i += 4)
  {
    num1 = _mm_loadu_ps(a + i); //loads unaligned array a into num1  num1= a[3]  a[2]  a[1]  a[0]
    num2 = _mm_loadu_ps(b + i); //loads unaligned array b into num2  num2= b[3]   b[2]   b[1]  b[0]
    num3 = _mm_mul_ps(num1, num2); //performs multiplication   num3 = a[3]*b[3]  a[2]*b[2]  a[1]*b[1]  a[0]*b[0]
    num3 = _mm_hadd_ps(num3, num3); //performs horizontal addition
    //num3=  a[3]*b[3]+ a[2]*b[2]  a[1]*b[1]+a[0]*b[0]  a[3]*b[3]+ a[2]*b[2]  a[1]*b[1]+a[0]*b[0]
    num4 = _mm_add_ps(num4, num3);  //performs vertical addition
  }
  num4 = _mm_hadd_ps(num4, num4);
  _mm_store_ss(&total, num4);
  return total;
}

#endif


/**
 * @brief
 */
class ComputeDotProductsImpl
{
    EbsdPatterns* m_Patterns;
    UInt64Array* m_Dots;

  public:
    ComputeDotProductsImpl(EbsdPatterns* patterns, UInt64Array* dots) :
      m_Patterns(patterns),
      m_Dots(dots)
    {
    }

    virtual ~ComputeDotProductsImpl() = default;

    // -----------------------------------------------------------------------------
    //
    // -----------------------------------------------------------------------------
    void convert(int yStart, int yEnd, int xStart, int xEnd) const
    {

      uint8_t* pattern = nullptr;
      int32_t iWidth = m_Patterns->getImageWidth();
      uint64_t* dots = m_Dots->getPointer(0);

      size_t count = m_Patterns->getPatternWidth() * m_Patterns->getPatternHeight();
      uint64_t total = 0;
      size_t dotsIdx = 0;
      for(int y = yStart; y < yEnd; ++y)
      {
        for(int x = xStart; x < xEnd; ++x)
        {
          dotsIdx = (y * iWidth) + x;
          total = 0;
          // Get the pattern for the Image Pixel(x,y)
          pattern = m_Patterns->getPatternPointer(x, y);
          for(size_t i = 0; i < count; ++i)
          {
            total = total + *pattern * *pattern;
            pattern = pattern + 1; // increment the pointer
          }
          dots[dotsIdx] = total;
        }
      }
    }

#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
    void operator()(const tbb::blocked_range2d<int, int>& r) const
    {
      convert(r.rows().begin(), r.rows().end(), r.cols().begin(), r.cols().end());
    }
#endif

  private:

};


class SubtractMeanPatternImpl
{
    EbsdPatterns* m_Patterns;
    FloatArray::Pointer m_MeanPattern;
    FloatArray::Pointer m_Results;

  public:
    SubtractMeanPatternImpl(EbsdPatterns* patterns,  FloatArray::Pointer meanPattern, FloatArray::Pointer results) :
      m_Patterns(patterns),
      m_MeanPattern(meanPattern),
      m_Results(results)
    {    }
    virtual ~SubtractMeanPatternImpl() = default;
    // -----------------------------------------------------------------------------
    //
    // -----------------------------------------------------------------------------
    void convert(int yStart, int yEnd, int xStart, int xEnd) const
    {

      uint8_t* pattern = nullptr;
      int32_t iWidth = m_Patterns->getImageWidth();
      float* mean = m_MeanPattern->getPointer(0);
      float* result = nullptr;
      size_t count = m_Patterns->getPatternWidth() * m_Patterns->getPatternHeight();
      for(int y = yStart; y < yEnd; ++y)
      {
        for(int x = xStart; x < xEnd; ++x)
        {
          int index = (y * iWidth) + x;
          // Get the pattern for the Image Pixel(x,y)
          pattern = m_Patterns->getPatternPointer(x, y);
          result = m_Results->getTuplePointer(index);
          for(size_t i = 0; i < count; ++i)
          {
            result[i] = (static_cast<float>(pattern[i]) - mean[i]);
          }
        }
      }
    }

#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
    void operator()(const tbb::blocked_range2d<int, int>& r) const
    {
      convert(r.rows().begin(), r.rows().end(), r.cols().begin(), r.cols().end());
    }
#endif

  private:
};

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
EbsdPatterns::EbsdPatterns() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
EbsdPatterns::~EbsdPatterns() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int EbsdPatterns::getPatternCount()
{
  return getImageHeight() * getImageWidth();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int EbsdPatterns::getPerPatternElementCount()
{
  return getPatternHeight() * getPatternWidth();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
EbsdPatterns::Pointer EbsdPatterns::createSameSizePatterns()
{
  EbsdPatterns::Pointer ptr = EbsdPatterns::New();
  ptr->setImageHeight(m_ImageHeight);
  ptr->setImageWidth(m_ImageWidth);
  ptr->setPatternHeight(m_PatternHeight);
  ptr->setPatternWidth(m_PatternWidth);

  std::vector<size_t> cDims(2);
  cDims[0] = m_PatternWidth;
  cDims[1] = m_PatternHeight;
  UInt8Array::Pointer data = UInt8Array::CreateArray(m_ImageHeight * m_ImageWidth, cDims, "EbsdPatterns", true);
  data->initializeWithZeros();
  ptr->setPatternData(data);
  return ptr;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
uint8_t* EbsdPatterns::getPatternPointer(int32_t x, int32_t y)
{
  int64_t index = (y * m_ImageWidth) + x;
  if (index < 0) { return nullptr; }
  return m_PatternData->getTuplePointer(index);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
uint8_t* EbsdPatterns::getPatternPointer(size_t index)
{
  if(!getIsValid())
  {
    return nullptr;
  }
  return m_PatternData->getTuplePointer(index);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
FloatArray::Pointer EbsdPatterns::computeMeanPattern()
{
  int32_t numPatterns = m_ImageWidth * m_ImageHeight;
  int32_t patternDataByteCount = m_PatternWidth * m_PatternHeight;
  uint8_t* buffer = nullptr;

  EbsdDataArray<float>::Pointer meanPtr = EbsdDataArray<float>::CreateArray((size_t)patternDataByteCount, std::string("MeanPattern"), true);
  meanPtr->initializeWithZeros();
  float* mean = meanPtr->getPointer(0);

  for(int32_t i = 0; i < numPatterns; ++i)
  {
    // Get the pointer to the Pattern at index i
    buffer = m_PatternData->getTuplePointer(i);
    // Loop over each byte in the pattern and increment the total in the mean pattern
    for(int i = 0; i < patternDataByteCount; ++i)
    {
      mean[i] = mean[i] + buffer[i];
    }
  }

  // Loop over each pixel in the mean pattern and divide by the total number of patterns.
  for(int i = 0; i < patternDataByteCount; ++i)
  {
    mean[i] = static_cast<float>(mean[i] / numPatterns);
  }
  return meanPtr;
}


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
UInt64Array::Pointer EbsdPatterns::computeDotProducts()
{

  UInt64Array::Pointer dots = UInt64Array::CreateArray(m_ImageWidth * m_ImageHeight, std::string("DotProducts"), true);

#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
  tbb::parallel_for(tbb::blocked_range2d<int, int>(0, m_ImageHeight, 0, m_ImageWidth),
                    ComputeDotProductsImpl(this, dots.get()), tbb::auto_partitioner());

#else
  ComputeDotProductsImpl serial(this, dots.get());
  serial.convert(0, m_ImageHeight, 0, m_ImageWidth);
#endif


  return dots;
}


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void EbsdPatterns::subtractMeanPattern(FloatArray::Pointer meanPtr, FloatArray::Pointer results)
{
#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
  tbb::parallel_for(tbb::blocked_range2d<int, int>(0, m_ImageHeight, 0, m_ImageWidth),
                    SubtractMeanPatternImpl(this, meanPtr, results), tbb::auto_partitioner());

#else
  SubtractMeanPatternImpl serial(this, meanPtr, results);
  serial.convert(0, m_ImageHeight, 0, m_ImageWidth);
#endif


}

// -----------------------------------------------------------------------------
EbsdPatterns::Pointer EbsdPatterns::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}

// -----------------------------------------------------------------------------
EbsdPatterns::Pointer EbsdPatterns::New()
{
  Pointer sharedPtr(new(EbsdPatterns));
  return sharedPtr;
}

// -----------------------------------------------------------------------------
std::string EbsdPatterns::getNameOfClass() const
{
  return std::string("EbsdPatterns");
}

// -----------------------------------------------------------------------------
std::string EbsdPatterns::ClassName()
{
  return std::string("EbsdPatterns");
}

// -----------------------------------------------------------------------------
void EbsdPatterns::setImageWidth(int32_t value)
{
  m_ImageWidth = value;
}

// -----------------------------------------------------------------------------
int32_t EbsdPatterns::getImageWidth() const
{
  return m_ImageWidth;
}

// -----------------------------------------------------------------------------
void EbsdPatterns::setImageHeight(int32_t value)
{
  m_ImageHeight = value;
}

// -----------------------------------------------------------------------------
int32_t EbsdPatterns::getImageHeight() const
{
  return m_ImageHeight;
}

// -----------------------------------------------------------------------------
void EbsdPatterns::setPatternWidth(int32_t value)
{
  m_PatternWidth = value;
}

// -----------------------------------------------------------------------------
int32_t EbsdPatterns::getPatternWidth() const
{
  return m_PatternWidth;
}

// -----------------------------------------------------------------------------
void EbsdPatterns::setPatternHeight(int32_t value)
{
  m_PatternHeight = value;
}

// -----------------------------------------------------------------------------
int32_t EbsdPatterns::getPatternHeight() const
{
  return m_PatternHeight;
}

// -----------------------------------------------------------------------------
void EbsdPatterns::setPatternData(const UInt8Array::Pointer& value)
{
  m_PatternData = value;
}

// -----------------------------------------------------------------------------
UInt8Array::Pointer EbsdPatterns::getPatternData() const
{
  return m_PatternData;
}

// -----------------------------------------------------------------------------
void EbsdPatterns::setIsValid(bool value)
{
  m_IsValid = value;
}

// -----------------------------------------------------------------------------
bool EbsdPatterns::getIsValid() const
{
  return m_IsValid;
}
