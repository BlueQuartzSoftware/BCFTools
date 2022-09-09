#include "BcfHdf5Convertor.h"

#include "BrukerIntegration/BrukerIntegrationConstants.h"
#include "BrukerIntegration/BrukerIntegrationStructs.h"
#include "BrukerIntegrationFilters/BrukerDataLoader.h"

#include "SIMPLib/DataArrays/DataArray.hpp"
#include "SIMPLib/Math/SIMPLibMath.h"

#include "H5Support/H5Lite.h"
#include "H5Support/H5ScopedErrorHandler.h"
#include "H5Support/H5ScopedSentinel.h"
#include "H5Support/H5Utilities.h"
using namespace H5Support;

#include "SFSNodeItem.h"
#include "SFSReader.h"

#include <QtCore/QDir>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTextStream>

#include <QtXml/QDomCharacterData>

#include <utility>
#include <cstring>
#include <array>
#include <cstddef>
#include <fstream>
#include <vector>

#if defined(__GNUC__) && !defined(__APPLE__)
#define BDL_POS(var) var.__pos
#else
#define BDL_POS(var) var
#endif

namespace
{
const std::string k_EBSD("EBSD");
const std::string k_SEM("SEM");
const std::string k_Manufacturer("Manufacturer");
const std::string k_Version("Version");
const std::string k_Data("Data");
const std::string k_Header("Header");

const int32_t k_FileVersion = 3;

/******************************************************************************
 * START TIFF WRITING SECTION
 *****************************************************************************/
const uint16_t PHOTOMETRIC_MINISBLACK = 0x0001;
const uint16_t PHOTOMETRIC_RGB = 0x0002;

struct TIFTAG
{
  int16_t tagId = 0;      // The tag identifier
  int16_t dataType = 0;   // The scalar type of the data items
  int32_t dataCount = 0;  // The number of items in the tag data
  int32_t dataOffset = 0; // The byte offset to the data items

  friend std::ostream& operator<<(std::ostream& out, const TIFTAG& tag)
  {
    out.write(reinterpret_cast<const char*>(&tag.tagId), sizeof(tag.tagId));
    out.write(reinterpret_cast<const char*>(&tag.dataType), sizeof(tag.dataType));
    out.write(reinterpret_cast<const char*>(&tag.dataCount), sizeof(tag.dataCount));
    out.write(reinterpret_cast<const char*>(&tag.dataOffset), sizeof(tag.dataOffset));
    return out;
  }
};

enum class Endianess
{
  Little = 0,
  Big
};

Endianess checkEndianess()
{
  constexpr uint32_t i = 0x01020304;
  const std::byte* u8 = reinterpret_cast<const std::byte*>(&i);

  return u8[0] == std::byte{0x01} ? Endianess::Big : Endianess::Little;
}

// This is just here to silence the compiler. We are not fully supporting tiff writing as an option. Just for debugging
std::pair<int32_t, std::string> WriteGrayScaleImage(const std::string& filepath, int32_t width, int32_t height, const uint16_t* data)
{
  return {-1, "16 Bit tiff not supported"};
}

// -----------------------------------------------------------------------------
std::pair<int32_t, std::string> WriteGrayScaleImage(const std::string& filepath, int32_t width, int32_t height, const uint8_t* data)
{
  int32_t samplesPerPixel = 1;
  // Check for Endianess of the system and write the appropriate magic number according to the tiff spec
  std::array<char, 4> magicNumber = {0x49, 0x49, 0x2A, 0x00};

  if(checkEndianess() == Endianess::Big)
  {
    magicNumber = {0x4D, 0x4D, 0x00, 0x2A};
  }

  // open file and write header
  std::ofstream outputFile(filepath, std::ios::binary);
  if(!outputFile.is_open())
  {
    return {-1, "Could not open output file for writing"};
  }

  outputFile.write(magicNumber.data(), magicNumber.size());
  // Generate the offset into the Image File Directory (ifd) which we are going to write first
  constexpr uint32_t ifd_Offset = 8;
  outputFile.write(reinterpret_cast<const char*>(&ifd_Offset), sizeof(ifd_Offset));

  std::vector<TIFTAG> tags;
  tags.push_back(TIFTAG{0x00FE, 0x0004, 1, 0x00000000});                       // NewSubfileType
  tags.push_back(TIFTAG{0x0100, 0x0004, 1, width});                            // ImageWidth
  tags.push_back(TIFTAG{0x0101, 0x0004, 1, height});                           // ImageLength
  tags.push_back(TIFTAG{0x0102, 0x0003, 1, 8 * sizeof(char)});                 // BitsPerSample
  tags.push_back(TIFTAG{0x0103, 0x0003, 1, 0x0001});                           // Compression
  tags.push_back(TIFTAG{0x0106, 0x0003, 1, PHOTOMETRIC_MINISBLACK});           // PhotometricInterpretation  // For SamplesPerPixel = 3 or 4 (RGB or RGBA)
  tags.push_back(TIFTAG{0x0112, 0x0003, 1, 1});                                // Orientation
  tags.push_back(TIFTAG{0x0115, 0x0003, 1, 1});                                // SamplesPerPixel
  tags.push_back(TIFTAG{0x0116, 0x0004, 1, height});                           // RowsPerStrip
  tags.push_back(TIFTAG{0x0117, 0x0004, 1, width * height * samplesPerPixel}); // StripByteCounts
  // TIFTAG XResolution;
  // TIFTAG YResolution;
  // TIFTAG ResolutionUnit;
  tags.push_back(TIFTAG{0x011c, 0x0003, 1, 0x0001}); // PlanarConfiguration

  // Now compute the offset to the image data so that we can put that into the tag.
  // THESE NEXT 2 LINES MUST BE THE LAST TAG TO BE PUSHED BACK INTO THE VECTOR OR THE MATH WILL BE WRONG
  int32_t imageDataOffset = static_cast<int32_t>(8 + ((tags.size() + 1) * 12) + 6); // Header + tags + IDF Tag entry count and Next IFD Offset
  tags.push_back(TIFTAG{0x0111, 0x0004, 1, imageDataOffset});                       // StripOffsets

  // Write the number of tags to the IFD section
  uint16_t numEntries = static_cast<uint16_t>(tags.size());
  outputFile.write(reinterpret_cast<const char*>(&numEntries), sizeof(numEntries));
  // write the tags to the file.
  for(const auto& tag : tags)
  {
    outputFile << tag;
  }
  // Write the "Next Tag Offset"
  constexpr uint32_t nextOffset = 0;
  outputFile.write(reinterpret_cast<const char*>(&nextOffset), sizeof(nextOffset));

  // Now write the actual image data
  int32_t imageByteCount = width * height * samplesPerPixel;
  outputFile.write(reinterpret_cast<const char*>(data), imageByteCount);

  // and we are done.
  return {0, "No Error"};
}

} // namespace

