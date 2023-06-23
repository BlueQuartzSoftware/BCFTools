/*
 * Your License should go here
 */
#pragma once

#include <string>

/**
* @brief This namespace is used to define some Constants for the plugin itself.
*/
namespace BrukerIntegrationConstants
{
  const std::string BrukerIntegrationPluginFile("BrukerIntegrationPlugin");
  const std::string BrukerIntegrationPluginDisplayName("BrukerIntegration");
  const std::string BrukerIntegrationBaseName("BrukerIntegration");

  namespace FilterGroups
  {
    const std::string BrukerIntegrationFilters("Bruker Integration");
  }
}

/**
* @brief Use this namespace to define any custom GUI widgets that collect FilterParameters
* for a filter. Do NOT define general reusable widgets here.
*/
namespace FilterParameterWidgetType
{

/*  const std::string SomeCustomWidget("SomeCustomWidget"); */

}



namespace Bruker
{
  namespace Files
  {
  static const std::string EBSDData("EBSDData");
  static const std::string Auxiliarien("Auxiliarien");
  static const std::string AuxIndexingOptions("AuxIndexingOptions");
  static const std::string Calibration("Calibration");
  static const std::string CameraConfiguration("CameraConfiguration");
  static const std::string CameraLifeConfig("CameraLifeConfig");
  static const std::string FrameData("FrameData");
  static const std::string FrameDescription("FrameDescription");
  static const std::string ImgPreparation("ImgPreparation");
  static const std::string IndexingResults("IndexingResults");
  static const std::string Miszellaneen("Miszellaneen");
  static const std::string PhaseList("PhaseList");
  static const std::string RadonLines("RadonLines");
  static const std::string SEMImage("SEMImage");
  static const std::string SEMStageData("SEMStageData");
  static const std::string ShownPhaseList("ShownPhaseList");
  static const std::string Version("Version");

  }

  namespace IndexingResults
  {
  static const std::string XBEAM("X BEAM");
  static const std::string YBEAM("Y BEAM");
  static const std::string RadonQuality("RadonQuality");
  static const std::string RadonBandCount("RadonBandCount");
  static const std::string Eulers("EulerAngles");
  static const std::string Phase("Phase");
  static const std::string NIndexedBands("NIndexedBands");
  static const std::string MAD("MAD");
  static const std::string EBSP("RawPatterns");
  static const std::string phi1("phi1");
  static const std::string PHI("PHI");
  static const std::string phi2("phi2");
  } // namespace IndexingResults

  namespace Header
  {
  static const std::string NCOLS("NCOLS");
  static const std::string NPoints("NPoints");
  static const std::string NROWS("NROWS");
  static const std::string OriginalFile("OriginalFile");
  static const std::string PatternHeight("PatternHeight");
  static const std::string PatternWidth("PatternWidth");
  static const std::string GridType("Grid Type");
  static const std::string isometric("isometric");
  static const std::string Phases("Phases");
  static const std::string ZOffset("ZOffset");
  } // namespace Header

  namespace SEM {
    static const std::string SEM("SEM");
    static const std::string SEMIX("SEM IX");
    static const std::string SEMIY("SEM IY");
  }
}


namespace BlueQuartz
{
  const std::string VendorName("BlueQuartz Software, LLC");
  const std::string URL("http://www.bluequartz.net");
  const std::string Copyright("(C) 2016-2019 BlueQuartz Software, LLC");
}



