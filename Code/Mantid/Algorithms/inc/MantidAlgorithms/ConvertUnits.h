#ifndef MANTID_ALGORITHMS_CONVERTUNITS_H_
#define MANTID_ALGORITHMS_CONVERTUNITS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** Converts the units in which a workspace is represented.
    Only implemented for histogram data, so far.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. Can be the same as the input one. </LI>
    <LI> Target          - The units to which the workspace should be converted. </LI>
    </UL>

    Optional properties required for certain units (DeltaE & DeltaE_inWavenumber):
    <UL>
    <LI> Emode  - The energy mode (0=elastic, 1=direct geometry, 2=indirect geometry)</LI>
    <LI> Efixed - Value of fixed energy: EI (emode=1) or EF (emode=2) (meV) </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 06/03/2008

    Copyright &copy; 2008 STFC Rutherford Appleton Laboratories

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport ConvertUnits : public API::Algorithm
{
public:
  /// Default constructor
	ConvertUnits();
	/// Virtual destructor
	virtual ~ConvertUnits();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "ConvertUnits"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual const std::string version() const { return "1"; }
	
private:
  // Overridden Algorithm methods
  void init();
  void exec();
  
  void convertQuickly(const int& numberOfSpectra, API::Workspace_sptr outputWS, const double& factor, const double& power);
  void convertViaTOF(const int& numberOfSpectra, API::Workspace_sptr inputWS, API::Workspace_sptr outputWS);
  
  /// Static reference to the logger class
  static Kernel::Logger& g_log;
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CONVERTUNITS_H_*/