// -----------------------------------------------------------------------------
BcfHdf5Convertor::BcfHdf5Convertor(QString inputFile, QString outputFile)
: m_InputFile(std::move(inputFile))
, m_OutputFile(std::move(outputFile))
{
}

// -----------------------------------------------------------------------------
BcfHdf5Convertor::~BcfHdf5Convertor() = default;

void BcfHdf5Convertor::setReorder(bool reorder)
{
  m_Reorder = reorder;
}

void BcfHdf5Convertor::setFlipPatterns(bool flipPatterns)
{
  m_FlipPatterns = flipPatterns;
}

// -----------------------------------------------------------------------------
int32_t writeCameraConfiguration(hid_t semGrpId, hid_t ebsdGrpId, const QString& cameraConfiguration)
{
  QString errorStr;
  int errorLine = -1;
  int errorColumn = -1;

  QFile device(cameraConfiguration);

  QDomDocument domDocument;
  QDomElement root;

  if(!domDocument.setContent(&device, true, &errorStr, &errorLine, &errorColumn))
  {
    QString ss = QObject::tr("Parse error at line %1, column %2:\n%3").arg(errorLine).arg(errorColumn).arg(errorStr);
    qDebug() << ss;
    return -7080;
  }

  root = domDocument.documentElement();
  QDomElement classInstance = root.firstChildElement("ClassInstance");
  if(classInstance.isNull())
  {
    std::cout << "XML DOM entry ClassInstance was not found." << std::endl;
    return -7002;
  }
  int32_t err = 0;

  // Start gathering the required information from the XML file
  QString pixelFormat = classInstance.firstChildElement("PixelFormat").text();
  int32_t pixelByteCount = 0;
  if(pixelFormat == "Gray8")
  {
    pixelByteCount = 1;
  }
  else if(pixelFormat == "Gray16")
  {
    pixelByteCount = 2;
  }
  err = H5Lite::writeScalarDataset(ebsdGrpId, "PixelByteCount", pixelByteCount);

  return err;
}

// -----------------------------------------------------------------------------
int32_t writeAuxIndexingOptions(hid_t semGrpId, hid_t ebsdGrpId, const QString& calibrationFile)
{
  QString errorStr;
  int errorLine = -1;
  int errorColumn = -1;

  QFile device(calibrationFile);

  QDomDocument domDocument;
  QDomElement root;

  if(!domDocument.setContent(&device, true, &errorStr, &errorLine, &errorColumn))
  {
    QString ss = QObject::tr("Parse error at line %1, column %2:\n%3").arg(errorLine).arg(errorColumn).arg(errorStr);
    qDebug() << ss;
    return -7080;
  }

  root = domDocument.documentElement();
  QDomElement classInstance = root.firstChildElement("ClassInstance");
  if(classInstance.isNull())
  {
    std::cout << "XML DOM entry ClassInstance was not found." << std::endl;
    return -7002;
  }
  bool ok = false;
  int32_t err = 0;

  // Start gathering the required information from the XML file
  int32_t minIndexedBands = classInstance.firstChildElement("MinIndexedBandCount").text().toInt(&ok);
  err = H5Lite::writeScalarDataset(ebsdGrpId, "MinIndexedBands", minIndexedBands);

  // Start gathering the required information from the XML file
  double madMax = classInstance.firstChildElement("MaxMAD").text().toDouble(&ok);
  err = H5Lite::writeScalarDataset(ebsdGrpId, "MADMax", madMax);

  return err;
}

// -----------------------------------------------------------------------------
int32_t writeCalibrationData(hid_t semGrpId, hid_t ebsdGrpId, const QString& calibrationFile, float& pcx, float& pcy)
{
  QString errorStr;
  int errorLine = -1;
  int errorColumn = -1;

  QFile device(calibrationFile);

  QDomDocument domDocument;
  QDomElement root;

  if(!domDocument.setContent(&device, true, &errorStr, &errorLine, &errorColumn))
  {
    QString ss = QObject::tr("Parse error at line %1, column %2:\n%3").arg(errorLine).arg(errorColumn).arg(errorStr);
    qDebug() << ss;
    return -7080;
  }

  root = domDocument.documentElement();
  QDomElement classInstance = root.firstChildElement("ClassInstance");
  if(classInstance.isNull())
  {
    std::cout << "XML DOM entry ClassInstance was not found." << std::endl;
    return -7002;
  }
  bool ok = false;
  int32_t err = 0;

  // Start gathering the required information from the XML file
  double workingDistance = classInstance.firstChildElement("WorkingDistance").text().toDouble(&ok);
  err = H5Lite::writeScalarDataset(semGrpId, "SEM WD", workingDistance);
  err = H5Lite::writeScalarDataset(ebsdGrpId, "WD", workingDistance);

  double topClip = classInstance.firstChildElement("TopClip").text().toDouble(&ok);
  err = H5Lite::writeScalarDataset(ebsdGrpId, "TopClip", topClip);

  pcx = classInstance.firstChildElement("PCX").text().toDouble(&ok);
  err = H5Lite::writeScalarDataset(ebsdGrpId, "PCX", pcx);

  pcy = classInstance.firstChildElement("PCY").text().toDouble(&ok);
  err = H5Lite::writeScalarDataset(ebsdGrpId, "PCX", pcy);

  float sampleTilt = classInstance.firstChildElement("ProbeTilt").text().toDouble(&ok);
  err = H5Lite::writeScalarDataset(ebsdGrpId, "SampleTilt", sampleTilt);


  return err;
}

