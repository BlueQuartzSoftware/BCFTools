#pragma once

#include <string>

class BcfHdf5Convertor
{
public:
  BcfHdf5Convertor(std::string inputFile, std::string outputFile);
  ~BcfHdf5Convertor();

  BcfHdf5Convertor(const BcfHdf5Convertor&) = delete;            // Copy Constructor Not Implemented
  BcfHdf5Convertor(BcfHdf5Convertor&&) = delete;                 // Move Constructor Not Implemented
  BcfHdf5Convertor& operator=(const BcfHdf5Convertor&) = delete; // Copy Assignment Not Implemented
  BcfHdf5Convertor& operator=(BcfHdf5Convertor&&) = delete;      // Move Assignment Not Implemented

  void setReorder(bool reorder);
  void setFlipPatterns(bool flipPatterns);
  void execute();

  int32_t getErrorCode() const;
  std::string getErrorMessage() const;

private:
  std::string m_InputFile;
  std::string m_OutputFile;

  std::string m_ErrorMessage = std::string("No Error");
  int32_t m_ErrorCode = 0;
  bool m_Reorder = false;
  bool m_FlipPatterns = false;
};
