#include "MantidAlgorithms/PDFFourierTransform.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"

#include <boost/math/special_functions/fpclassify.hpp>
#include <cmath>
#include <sstream>

namespace Mantid {
namespace Algorithms {

using std::string;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PDFFourierTransform)

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace { // anonymous namespace
/// Crystalline PDF
const string BIG_G_OF_R("G(r)");
/// Liquids PDF
const string LITTLE_G_OF_R("g(r)");
/// Radial distribution function
const string RDF_OF_R("RDF(r)");

/// Normalized intensity
const string S_OF_Q("S(Q)");
/// Asymptotes to zero
const string S_OF_Q_MINUS_ONE("S(Q)-1");
/// Kernel of the Fourier transform
const string Q_S_OF_Q_MINUS_ONE("Q[S(Q)-1]");
}

const std::string PDFFourierTransform::name() const {
  return "PDFFourierTransform";
}

int PDFFourierTransform::version() const { return 1; }

const std::string PDFFourierTransform::category() const {
  return "Diffraction\\Utility";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
*/
void PDFFourierTransform::init() {
  auto uv = boost::make_shared<API::WorkspaceUnitValidator>("MomentumTransfer");

  declareProperty(make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                   Direction::Input, uv),
                  S_OF_Q + ", " + S_OF_Q_MINUS_ONE + ", or " +
                      Q_S_OF_Q_MINUS_ONE);
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "Result paired-distribution function");

  // Set up input data type
  std::vector<std::string> inputTypes;
  inputTypes.push_back(S_OF_Q);
  inputTypes.push_back(S_OF_Q_MINUS_ONE);
  inputTypes.push_back(Q_S_OF_Q_MINUS_ONE);
  declareProperty("InputSofQType", S_OF_Q,
                  boost::make_shared<StringListValidator>(inputTypes),
                  "To identify whether input function");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);

  declareProperty(
      "Qmin", EMPTY_DBL(), mustBePositive,
      "Minimum Q in S(Q) to calculate in Fourier transform (optional).");
  declareProperty(
      "Qmax", EMPTY_DBL(), mustBePositive,
      "Maximum Q in S(Q) to calculate in Fourier transform. (optional)");
  declareProperty("Filter", false,
                  "Set to apply Lorch function filter to the input");

  // Set up output data type
  std::vector<std::string> outputTypes;
  outputTypes.push_back(BIG_G_OF_R);
  outputTypes.push_back(LITTLE_G_OF_R);
  outputTypes.push_back(RDF_OF_R);
  declareProperty("PDFType", BIG_G_OF_R,
                  boost::make_shared<StringListValidator>(outputTypes),
                  "Type of output PDF including G(r)");

  declareProperty("DeltaR", EMPTY_DBL(), mustBePositive,
                  "Step size of r of G(r) to calculate.  Default = "
                  ":math:`\\frac{\\pi}{Q_{max}}`.");
  declareProperty("Rmax", 20., mustBePositive,
                  "Maximum r for G(r) to calculate.");
  declareProperty(
      "rho0", EMPTY_DBL(), mustBePositive,
      "Average number density used for g(r) and RDF(r) conversions (optional)");

  string recipGroup("Reciprocal Space");
  setPropertyGroup("InputSofQType", recipGroup);
  setPropertyGroup("Qmin", recipGroup);
  setPropertyGroup("Qmax", recipGroup);
  setPropertyGroup("Filter", recipGroup);

  string realGroup("Real Space");
  setPropertyGroup("PDFType", realGroup);
  setPropertyGroup("DeltaR", realGroup);
  setPropertyGroup("Rmax", realGroup);
  setPropertyGroup("rho0", realGroup);
}