// -----------------------------------------------------------------------------
int32_t writeSEMData(hid_t semGrpId, hid_t ebsdGrpId, const QString& semFile)
{
  QString errorStr;
  int errorLine;
  int errorColumn;

  QFile device(semFile);

  QDomDocument domDocument;
  QDomElement root;

  if(!domDocument.setContent(&device, true, &errorStr, &errorLine, &errorColumn))
  {
    QString ss = QObject::tr("Parse error at line %1, column %2:\n%3").arg(errorLine).arg(errorColumn).arg(errorStr);
    qDebug() << ss;
    return -7080;
  }

  root = domDocument.documentElement();
  QDomElement classInstance = root.firstChildElement("ClassInstance");
  if(classInstance.isNull())
  {
    std::cout << "XML DOM entry ClassInstance was not found." << std::endl;
    return -7002;
  }
  bool ok = false;
  int32_t err = 0;

  QString date = classInstance.firstChildElement("Date").text();
  err = H5Lite::writeStringDataset(ebsdGrpId, "Date", date.toStdString());
  err = H5Lite::writeStringAttribute(ebsdGrpId, "Date", "Format (ISO 8601)", "dd.mm.yyyy");

  QString time = classInstance.firstChildElement("Time").text();
  err = H5Lite::writeStringDataset(ebsdGrpId, "Time", time.toStdString());
  err = H5Lite::writeStringAttribute(ebsdGrpId, "Time", "Format (ISO 8601)", "hh:mm:ss");

  int32_t width = classInstance.firstChildElement("Width").text().toInt(&ok);
  err = H5Lite::writeScalarDataset(semGrpId, "SEM ImageWidth", width);

  int32_t height = classInstance.firstChildElement("Height").text().toInt(&ok);
  err = H5Lite::writeScalarDataset(semGrpId, "SEM ImageHeight", height);

  std::vector<hsize_t> tDims = {static_cast<hsize_t>(height), static_cast<hsize_t>(width)};

  float xRes = classInstance.firstChildElement("XCalibration").text().toFloat(&ok);
  if(xRes == 0.0) { xRes = 1.0f;}
  err = H5Lite::writeScalarDataset(semGrpId, "SEM XResolution", xRes);
  err = H5Lite::writeScalarDataset(ebsdGrpId, "SEPixelSizeX", xRes);
  err = H5Lite::writeScalarDataset(ebsdGrpId, "XSTEP", xRes);

  float yRes = classInstance.firstChildElement("YCalibration").text().toFloat(&ok);
  if(yRes == 0.0) { yRes = 1.0f; }
  err = H5Lite::writeScalarDataset(semGrpId, "SEM YResolution", yRes);
  err = H5Lite::writeScalarDataset(ebsdGrpId, "SEPixelSizeY", yRes);
  err = H5Lite::writeScalarDataset(ebsdGrpId, "YSTEP", yRes);

  int itemSize = classInstance.firstChildElement("ItemSize").text().toInt(&ok);

  int32_t planeCount = classInstance.firstChildElement("PlaneCount").text().toInt(&ok);
  for(int32_t p = 0; p < planeCount; p++)
  {
    QString tagName = "Plane" + QString::number(p);
    QDomElement planeDomEle = classInstance.firstChildElement(tagName);
    QByteArray b64str = planeDomEle.firstChildElement("Data").text().toLatin1();
    QByteArray decodedImage = QByteArray::fromBase64(b64str);
    QString nameDomEle = planeDomEle.firstChildElement("Name").text();
    QString descDomEle = planeDomEle.firstChildElement("Description").text();

    if(!nameDomEle.isEmpty() && !descDomEle.isEmpty())
    {
      if(itemSize == 1)
      {
        err = H5Lite::writePointerDataset<uint8_t>(semGrpId, "SEM Image", 2, tDims.data(), reinterpret_cast<uint8_t*>(decodedImage.data()));
      }
      else if(itemSize == 2)
      {
        err = H5Lite::writePointerDataset<uint16_t>(semGrpId, "SEM Image", 2, tDims.data(), reinterpret_cast<uint16_t*>(decodedImage.data()));
      }
      err = H5Lite::writeStringAttribute(semGrpId, "SEM Image", "CLASS", "IMAGE");
      err = H5Lite::writeStringAttribute(semGrpId, "SEM Image", "IMAGE_SUBCLASS", "IMAGE_INDEXED");
      err = H5Lite::writeStringAttribute(semGrpId, "SEM Image", "IMAGE_VERSION", "1.2");

      // And now write the same image in the EBSD/Header group?
      if(itemSize == 1)
      {
        err = H5Lite::writePointerDataset<uint8_t>(ebsdGrpId, "SEM Image", 2, tDims.data(), reinterpret_cast<uint8_t*>(decodedImage.data()));
      }
      else if(itemSize == 2)
      {
        err = H5Lite::writePointerDataset<uint16_t>(ebsdGrpId, "SEM Image", 2, tDims.data(), reinterpret_cast<uint16_t*>(decodedImage.data()));
      }
      err = H5Lite::writeStringAttribute(ebsdGrpId, "SEM Image", "CLASS", "IMAGE");
      err = H5Lite::writeStringAttribute(ebsdGrpId, "SEM Image", "IMAGE_SUBCLASS", "IMAGE_INDEXED");
      err = H5Lite::writeStringAttribute(ebsdGrpId, "SEM Image", "IMAGE_VERSION", "1.2");

      if(!nameDomEle.isEmpty())
      {
        err = H5Lite::writeStringAttribute(semGrpId, "SEM Image", "Name", nameDomEle.toStdString());
        err = H5Lite::writeStringAttribute(ebsdGrpId, "SEM Image", "Name", nameDomEle.toStdString());
      }
      if(!descDomEle.isEmpty())
      {
        err = H5Lite::writeStringAttribute(semGrpId, "SEM Image", "Description", descDomEle.toStdString());
        err = H5Lite::writeStringAttribute(ebsdGrpId, "SEM Image", "Description", descDomEle.toStdString());
      }
    }
  }

  float semKV = 0.0f;
  float semMag = -1.0f;

  QDomElement trtHeaderedClass = classInstance.firstChildElement("TRTHeaderedClass");
  QDomElement tRTREMHeader = trtHeaderedClass.firstChildElement("ClassInstance");
  if(!tRTREMHeader.isNull())
  {
    semKV = tRTREMHeader.firstChildElement("Energy").text().toFloat(&ok);
    semMag = tRTREMHeader.firstChildElement("Magnification").text().toFloat(&ok);
    //    float wd = tRTREMHeader.firstChildElement("WorkingDistance").text().toFloat(&ok);
    //    err = H5Lite::writeScalarDataset(semGrpId, "SEM WD", wd);
  }
  err = H5Lite::writeScalarDataset(semGrpId, "SEM KV", semKV);
  err = H5Lite::writeScalarDataset(ebsdGrpId, "KV", semKV);
  err = H5Lite::writeScalarDataset(semGrpId, "SEM Magnification", semMag);
  err = H5Lite::writeScalarDataset(ebsdGrpId, "Magnification", semMag);

  return 0;
}

