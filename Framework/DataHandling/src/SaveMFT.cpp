#include "MantidDataHandling/SaveMFT.h"
#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ListValidator.h"

#include <Poco/File.h>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <cmath>
#include <iomanip>
#include <limits>
#include <map>
#include <stdexcept>
#include <vector>

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveMFT)

/// Initialise the algorithm
void SaveMFT::init() {
  declareProperty(Kernel::make_unique<ArrayProperty<std::string>>(
                      "InputWorkspace", boost::make_shared<ADSValidator>()),
                  "List or group of MatrixWorkspaces");
  declareProperty(
      make_unique<FileProperty>("Filename", "", FileProperty::Save, ".mft"),
      "The output filename.");
  declareProperty(make_unique<ArrayProperty<std::string>>("LogList"),
                  "List of logs to write to file.");
  declareProperty("WriteHeader", true, "Whether to write header lines.");
}

/// Input validation
std::map<std::string, std::string> SaveMFT::validateInputs() {
  std::map<std::string, std::string> issues;
  const std::string wsName = getProperty("InputWorkspace");
  m_ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);
  if (m_ws) {
    // MatrixWorkspace
    try {
      m_ws->y(0).size();
    } catch (std::range_error) {
      issues["InputWorkspace"] = "InputWorkspace does not contain data";
    }
  } else {
    m_group =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(wsName);
    if (m_group) {
      // WorkspaceGroup
      if (m_group->isEmpty())
        issues["InputWorkspace"] = "Empty WorkspaceGroup";
      for (auto i : m_group->getAllItems()) {
        try {
          boost::dynamic_pointer_cast<MatrixWorkspace>(i)->y(0).size();
        } catch (std::range_error) {
          issues["InputWorkspace"] =
              "InputWorkspace " + i->getName() + " does not contain data";
          m_group->remove(i->getName());
        }
      }
    } else {
      issues["InputWorkspace"] = "The input workspace is neither a "
                                 "WorkspaceGroup nor a MatrixWorkspace";
    }
  }
  return issues;
}

/// Write data to file
void SaveMFT::data() {
  m_file << std::scientific;
  m_file << std::setprecision(std::numeric_limits<double>::digits10);
  const auto points = m_ws->points(0);
  const auto &yData = m_ws->y(0);
  const auto &eData = m_ws->e(0);
  for (size_t i = 0; i < m_ws->y(0).size(); ++i) {
    outputval(points[i]);
    outputval(yData[i]);
    outputval(eData[i]);
    if (m_ws->hasDx(0))
      outputval(m_ws->dx(0)[i]);
    m_file << '\n';
  }
}

/** Write formatted line of data
 *  @param val :: the double value to be written
 */
void SaveMFT::outputval(double val) {
  m_file << std::setw(28);
  if (!std::isnan(val) && !std::isinf(val))
    m_file << val;
  else if (std::isinf(val))
    m_file << "inf";
  else
    m_file << "nan";
}

/** Write formatted line of data
 *  @param val :: a string value to be written
 */
void SaveMFT::outputval(std::string val) { m_file << std::setw(28) << val; }

/// Retrieve sample log information
std::string SaveMFT::sampleInfo(const std::string &logName) {
  auto run = m_ws->run();
  try {
    return boost::lexical_cast<std::string>(run.getLogData(logName)->value());
  } catch (Exception::NotFoundError) {
  }
  return "Not defined";
}

/// Write one header line
void SaveMFT::writeInfo(const std::string logName, const std::string logValue) {
  if (!logValue.empty())
    m_file << logName << " : " << sampleInfo(logValue) << '\n';
  else
    m_file << logName << " : " << sampleInfo(logName) << '\n';
}

/// Write header lines
void SaveMFT::header() {
  m_file << std::setfill(' ');
  m_file << "MFT\n";
  std::map<std::string, std::string> logs;
  logs["Instrument"] = "instrument.name";
  logs["User-local contact"] = "user.namelocalcontact";
  logs["Title"] = "title";
  logs["Subtitle"] = "";
  logs["Start date + time"] = "start_time";
  logs["End date + time"] = "end_time";
  logs["Theta 1 + dir + ref numbers"] = "";
  logs["Theta 2 + dir + ref numbers"] = "";
  logs["Theta 3 + dir + ref numbers"] = "";
  writeInfo("Instrument", "instrument.name");
  writeInfo("User-local contact", "user.namelocalcontact");
  writeInfo("Title", "title");
  writeInfo("Subtitle", "");
  writeInfo("Start date + time", "start_time");
  writeInfo("End date + time", "end_time");
  writeInfo("Theta 1 + dir + ref numbers", "");
  writeInfo("Theta 2 + dir + ref numbers", "");
  writeInfo("Theta 3 + dir + ref numbers", "");
  const std::vector<std::string> logList = getProperty("LogList");
  int nlogs = 0;
  for (const auto &log : logList) {
    if (logs.find(log) == logs.end()) {
      writeInfo(log);
      ++nlogs;
    }
  }
  for (auto i = nlogs + 1; i < 10; ++i)
    writeInfo("Parameter ");
  m_file << "Number of file format : "
         << "40\n";
  m_file << "Number of data points : " << m_ws->y(0).size() << '\n';
  m_file << '\n';
  outputval("q");
  outputval("refl");
  outputval("refl_err");
  if (m_ws->hasDx(0))
    outputval("q_res (FWHM)");
  m_file << "\n";
}

/// Check file
void SaveMFT::checkFile(const std::string filename) {
  if (Poco::File(filename).exists()) {
    g_log.warning("File already exists and will be overwritten");
    try {
      Poco::File(filename).remove();
    } catch (...) { // maybe we do not have the permission to delete the file
      g_log.error("Error deleting file " + filename);
    }
  }
  m_file.open(filename);
  if (!m_file.is_open()) {
    g_log.error("Unable to create file: " + filename);
  }
  g_log.information("Filename: " + filename);
}

/// Execute the algorithm
void SaveMFT::exec() {
  const std::string filename = getProperty("Filename");
  if (m_ws) {
    checkFile(filename);
    if (getProperty("WriteHeader"))
      header();
    data();
    m_file.close();
  } else {
    for (auto i = 0u; i < m_group->size(); ++i) {
      auto ws = m_group->getItem(i);
      m_ws = boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
      const std::string uniqueFilename =
          filename.substr(0, filename.find(".")) + m_ws->getName() +
          filename.substr(filename.find("."));
      checkFile(uniqueFilename);
      if (getProperty("WriteHeader"))
        header();
      data();
      m_file.close();
    }
  }
}

} // namespace DataHandling
} // namespace Mantid