std::map<string, string> PDFFourierTransform::validateInputs() {
  std::map<string, string> result;

  double Qmin = getProperty("Qmin");
  double Qmax = getProperty("Qmax");
  if ((!isEmpty(Qmin)) && (!isEmpty(Qmax)))
    if (Qmax <= Qmin)
      result["Qmax"] = "Must be greater than Qmin";

  // check for null pointers - this is to protect against workspace groups
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  if (!inputWS) {
    return result;
  }

  if (inputWS->getNumberHistograms() != 1) {
    result["InputWorkspace"] = "Input workspace must have only one spectrum";
  }

  return result;
}

size_t PDFFourierTransform::determineQminIndex() {
  // get input data
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const auto &inputQ = inputWS->dataX(0);    //  x for input
  const auto &inputFOfQ = inputWS->dataY(0); //  y for input
  double qmin = getProperty("Qmin");

  // check against available Q-range
  if (isEmpty(qmin)) {
    qmin = inputQ.front();
  } else if (qmin < inputQ.front()) {
    g_log.information(
        "Specified Qmin < range of data. Adjusting to data range.");
    qmin = inputQ.front();
  }

  // get index for the Qmin from the Q-range
  auto q_iter = std::upper_bound(inputQ.begin(), inputQ.end(), qmin);
  size_t qmin_index = std::distance(inputQ.begin(), q_iter);
  if (qmin_index == 0)
    qmin_index += 1; // so there doesn't have to be a check in integration loop

  // go to first non-nan value
  q_iter = std::find_if(std::next(inputFOfQ.begin(), qmin_index),
                        inputFOfQ.end(), boost::math::isnormal<double>);
  size_t first_normal_index = std::distance(inputFOfQ.begin(), q_iter);
  if (first_normal_index > qmin_index) {
    g_log.information(
        "Specified Qmin where data is nan/inf. Adjusting to data range.");
    qmin_index = first_normal_index;
  }

  return qmin_index;
}