// -----------------------------------------------------------------------------
int32_t writePhaseInformation(hid_t headerGrpId, const QString& phaseListFile)
{

  hid_t phaseGrpId = H5Utilities::createGroup(headerGrpId, Bruker::Header::Phases.toStdString());
  H5GroupAutoCloser phaseGrpAutoClose(phaseGrpId);
  QString errorStr;
  int errorLine;
  int errorColumn;

  QFile device(phaseListFile);

  QDomDocument domDocument;
  QDomElement root;

  if(!domDocument.setContent(&device, true, &errorStr, &errorLine, &errorColumn))
  {
    QString ss = QObject::tr("Parse error at line %1, column %2:\n%3").arg(errorLine).arg(errorColumn).arg(errorStr);
    std::cout << ss.toStdString() << std::endl;
    return -70000;
  }

  root = domDocument.documentElement();

  QDomElement classInstance = root.firstChildElement("ClassInstance");
  if(classInstance.isNull())
  {
    std::cout << "XML DOM entry ClassInstance was not found." << std::endl;
    return -7002;
  }

  QDomElement childClassInstances = classInstance.firstChildElement("ChildClassInstances");
  if(childClassInstances.isNull())
  {
    std::cout << "XML DOM entry ChildClassInstances was not found." << std::endl;
    return -7003;
  }

  QDomNodeList phaseInstaces = childClassInstances.childNodes();
  int32_t count = phaseInstaces.count();
  for(int32_t i = 0; i < count; i++)
  {
    int32_t err = 0;
    bool ok = true;
    hid_t grpId = H5Utilities::createGroup(phaseGrpId, QString::number(i + 1).toStdString());
    H5GroupAutoCloser groupAutoCloser(grpId);

    QDomNode phaseInstance = phaseInstaces.at(i);
    QString name = phaseInstance.toElement().attribute("Name");

    err = H5Lite::writeStringDataset(grpId, "Name", name.toStdString());

    QDomNode tEBSDPhaseEntry = phaseInstance.firstChildElement("TEBSDPhaseEntry");

    //    phaseInstance.firstChildElement("POS0").text() );

    //    phaseInstance.firstChildElement("REF").text() );

    //    phaseInstance.firstChildElement("Complete").text() );

    //    phaseInstance.firstChildElement("BPS1").text() );

    //    phaseInstance.firstChildElement("BPS2").text() );
    QString strValue = tEBSDPhaseEntry.firstChildElement("Chem").text();
    err = H5Lite::writeStringDataset(grpId, "Formula", strValue.toStdString());

    //    phaseInstance.firstChildElement("DENSITY").text() );

    //    phaseInstance.firstChildElement("Elem").text() );

    /* Parse a DefaultGlideSystem subclass */
    //    m_DefaultGlideSystem = DefaultGlideSystem::Pointer(new DefaultGlideSystem(parent()));
    //    QDomElement ele_DefaultGlideSystem = phaseInstance.firstChildElement("DefaultGlideSystem");
    //    err = m_DefaultGlideSystem->parse(ele_DefaultGlideSystem);

    //    phaseInstance.firstChildElement("ISUSERDB").text() );

    //    phaseInstance.firstChildElement("PKEY").text() );

    //    phaseInstance.firstChildElement("DBNAME").text() );

    /* Parse a Cell subclass */
    //    m_Cell = Cell::Pointer(new Cell(parent()));
    QDomElement ele_Cell = tEBSDPhaseEntry.firstChildElement("Cell");
    {
      std::vector<float> latticeConstants(6);
      QStringList values = ele_Cell.firstChildElement("Dim").text().split(",");
      latticeConstants[0] = values[0].toFloat(&ok);
      latticeConstants[1] = values[1].toFloat(&ok);
      latticeConstants[2] = values[2].toFloat(&ok);
      values = ele_Cell.firstChildElement("Angles").text().split(",");
      latticeConstants[3] = values[0].toFloat(&ok);
      latticeConstants[4] = values[1].toFloat(&ok);
      latticeConstants[5] = values[2].toFloat(&ok);
      std::vector<hsize_t> dims = {6};
      err = H5Lite::writeVectorDataset(grpId, "LatticeConstants", dims, latticeConstants);
    }

    //    err = m_Cell->parse(ele_Cell);

    //    phaseInstance.firstChildElement("MU").text() );

    //    phaseInstance.firstChildElement("USED").text() );

    //    phaseInstance.firstChildElement("ENTRYID").text() );

    //    phaseInstance.firstChildElement("Encrypted").text() );

    int32_t se = tEBSDPhaseEntry.firstChildElement("SE").text().toInt(&ok);
    err = H5Lite::writeScalarDataset(grpId, "Setting", se);

    //    phaseInstance.firstChildElement("CC").text() );

    //    phaseInstance.firstChildElement("REFLVERSION").text() );

    strValue = tEBSDPhaseEntry.firstChildElement("SG").text();
    err = H5Lite::writeStringDataset(grpId, "SpaceGroup", strValue.toStdString());

    int32_t it = tEBSDPhaseEntry.firstChildElement("IT").text().toInt(&ok);
    err = H5Lite::writeScalarDataset(grpId, "IT", it);

    hid_t atGrpId = H5Utilities::createGroup(grpId, "AtomPositions");
    H5GroupAutoCloser atGrpIdAutoCloser(atGrpId);
    // This tells us the number of atoms that we need to parse
    it = tEBSDPhaseEntry.firstChildElement("AT").text().toInt(&ok);
    for(int32_t atom = 1; atom <= it; atom++)
    {
      QString tagName = "POS" + QString::number(atom - 1);
      strValue = tEBSDPhaseEntry.firstChildElement(tagName).text();
      err = H5Lite::writeStringDataset(atGrpId, QString::number(atom).toStdString(), strValue.toStdString());
    }
  }

  return 0;
}

// -----------------------------------------------------------------------------
template<typename T>
int32_t writeDataArrayToHdf5(DataArray<T>* data, hid_t grpId, size_t numElements)
{
  std::vector<size_t> cDims = data->getComponentDimensions();
  if(cDims.size() == 1 && cDims[0] == 1)
  {
    cDims.clear();
  }
//  int32_t rank = 1 + cDims.size();
  std::vector<hsize_t> dims;
  dims.push_back(numElements);
  for(const auto& dim : cDims)
  {
    dims.push_back(dim);
  }

  int32_t err = H5Lite::writePointerDataset(grpId, data->getName().toStdString(), static_cast<int32_t>(dims.size()), dims.data(), data->getPointer(0));
  return err;
}

// -----------------------------------------------------------------------------
int32_t analyzeFrameDescriptionFile(const QString& descFile)
{
  std::cout << "************** Frame Description File START ****************************" << std::endl;
  QString filePath = descFile;
  QFileInfo descFileInfo(filePath);
  if(!descFileInfo.exists())
  {
    std::cout << "The FrameDescription File does not exist: '" << filePath.toStdString() << "'" << std::endl;
    return -10;
  }

  // Open the FrameData File
  FILE* f = fopen(descFileInfo.absoluteFilePath().toStdString().c_str(), "rb");
  if(nullptr == f)
  {
    std::cout << "Could not open the FrameData File" << std::endl;
    return -14;
  }

  FrameDescriptionHeader_t descHeader;
  // uint64_t filePos = 0;
  size_t nRead = fread(&descHeader, 12, 1, f);
  std::cout << "Frame Description File:" << std::endl;
  std::cout << "    Width:" << descHeader.width << std::endl;
  std::cout << "    Height:" << descHeader.height << std::endl;
  std::cout << "    Total Possible Scanned Points:" << descHeader.patternCount << std::endl;
  int32_t totalPatternsAvailable = 0;
  uint64_t maxIndex = 0;
  for(int32_t i = 0; i < descHeader.patternCount; i++)
  {
    uint64_t offset = 0;
    fpos_t pos;
    fgetpos(f, &pos);
    nRead = fread(&offset, sizeof(uint64_t), 1, f);
    if(offset != 0xFFFFFFFFFFFFFFFF)
    {
      totalPatternsAvailable++;
      if(offset > maxIndex)
      {
        maxIndex = offset;
      }
    }
  }
  std::cout << "Total Pixels Measured: " << totalPatternsAvailable << std::endl;
  // std::cout << "Max File Index: " << maxIndex << std::endl;
  fclose(f);
  std::cout << "************** Frame Description File END ****************************" << std::endl;

  return 0;
}


