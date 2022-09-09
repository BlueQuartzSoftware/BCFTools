// Qt Includes
#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>

#include "BcfHdf5Convertor.h"

#include <iostream>

int main(int argc, char* argv[])
{
  QString version("1.0.0");
  // Instantiate the QCoreApplication that we need to get the current path and load plugins.
  QCoreApplication* app = new QCoreApplication(argc, argv);
  QCoreApplication::setOrganizationName("BlueQuartz Software");
  QCoreApplication::setOrganizationDomain("bluequartz.net");
  QCoreApplication::setApplicationName("bcf2hdf5");
  QCoreApplication::setApplicationVersion(version);

  QCommandLineParser parser;
  QString str;
  QTextStream ss(&str);
  ss << "bcf2hdf5 (" << version << "): This application will convert a Bruker Nano .bcf file into a somewhat compatible HDF5 file.";
  parser.setApplicationDescription(str);
  parser.addHelpOption();
  parser.addVersionOption();

  QCommandLineOption bcfFileArg(QStringList() << "b"
                                              << "bcf",
                                "Input BCF <file>", "file");
  parser.addOption(bcfFileArg);

  QCommandLineOption hdf5FileArg(QStringList() << "o"
                                               << "output",
                                 "Output HDF5 <file>", "file");
  parser.addOption(hdf5FileArg);

  QCommandLineOption reorderArg(QStringList() << "r"
                                              << "reorder",
                                "Reorder Data inside of HDF5 file. This can increase final file size significantly. true or false.", "file");
  parser.addOption(reorderArg);

  QCommandLineOption flipPatternArg(QStringList() << "f"
                                              << "flip",
                                "Flip the patterns across the X Axis (Vertical Flip). true or false.", "file");
  parser.addOption(flipPatternArg);

  // Process the actual command line arguments given by the user
  parser.process(*app);

  if(argc != 9)
  {
    std::cout << "7 Arguments are required. Use --help for more information." << std::endl;
    return EXIT_FAILURE;
  }

  QString inputFile = parser.value(bcfFileArg);
  if(inputFile.isEmpty())
  {
    std::cout << "Input file was empty. Use --help for more information." << std::endl;
    return EXIT_FAILURE;
  }
  QString outputFile = parser.value(hdf5FileArg);
  if(outputFile.isEmpty())
  {
    std::cout << "Output file was empty. Use --help for more information." << std::endl;
    return EXIT_FAILURE;
  }

  QString reorder = parser.value(reorderArg);
  QString flipPatterns = parser.value(flipPatternArg);

  BcfHdf5Convertor convertor(inputFile, outputFile);
  convertor.setReorder(reorder == "true");
  convertor.setFlipPatterns(flipPatterns == "true");
  convertor.execute();
  int32_t err = convertor.getErrorCode();
  if(err < 0)
  {
    std::cout << convertor.getErrorMessage().toStdString() << ": " << convertor.getErrorCode() << std::endl;
  }

  std::cout << "Complete" << std::endl;

  /* code */
  return err;
}