size_t PDFFourierTransform::determineQmaxIndex() {
  // get input data
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const auto &inputQ = inputWS->dataX(0);    //  x for input
  const auto &inputFOfQ = inputWS->dataY(0); //  y for input
  double qmax = getProperty("Qmax");

  // check against available Q-range
  if (isEmpty(qmax)) {
    qmax = inputQ.back();
  } else if (qmax > inputQ.back()) {
    g_log.information()
        << "Specified Qmax > range of data. Adjusting to data range.\n";
    qmax = inputQ.back();
  }

  // get pointers for the data range
  auto q_iter = std::lower_bound(inputQ.begin(), inputQ.end(), qmax);
  size_t qmax_index = std::distance(inputQ.begin(), q_iter);

  // go to first non-nan value
  auto q_back_iter = std::find_if(inputFOfQ.rbegin(), inputFOfQ.rend(),
                                  boost::math::isnormal<double>);
  size_t first_normal_index =
      inputFOfQ.size() - std::distance(inputFOfQ.rbegin(), q_back_iter) - 1;
  if (first_normal_index < qmax_index) {
    g_log.information(
        "Specified Qmax where data is nan/inf. Adjusting to data range.");
    qmax_index = first_normal_index;
  }

  return qmax_index;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
*/
void PDFFourierTransform::exec() {
  // get input data
  API::MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  MantidVec inputQ = inputWS->dataX(0);                   //  x for input
  HistogramData::HistogramDx inputDQ(inputQ.size(), 0.0); // dx for input
  if (inputWS->sharedDx(0))
    inputDQ = inputWS->dx(0);
  MantidVec inputFOfQ = inputWS->dataY(0);  //  y for input
  MantidVec inputDfOfQ = inputWS->dataE(0); // dy for input

  // transform input data into Q/MomentumTransfer
  const std::string inputXunit = inputWS->getAxis(0)->unit()->unitID();
  if (inputXunit == "MomentumTransfer") {
    // nothing to do
  } else if (inputXunit == "dSpacing") {
    // convert the x-units to Q/MomentumTransfer
    const double PI_2(2. * M_PI);
    std::for_each(inputQ.begin(), inputQ.end(),
                  [&PI_2](double &Q) { Q /= PI_2; });
    std::transform(inputDQ.begin(), inputDQ.end(), inputQ.begin(),
                   inputDQ.begin(), std::divides<double>());
    // reverse all of the arrays
    std::reverse(inputQ.begin(), inputQ.end());
    std::reverse(inputDQ.begin(), inputDQ.end());
    std::reverse(inputFOfQ.begin(), inputFOfQ.end());
    std::reverse(inputDfOfQ.begin(), inputDfOfQ.end());
  } else {
    std::stringstream msg;
    msg << "Input data x-axis with unit \"" << inputXunit
        << "\" is not supported (use \"MomentumTransfer\" or \"dSpacing\")";
    throw std::invalid_argument(msg.str());
  }
  g_log.debug() << "Input unit is " << inputXunit << "\n";

  // convert from histogram to density
  if (!inputWS->isHistogramData()) {
    g_log.warning() << "This algorithm has not been tested on density data "
                       "(only on histograms)\n";
    /* Don't do anything for now
    double deltaQ;
    for (size_t i = 0; i < inputFOfQ.size(); ++i)
    {
    deltaQ = inputQ[i+1] -inputQ[i];
    inputFOfQ[i] = inputFOfQ[i]*deltaQ;
    inputDfOfQ[i] = inputDfOfQ[i]*deltaQ; // TODO feels wrong
    inputQ[i] += -.5*deltaQ;
    inputDQ[i] += .5*(inputDQ[i] + inputDQ[i+1]); // TODO running average
    }
    inputQ.push_back(inputQ.back()+deltaQ);
    inputDQ.push_back(inputDQ.back()); // copy last value
    */
  }

  // convert to Q[S(Q)-1]
  string soqType = getProperty("InputSofQType");
  if (soqType == S_OF_Q) {
    g_log.information() << "Subtracting one from all values\n";
    // there is no error propagation for subtracting one
    std::for_each(inputFOfQ.begin(), inputFOfQ.end(), [](double &F) { F--; });
    soqType = S_OF_Q_MINUS_ONE;
  }
  if (soqType == S_OF_Q_MINUS_ONE) {
    g_log.information() << "Multiplying all values by Q\n";
    // error propagation
    for (size_t i = 0; i < inputDfOfQ.size(); ++i) {
      inputDfOfQ[i] = inputQ[i] * inputDfOfQ[i] + inputFOfQ[i] * inputDQ[i];
    }
    // convert the function
    std::transform(inputFOfQ.begin(), inputFOfQ.end(), inputQ.begin(),
                   inputFOfQ.begin(), std::multiplies<double>());
    soqType = Q_S_OF_Q_MINUS_ONE;
  }
  if (soqType != Q_S_OF_Q_MINUS_ONE) {
    // should never get here
    std::stringstream msg;
    msg << "Do not understand InputSofQType = " << soqType;
    throw std::runtime_error(msg.str());
  }

  // determine Q-range
  size_t qmin_index = determineQminIndex();
  size_t qmax_index = determineQmaxIndex();
  { // keep variable scope small
    size_t qmi_out = qmax_index;
    if (qmi_out == inputQ.size())
      qmi_out--; // prevent unit test problem under windows (and probably other
    // hardly identified problem)
    g_log.notice() << "Adjusting to data: Qmin = " << inputQ[qmin_index]
                   << " Qmax = " << inputQ[qmi_out] << "\n";
  }

  // determine r axis for result
  const double rmax = getProperty("RMax");
  double rdelta = getProperty("DeltaR");
  if (isEmpty(rdelta))
    rdelta = M_PI / inputQ[qmax_index];
  size_t sizer = static_cast<size_t>(rmax / rdelta);

  bool filter = getProperty("Filter");

  // create the output workspace
  API::MatrixWorkspace_sptr outputWS =
      WorkspaceFactory::Instance().create("Workspace2D", 1, sizer, sizer);
  outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("Label");
  Unit_sptr unit = outputWS->getAxis(0)->unit();
  boost::shared_ptr<Units::Label> label =
      boost::dynamic_pointer_cast<Units::Label>(unit);
  label->setLabel("AtomicDistance", "Angstrom");
  outputWS->setYUnitLabel("PDF");

  outputWS->mutableRun().addProperty("Qmin", inputQ[qmin_index], "Angstroms^-1",
                                     true);
  outputWS->mutableRun().addProperty("Qmax", inputQ[qmax_index], "Angstroms^-1",
                                     true);

  MantidVec &outputR = outputWS->dataX(0);
  for (size_t i = 0; i < sizer; i++) {
    outputR[i] = rdelta * static_cast<double>(1 + i);
  }
  g_log.information() << "Using rmin = " << outputR.front()
                      << "Angstroms and rmax = " << outputR.back()
                      << "Angstroms\n";
  // always calculate G(r) then convert
  MantidVec &outputY = outputWS->dataY(0);
  MantidVec &outputE = outputWS->dataE(0);

  // do the math
  for (size_t r_index = 0; r_index < sizer; r_index++) {
    const double r = outputR[r_index];
    double fs = 0;
    double error = 0;
    for (size_t q_index = qmin_index; q_index < qmax_index; q_index++) {
      double q = inputQ[q_index];
      double deltaq = inputQ[q_index] - inputQ[q_index - 1];
      double sinus = sin(q * r) * deltaq;

      // multiply by filter function sin(q*pi/qmax)/(q*pi/qmax)
      if (filter && q != 0) {
        sinus *= sin(q * rdelta) / (q * rdelta);
      }
      fs += sinus * inputFOfQ[q_index];
      error += (sinus * inputDfOfQ[q_index]) * (sinus * inputDfOfQ[q_index]);
      // g_log.debug() << "q[" << i << "] = " << q << "  dq = " << deltaq << "
      // S(q) =" << s;
      // g_log.debug() << "  d(gr) = " << temp << "  gr = " << gr << '\n';
    }

    // put the information into the output
    outputY[r_index] = fs * 2 / M_PI;
    outputE[r_index] = sqrt(error) * 2 / M_PI;
  }

  // convert to the correct form of PDF
  string pdfType = getProperty("PDFType");
  double rho0 = getProperty("rho0");
  if (isEmpty(rho0)) {
    const Kernel::Material &material = inputWS->sample().getMaterial();
    double materialDensity = material.numberDensity();

    if (!isEmpty(materialDensity) && materialDensity > 0)
      rho0 = materialDensity;
    else
      rho0 = 1.;
    // write out that it was reset if the value is coming into play
    if (pdfType == LITTLE_G_OF_R || pdfType == RDF_OF_R)
      g_log.information() << "Using rho0 = " << rho0 << "\n";
  }
  if (pdfType == BIG_G_OF_R) {
    // nothing to do
  } else if (pdfType == LITTLE_G_OF_R) {
    const double factor = 1. / (4. * M_PI * rho0);
    for (size_t i = 0; i < outputY.size(); ++i) {
      // error propagation - assuming uncertainty in r = 0
      outputE[i] = outputE[i] / outputR[i];
      // transform the data
      outputY[i] = 1. + factor * outputY[i] / outputR[i];
    }
  } else if (pdfType == RDF_OF_R) {
    const double factor = 4. * M_PI * rho0;
    for (size_t i = 0; i < outputY.size(); ++i) {
      // error propagation - assuming uncertainty in r = 0
      outputE[i] = outputE[i] * outputR[i];
      // transform the data
      outputY[i] = outputR[i] * outputY[i] + factor * outputR[i] * outputR[i];
    }
  } else {
    // should never get here
    std::stringstream msg;
    msg << "Do not understand PDFType = " << pdfType;
    throw std::runtime_error(msg.str());
  }

  // set property
  setProperty("OutputWorkspace", outputWS);
}

} // namespace Mantid
} // namespace Algorithms