// -----------------------------------------------------------------------------
template <typename T>
int32_t writePatternData(const SFSReader& sfsFile, hid_t native_type, int32_t mapWidth, int32_t mapHeight, int32_t ebspWidth,
                         int32_t ebspHeight, bool flipPatterns, const QString& tempDir, const QString& dataFile,
                         const QString& descFile, hid_t dataGrpId)
{
  int32_t err = 0;
  // ===================================================
  // Get the XBEAM and YBEAM data from the HDF5 file
  std::vector<int32_t> xbeam;
  err = H5Lite::readVectorDataset(dataGrpId, Bruker::IndexingResults::XBEAM.toStdString(), xbeam);

  std::vector<int32_t> ybeam;
  err = H5Lite::readVectorDataset(dataGrpId, Bruker::IndexingResults::YBEAM.toStdString(), ybeam);

  // ===================================================
  // Check the FrameDescription File exists
  QString filePath = descFile;
  QFileInfo descFileInfo(filePath);
  if(!descFileInfo.exists())
  {
    std::cout << "The FrameDescription File does not exist: '" << filePath.toStdString() << "'" << std::endl;
    return -10;
  }
  err = analyzeFrameDescriptionFile(descFile);
  if(err < 0)
  {
    return err;
  }

  // Open the FrameDescription File. This tells for every scan point, where the pattern
  // data starts in the FrameData file.
  std::vector<size_t> frameDescription;
  {
    FILE* f = fopen(descFileInfo.absoluteFilePath().toStdString().c_str(), "rb");
    FrameDescriptionHeader_t descHeader;
    size_t nRead = fread(&descHeader, 12, 1, f);
    frameDescription.resize(descHeader.patternCount);
    nRead = fread(frameDescription.data(), sizeof(size_t), descHeader.patternCount, f);
    fclose(f);
  }

  // ===================================================
  // Extract the FrameData file from the .bcf file. this can take a bit....
  err = sfsFile.extractFile(tempDir.toStdString(), dataFile.toStdString());
  if(err != 0)
  {
    std::cout << "Error extracting the " << dataFile.toStdString() << ". This data set will not be included in the resulting HDF5 file." << std::endl;
    return err;
  }

  filePath = tempDir + "/" + dataFile;
  QFileInfo dataFileInfo(filePath);
  if(!dataFileInfo.exists())
  {
    std::cout << "The FrameData File does not exist: '" << filePath.toStdString() << "'" << std::endl;
    return -11;
  }
  qint64 filesize = dataFileInfo.size();

  // Open the FrameData File
  FILE* f = fopen(dataFileInfo.absoluteFilePath().toStdString().c_str(), "rb");
  if(nullptr == f)
  {
    std::cout << "Could not open the FrameData File" << std::endl;
    return -14;
  }

  // Put the file pointer back to the start of the file
  rewind(f);
  FrameDataHeader_t patternHeader;
  std::cout << "Parsing the Pattern Size from the first data Record...." << std::endl;
  // Read the first pattern header which will give us the height & width of the actual pattern data.
  size_t nRead = fread(&patternHeader, 1, 25, f);
  if(nRead != 25)
  {
    std::cout << "Could not read the Frame Data Header values. Only " << nRead << " values were parsed" << std::endl;
    fclose(f);
    return -15;
  }

  std::cout << "Pattern size is W=" << patternHeader.width << "\tH=" << patternHeader.height << "\tBytes_Per_Pixel=" << patternHeader.bytesPerPixel << std::endl;
  // Put the file pointer back to the start of the file
  rewind(f);

  int32_t patternDataTupleCount = patternHeader.width * patternHeader.height;
  // Allocate a row's worth of memory for the pattern data to be read into
  std::vector<T> patternData(mapWidth * patternHeader.width * patternHeader.height);

  // ===================================================
  int32_t patternRank = 3;
  std::array<hsize_t, 3> dims = {static_cast<hsize_t>(mapWidth), static_cast<hsize_t>(ebspHeight), static_cast<hsize_t>(ebspWidth)};
  std::array<hsize_t, 3> maxdims = {static_cast<hsize_t>(mapWidth * mapHeight), static_cast<hsize_t>(ebspHeight), static_cast<hsize_t>(ebspWidth)};
  hid_t dataspace = H5Screate_simple(patternRank, dims.data(), maxdims.data());

  // Modify dataset creation properties, i.e. enable chunking.
  std::array<hsize_t, 3> chunk_dims = {static_cast<hsize_t>(mapWidth), static_cast<hsize_t>(ebspHeight), static_cast<hsize_t>(ebspWidth)};
  hid_t cparms = H5Pcreate(H5P_DATASET_CREATE);
  herr_t status = H5Pset_chunk(cparms, patternRank, chunk_dims.data());
  T fillvalue = 0;
  status = H5Pset_fill_value(cparms, native_type, &fillvalue);

  // Create a new dataset within the file using cparms creation properties.
  hid_t dataset = H5Dcreate2(dataGrpId, Bruker::IndexingResults::EBSP.toStdString().c_str(), native_type, dataspace, H5P_DEFAULT, cparms, H5P_DEFAULT);
  hid_t filespace = 0;

  size_t beamIdx = 0;
  for(int32_t y = 0; y < mapHeight; y++)
  {
    std::cout << dataFileInfo.fileName().toStdString() << " Writing Row " << y << "/" << mapHeight << "\r";
    std::cout.flush();

    for(int32_t x = 0; x < mapWidth; x++)
    {
      fpos_t pos;
      size_t patternDataPtrOffset = x * ebspWidth * ebspHeight;
      uint64_t filePos = frameDescription[beamIdx++];        // Get the file position of the pattern
      if(filePos != 0xFFFFFFFFFFFFFFFF)
      {
        fseek(f, filePos + 25, SEEK_SET);                                       // Set the file position to the pattern data
        // Compute the index into the current pattern vector to store the pattern
        // Read the Data associated with the header that was read.
        fgetpos(f, &pos);

        if(flipPatterns)
        {
          std::vector<T> tiffPatternData(patternDataTupleCount, 0);
          nRead = fread(tiffPatternData.data(), sizeof(T), patternDataTupleCount, f);
          size_t targetIndex = 0;
          T* targetPattern = patternData.data() + patternDataPtrOffset;
          for(int h = patternHeader.height -1; h >= 0; h--)
          {
            size_t pIndex = h * patternHeader.width;
            T* sourcePatternRowPtr = &tiffPatternData[pIndex];
            T* targetPatternRowPtr = &targetPattern[targetIndex];
            ::memcpy(targetPatternRowPtr, sourcePatternRowPtr, sizeof(T) * patternHeader.width);
            targetIndex += patternHeader.width;
          }
#if 0
SERIOUSLY: DO NOT UNCOMMENT THIS UNLESS YOU WANT TO KILL YOUR COMPUTER
          std::stringstream ss;
          ss << "/tmp/pattern_" << x << "_" << y << ".tiff";
          std::pair<int32_t, std::string> result = ::WriteGrayScaleImage(ss.str(), patternHeader.width, patternHeader.height, patternData.data() + patternDataPtrOffset);
          if(result.first < 0)
          {
            std::cout << result.second << std::endl;
          }
#endif
        }
        else
        {
          nRead = fread(patternData.data() + patternDataPtrOffset, sizeof(T), patternDataTupleCount, f);
        }
#if 0
// This section is for writing patterns to a tiff file. ONLY DO THIS IF YOU ARE IN
// A DEBUGGER STEPPING THROUGH THE CODE. Dumping a few hundred thousand files onto
// your desktop is not going to end well for ANY operating system, yes, Linux included.
        {
          fseek(f, filePos + 25, SEEK_SET);                                       // Set the file position to the pattern data
          // Compute the index into the current pattern vector to store the pattern
          // Read the Data associated with the header that was read.
          fgetpos(f, &pos);
          std::stringstream ss;
          ss << "/tmp/pattern_" << x << "_" << y << ".tiff";
          std::vector<uint8_t> tiffPatternData(patternDataTupleCount, 0);
          nRead = fread(tiffPatternData.data(), sizeof(T), patternDataTupleCount, f);

          std::pair<int32_t, std::string> result = ::WriteGrayScaleImage(ss.str(), patternHeader.width, patternHeader.height, tiffPatternData.data());
          if(result.first < 0)
          {
            std::cout << result.second << std::endl;
          }
        }
#endif
      }
      else
      {
        // Write ZEROS to the pattern data
        std::memset(patternData.data() + patternDataPtrOffset, 0x00, patternDataTupleCount);
      }


      if(feof(f) != 0)
      {
        std::cout << "Unexpected End of File (EOF) was encountered. Details follow" << std::endl;
        std::cout << "File Size: " << filesize << std::endl;
        std::cout << "ferror(f) = " << ferror(f) << "  feof(f) = " << feof(f) << std::endl;
        // std::cout << "File Pos When Reading: " << static_cast<size_t>(pos) << std::endl;
        printf("File Pos When Reading: %llu\n", static_cast<unsigned long long int>(BDL_POS(pos)));
        fgetpos(f, &pos);
        std::cout << "error reading data file: nRead=" << nRead << " but needed: " << patternDataTupleCount << std::endl;
        printf(" Current File Position: %llu\n", static_cast<unsigned long long int>(BDL_POS(pos)));
        std::cout << "X,Y Position from Pattern Header: " << patternHeader.xIndex << " , " << patternHeader.yIndex << std::endl;
        y = mapHeight;
        x = mapWidth;
        break;
      }
    }

    // Extend the dataset.
    std::array<hsize_t, 3> size = {static_cast<hsize_t>(mapWidth * (y + 1)), static_cast<hsize_t>(ebspHeight), static_cast<hsize_t>(ebspWidth)};
    status = H5Dset_extent(dataset, size.data());

    // Select a hyperslab.
    std::array<hsize_t, 3> offset = {static_cast<hsize_t>(mapWidth * y), 0, 0};
    filespace = H5Dget_space(dataset);
    status = H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offset.data(), nullptr, dims.data(), nullptr);

    // Define memory space
    dataspace = H5Screate_simple(patternRank, dims.data(), nullptr);

    // Write the data to the hyperslab.
    status = H5Dwrite(dataset, native_type, dataspace, filespace, H5P_DEFAULT, patternData.data());
  }
  // Close/release resources.
  H5Dclose(dataset);
  H5Sclose(dataspace);
  H5Sclose(filespace);
  H5Pclose(cparms);
  // Close our FrameData File
  fclose(f);

  std::cout << std::endl;
  std::cout.flush();
  return 0;
}

