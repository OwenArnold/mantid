#ifndef MANTID_CUSTOMINTERFACES_POLARIZATIONCORRECTIONS_H_
#define MANTID_CUSTOMINTERFACES_POLARIZATIONCORRECTIONS_H_
#include "../DllConfig.h"
#include <boost/optional.hpp>
#include <stdexcept>
#include <string>
namespace MantidQt {
namespace CustomInterfaces {
enum class PolarizationCorrectionType { None, PA, PNR, ParameterFile };

inline PolarizationCorrectionType
polarizationCorrectionTypeFromString(std::string const &correctionType) {
  if (correctionType == "None")
    return PolarizationCorrectionType::None;
  else if (correctionType == "PA")
    return PolarizationCorrectionType::PA;
  else if (correctionType == "PNR")
    return PolarizationCorrectionType::PNR;
  else if (correctionType == "ParameterFile")
    return PolarizationCorrectionType::ParameterFile;
  else
    throw std::runtime_error("Unexpected polarization correction type.");
}

class MANTIDQT_ISISREFLECTOMETRY_DLL PolarizationCorrections {
public:
  PolarizationCorrections(PolarizationCorrectionType correctionType,
                          boost::optional<double> CRho,
                          boost::optional<double> CAlpha,
                          boost::optional<double> CAp,
                          boost::optional<double> CPp);

  PolarizationCorrectionType correctionType();
  boost::optional<double> cRho() const;
  boost::optional<double> cAlpha() const;
  boost::optional<double> cAp() const;
  boost::optional<double> cPp() const;
  bool enableInputs() const;

private:
  PolarizationCorrectionType m_correctionType;
  boost::optional<double> m_cRho;
  boost::optional<double> m_cAlpha;
  boost::optional<double> m_cAp;
  boost::optional<double> m_cPp;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool
operator==(PolarizationCorrections const &lhs,
           PolarizationCorrections const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool
operator!=(PolarizationCorrections const &lhs,
           PolarizationCorrections const &rhs);
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_POLARIZATIONCORRECTIONS_H_