// -----------------------------------------------------------------------------
void BcfHdf5Convertor::execute()
{
  const bool k_ShowHdf5Errors = true;

  // We are going to construct a QTemporaryDir _templatepath_ variable so we use a temp
  // location next to the input file. This *should* be ok for most situations.
  // Qt will clean up the temp dir when it goes out of scope.
  QFileInfo ifInfo(m_InputFile);
  QString tmpPath = ifInfo.absolutePath() + "/" + ifInfo.baseName() + "_XXXXXX";
  QTemporaryDir tempDir(tmpPath);
  if(!tempDir.isValid())
  {
    m_ErrorCode = -7000;
    m_ErrorMessage = QString("Temp Directory could not be created.");
    return;
  }

  int32_t err = 0;

  hid_t fid = H5Utilities::createFile(m_OutputFile.toStdString());
  H5ScopedFileSentinel fileSentinel(fid, k_ShowHdf5Errors);

  // ***************************************************************************
  // IF ANYTHING IN HERE CHANGES YOU NEED TO INCREMENT THE FILEVERSION NUMBER
  // WHICH INDICATES THAT THE ORGANIZATION HAS BEED APPENDED/EDITED/REVISED.
  // ***************************************************************************
  err = H5Lite::writeScalarAttribute(fid, "/", "FileVersion", ::k_FileVersion);

  QFileInfo fi(m_InputFile);
  QString baseInputFileName = fi.completeBaseName();

  hid_t topGrpId = H5Utilities::createGroup(fid, baseInputFileName.toStdString());
  fileSentinel.addGroupId(topGrpId);

  hid_t ebsdGrpId = H5Utilities::createGroup(topGrpId, k_EBSD);
  fileSentinel.addGroupId(ebsdGrpId);

  hid_t dataGrpId = H5Utilities::createGroup(ebsdGrpId, k_Data);
  fileSentinel.addGroupId(dataGrpId);

  hid_t semGrpId = H5Utilities::createGroup(topGrpId, k_SEM);
  fileSentinel.addGroupId(semGrpId);

  hid_t headerGrpId = H5Utilities::createGroup(ebsdGrpId, k_Header);
  fileSentinel.addGroupId(headerGrpId);

  std::string manufacturer("DREAM.3D");
  err = H5Lite::writeStringDataset(fid, ::k_Manufacturer, manufacturer);
  if(err < 0)
  {
    m_ErrorCode = err;
    m_ErrorMessage = QString("Manufacturer Dataset was not written");
    return;
  }

  std::string version = "0.2.0";
  err = H5Lite::writeStringDataset(fid, ::k_Version, version);
  if(err < 0)
  {
    m_ErrorCode = err;
    m_ErrorMessage = QString("Version Dataset was not written");
    return;
  }

  SFSReader sfsFile;
  sfsFile.parseFile(m_InputFile.toStdString());
  QString outputFile;
  QTextStream outFileStrm(&outputFile);

  std::cout << "Using Temp Dir: " << tempDir.path().toStdString() << std::endl;

  outputFile.clear();
  outFileStrm << Bruker::Files::EBSDData << "/" << Bruker::Files::FrameDescription;
  QString descFile = outputFile;
  // std::cout << "Extracting FrameDescription";
  err = sfsFile.extractFile(tempDir.path().toStdString(), descFile.toStdString());
  if(err < 0)
  {
    m_ErrorCode = -7020;
    m_ErrorMessage = QString("Could not extract EBSDData/FrameDescription File.");
    return;
  }

  outputFile.clear();
  outFileStrm << Bruker::Files::EBSDData << "/" << Bruker::Files::IndexingResults;
  QString indexingResultsFile = outputFile;
  // std::cout << "Extracting IndexingResults";
  err = sfsFile.extractFile(tempDir.path().toStdString(), indexingResultsFile.toStdString());
  if(err < 0)
  {
    m_ErrorCode = -7030;
    m_ErrorMessage = QString("Could not extract EBSDData/IndexingResults File.");
    return;
  }

  outputFile.clear();
  outFileStrm << Bruker::Files::EBSDData << "/" << Bruker::Files::Auxiliarien;
  QString auxiliarienFile = outputFile;
  // std::cout << "Extracting Auxiliarien";
  err = sfsFile.extractFile(tempDir.path().toStdString(), auxiliarienFile.toStdString());
  if(err < 0)
  {
    m_ErrorCode = -7040;
    m_ErrorMessage = QString("Could not extract EBSDData/Auxiliarien File.");
    return;
  }

  outputFile.clear();
  outFileStrm << tempDir.path() << "/" << Bruker::Files::EBSDData;
  QString ebsdDataDir = outputFile;

  // Read the Dimensions of the Map and EBSP
  int32_t mapHeight;
  int32_t mapWidth;
  int32_t ebspHeight;
  int32_t ebspWidth;
  err = BrukerDataLoader::ReadScanSizes(ebsdDataDir, mapWidth, mapHeight, ebspWidth, ebspHeight);
  if(err < 0)
  {
    std::cout << "Error reading Scan Sizes from Description file: " << err << std::endl;
    m_ErrorCode = -7050;
    return;
  }
  auto numElements = static_cast<size_t>(mapWidth * mapHeight);
  std::vector<size_t> cDims = {2};
  std::vector<size_t> tDims = {numElements};

  std::vector<uint16_t> roi;

  // Scope this entire next part so that the arrays get cleaned up when we are done....
  {
    UInt16ArrayType::Pointer indices = UInt16ArrayType::CreateArray(numElements, cDims, "Positions", true);
    cDims[0] = 3;
    FloatArrayType::Pointer eulers = FloatArrayType::CreateArray(numElements, cDims, Bruker::IndexingResults::Eulers, true);

    cDims[0] = 1;
    FloatArrayType::Pointer patQual = FloatArrayType::CreateArray(numElements, cDims, Bruker::IndexingResults::RadonQuality, true);
    UInt16ArrayType::Pointer detectedBands = UInt16ArrayType::CreateArray(numElements, cDims, Bruker::IndexingResults::RadonBandCount, true);
    Int16ArrayType::Pointer phases = Int16ArrayType::CreateArray(numElements, cDims, Bruker::IndexingResults::Phase, true);
    UInt16ArrayType::Pointer indexedBands = UInt16ArrayType::CreateArray(numElements, cDims, Bruker::IndexingResults::NIndexedBands, true);
    FloatArrayType::Pointer bmm = FloatArrayType::CreateArray(numElements, cDims, Bruker::IndexingResults::MAD, true);

    descFile = tempDir.path() + "/" + Bruker::Files::EBSDData + "/" + Bruker::Files::FrameDescription;
    indexingResultsFile = tempDir.path() + "/" + Bruker::Files::EBSDData + "/" + Bruker::Files::IndexingResults;

    err = BrukerDataLoader::LoadIndexingResults(descFile, indexingResultsFile, indices, eulers, patQual, detectedBands, phases, indexedBands, bmm, mapWidth, mapHeight, roi, m_Reorder);
    if(err < 0)
    {
      m_ErrorCode = -7050;
      m_ErrorMessage = QString("Error Reading IndexingResults from extracted file: ");
      return;
    }

    // Write all the data to the HDF5 file with conversions
    Int32ArrayType::Pointer i32Array = Int32ArrayType::CreateArray(numElements, Bruker::IndexingResults::XBEAM, true);
    for(size_t i = 0; i < numElements; i++)
    {
      auto value = static_cast<int32_t>(indices->getComponent(i, 0));
      i32Array->setValue(i, value);
    }
    err = writeDataArrayToHdf5(i32Array.get(), dataGrpId, numElements);
    i32Array->setName(Bruker::SEM::SEMIX);
    err = writeDataArrayToHdf5(i32Array.get(), semGrpId, numElements);

    i32Array->setName(Bruker::IndexingResults::YBEAM);
    for(size_t i = 0; i < numElements; i++)
    {
      auto value = static_cast<int32_t>(indices->getComponent(i, 1));
      i32Array->setValue(i, value);
    }
    err = writeDataArrayToHdf5(i32Array.get(), dataGrpId, numElements);
    i32Array->setName(Bruker::SEM::SEMIY);
    err = writeDataArrayToHdf5(i32Array.get(), semGrpId, numElements);

    // Convert from a single array of Eulers to 3 separate arrays and convert to degrees .. why?
    std::vector<QString> names = {{Bruker::IndexingResults::phi1, Bruker::IndexingResults::PHI, Bruker::IndexingResults::phi2}};
    std::vector<int32_t> comp = {{0, 1, 2}};
    FloatArrayType::Pointer euler = FloatArrayType::CreateArray(numElements, Bruker::IndexingResults::phi1, true);
    for(size_t c = 0; c < 3; c++)
    {
      euler->setName(names.at(c));
      for(size_t i = 0; i < numElements; i++)
      {
        float value = eulers->getComponent(i, c) * SIMPLib::Constants::k_180OverPiD;
        euler->setValue(i, value);
      }
      err = writeDataArrayToHdf5(euler.get(), dataGrpId, numElements);
    }

    // This one doesn't need a conversion.
    err = writeDataArrayToHdf5(patQual.get(), dataGrpId, numElements);

    // Convert RadonBandCount
    i32Array->setName(Bruker::IndexingResults::RadonBandCount);
    for(size_t i = 0; i < numElements; i++)
    {
      auto value = static_cast<int32_t>(detectedBands->getValue(i));
      i32Array->setValue(i, value);
    }
    err = writeDataArrayToHdf5(i32Array.get(), dataGrpId, numElements);

    // Convert Phase
    i32Array->setName(Bruker::IndexingResults::Phase);
    for(size_t i = 0; i < numElements; i++)
    {
      auto value = static_cast<int32_t>(phases->getValue(i));
      i32Array->setValue(i, value);
    }
    err = writeDataArrayToHdf5(i32Array.get(), dataGrpId, numElements);

    // Convert NIndexedBands
    i32Array->setName(Bruker::IndexingResults::NIndexedBands);
    for(size_t i = 0; i < numElements; i++)
    {
      auto value = static_cast<int32_t>(indexedBands->getValue(i));
      i32Array->setValue(i, value);
    }
    err = writeDataArrayToHdf5(i32Array.get(), dataGrpId, numElements);

    // MAD doesn't need a conversion
    err = writeDataArrayToHdf5(bmm.get(), dataGrpId, numElements);
  }

  // Write all the Header information
  {
    outputFile.clear();
    outFileStrm << Bruker::Files::EBSDData << "/" << Bruker::Files::PhaseList;
    QString phaseListFile = outputFile;
    // std::cout << "Extracting PhaseList";
    err = sfsFile.extractFile(tempDir.path().toStdString(), phaseListFile.toStdString());
    phaseListFile = tempDir.path() + "/" + phaseListFile;
    if(err == 0)
    {
      err = H5Lite::writeScalarDataset(headerGrpId, Bruker::Header::NCOLS.toStdString(), mapWidth);
      err = H5Lite::writeScalarDataset(headerGrpId, Bruker::Header::NROWS.toStdString(), mapHeight);
      err = H5Lite::writeScalarDataset(headerGrpId, Bruker::Header::NPoints.toStdString(), numElements);
      err = H5Lite::writeStringDataset(headerGrpId, Bruker::Header::OriginalFile.toStdString(), m_InputFile.toStdString());
      err = H5Lite::writeScalarDataset(headerGrpId, Bruker::Header::PatternWidth.toStdString(), ebspWidth);
      err = H5Lite::writeScalarDataset(headerGrpId, Bruker::Header::PatternHeight.toStdString(), ebspHeight);
      err = H5Lite::writeStringDataset(headerGrpId, Bruker::Header::GridType.toStdString(), Bruker::Header::isometric.toStdString());
      double zOffset = 0.0;
      err = H5Lite::writeScalarDataset(headerGrpId, Bruker::Header::ZOffset.toStdString(), zOffset);
      writePhaseInformation(headerGrpId, phaseListFile);
    }
  }

  // Write the SEM Data
  {
    outputFile.clear();
    outFileStrm << Bruker::Files::EBSDData << "/" << Bruker::Files::SEMImage;
    QString semFile = outputFile;
    // std::cout << "Extracting SEIMImage";
    err = sfsFile.extractFile(tempDir.path().toStdString(), semFile.toStdString());
    if(err < 0)
    {
      m_ErrorCode = -7060;
      m_ErrorMessage = QString("Could not extract EBSDData/SEMImage File.");
      return;
    }
    semFile = tempDir.path() + "/" + semFile;

    writeSEMData(semGrpId, headerGrpId, semFile);
  }

  // Write the Calibration Data
  {
    outputFile.clear();
    outFileStrm << Bruker::Files::EBSDData << "/" << Bruker::Files::Calibration;
    QString semFile = outputFile;
    // std::cout << "Extracting Calibration";
    err = sfsFile.extractFile(tempDir.path().toStdString(), semFile.toStdString());
    if(err < 0)
    {
      m_ErrorCode = -7060;
      m_ErrorMessage = QString("Could not extract EBSDData/Calibration File.");
      return;
    }
    semFile = tempDir.path() + "/" + semFile;

    float pcx = 0.0f;
    float pcy = 0.0f;
    writeCalibrationData(semGrpId, headerGrpId, semFile, pcx, pcy);

    std::vector<hsize_t> dims = { static_cast<hsize_t> (mapHeight * mapWidth)};
    {
      std::vector<float> patternCenter(mapWidth * mapHeight, pcx);
      err = H5Lite::writeVectorDataset(dataGrpId, "PCX", dims, patternCenter);
    }
    {
      std::vector<float> patternCenter(mapWidth * mapHeight, pcy);
      err = H5Lite::writeVectorDataset(dataGrpId, "PCY", dims, patternCenter);
    }

  }

  // Write the AuxIndexingOptions Data
  {
    outputFile.clear();
    outFileStrm << Bruker::Files::EBSDData << "/" << Bruker::Files::AuxIndexingOptions;
    QString semFile = outputFile;
    // std::cout << "Extracting AuxIndexingOptions";
    err = sfsFile.extractFile(tempDir.path().toStdString(), semFile.toStdString());
    if(err < 0)
    {
      m_ErrorCode = -7060;
      m_ErrorMessage = QString("Could not extract EBSDData/AuxIndexingOptions File.");
      return;
    }
    semFile = tempDir.path() + "/" + semFile;

    writeAuxIndexingOptions(semGrpId, headerGrpId, semFile);
  }

  int32_t pixelByteCount = 0;
  // Write the CameraConfiguration Data
  {
    outputFile.clear();
    outFileStrm << Bruker::Files::EBSDData << "/" << Bruker::Files::CameraConfiguration;
    QString semFile = outputFile;
    // std::cout << "Extracting CameraConfiguration";
    err = sfsFile.extractFile(tempDir.path().toStdString(), semFile.toStdString());
    if(err < 0)
    {
      m_ErrorCode = -7060;
      m_ErrorMessage = QString("Could not extract EBSDData/CameraConfiguration File.");
      return;
    }
    semFile = tempDir.path() + "/" + semFile;

    writeCameraConfiguration(semGrpId, headerGrpId, semFile);
    // Get the Pattern Pixel Byte Count
    err = H5Lite::readScalarDataset(headerGrpId, "PixelByteCount", pixelByteCount);
  }

  outputFile.clear();
  outFileStrm << Bruker::Files::EBSDData << "/" << Bruker::Files::FrameData;
  QString dataFile = outputFile;
  if(pixelByteCount == 1)
  {
    writePatternData<uint8_t>(sfsFile, H5T_NATIVE_UINT8, mapWidth, mapHeight, ebspWidth, ebspHeight, m_FlipPatterns, tempDir.path(), dataFile, descFile, dataGrpId);
  }
  else if(pixelByteCount == 2)
  {
    writePatternData<uint16_t>(sfsFile, H5T_NATIVE_UINT16, mapWidth, mapHeight, ebspWidth, ebspHeight, m_FlipPatterns, tempDir.path(), dataFile, descFile, dataGrpId);
  }
}

// -----------------------------------------------------------------------------
int32_t BcfHdf5Convertor::getErrorCode() const
{
  return m_ErrorCode;
}

// -----------------------------------------------------------------------------
QString BcfHdf5Convertor::getErrorMessage() const
{
  return m_ErrorMessage;
}
