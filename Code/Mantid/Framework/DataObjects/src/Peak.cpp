#include "MantidDataObjects/Peak.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/System.h"
#include "MantidGeometry/Objects/InstrumentRayTracer.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace Mantid
{
namespace DataObjects
{


  //----------------------------------------------------------------------------------------------
  /** Constructor that uses the Q position of the peak (in the lab frame).
   * No detector ID is set.
   *
   * @param m_inst :: Shared pointer to the instrument for this peak detection
   * @param QLabFrame :: Q of the center of the peak, in reciprocal space
   * @param detectorDistance :: distance between the sample and the detector.
   *        Used to give a valid TOF. Default 1.0 meters.
   */
  Peak::Peak(Mantid::Geometry::IInstrument_const_sptr m_inst, Mantid::Kernel::V3D QLabFrame, double detectorDistance)
  : m_H(0), m_K(0), m_L(0),
    m_Intensity(0), m_SigmaIntensity(0), m_BinCount(0),
    m_GoniometerMatrix(3,3,true),
    m_InverseGoniometerMatrix(3,3,true),
    m_RunNumber(0)
  {
    this->setInstrument(m_inst);
    this->setQLabFrame(QLabFrame, detectorDistance);
  }


  //----------------------------------------------------------------------------------------------
  /** Constructor that uses the Q position of the peak (in the sample frame)
   * and a goniometer rotation matrix.
   * No detector ID is set.
   *
   * @param m_inst :: Shared pointer to the instrument for this peak detection
   * @param QSampleFrame :: Q of the center of the peak, in reciprocal space, in the sample frame (goniometer rotation accounted for).
   * @param goniometer :: a 3x3 rotation matrix
   * @param detectorDistance :: distance between the sample and the detector.
   *        Used to give a valid TOF. Default 1.0 meters.
   */
  Peak::Peak(Mantid::Geometry::IInstrument_const_sptr m_inst, Mantid::Kernel::V3D QSampleFrame,
      Mantid::Kernel::Matrix<double> goniometer, double detectorDistance)
  : m_H(0), m_K(0), m_L(0),
    m_Intensity(0), m_SigmaIntensity(0), m_BinCount(0),
    m_GoniometerMatrix(goniometer),
    m_InverseGoniometerMatrix(goniometer),
    m_RunNumber(0)
  {
    if(fabs(m_InverseGoniometerMatrix.Invert())<1e-8) throw std::invalid_argument("Peak::ctor(): Goniometer matrix must non-singular.");
    this->setInstrument(m_inst);
    this->setQSampleFrame(QSampleFrame, detectorDistance);
  }



  //----------------------------------------------------------------------------------------------
  /** Constructor
   *
   * @param m_inst :: Shared pointer to the instrument for this peak detection
   * @param m_DetectorID :: ID to the detector of the center of the peak
   * @param m_Wavelength :: incident neutron wavelength, in Angstroms
   * @return
   */
  Peak::Peak(Mantid::Geometry::IInstrument_const_sptr m_inst, int m_DetectorID, double m_Wavelength)
  : m_H(0), m_K(0), m_L(0),
    m_Intensity(0), m_SigmaIntensity(0), m_BinCount(0),
    m_GoniometerMatrix(3,3,true),
    m_InverseGoniometerMatrix(3,3,true),
    m_RunNumber(0)
  {
    this->setInstrument(m_inst);
    this->setDetectorID(m_DetectorID);
    this->setWavelength(m_Wavelength);
  }


  //----------------------------------------------------------------------------------------------
  /** Constructor
   *
   * @param m_inst :: Shared pointer to the instrument for this peak detection
   * @param m_DetectorID :: ID to the detector of the center of the peak
   * @param m_Wavelength :: incident neutron wavelength, in Angstroms
   * @param HKL :: vector with H,K,L position of the peak
   * @return
   */
  Peak::Peak(Mantid::Geometry::IInstrument_const_sptr m_inst, int m_DetectorID, double m_Wavelength, Mantid::Kernel::V3D HKL)
  : m_H(HKL[0]), m_K(HKL[1]), m_L(HKL[2]),
    m_Intensity(0), m_SigmaIntensity(0), m_BinCount(0),
    m_GoniometerMatrix(3,3,true),
    m_InverseGoniometerMatrix(3,3,true),
    m_RunNumber(0)
  {
    this->setInstrument(m_inst);
    this->setDetectorID(m_DetectorID);
    this->setWavelength(m_Wavelength);
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor
   *
   * @param m_inst :: Shared pointer to the instrument for this peak detection
   * @param m_DetectorID :: ID to the detector of the center of the peak
   * @param m_Wavelength :: incident neutron wavelength, in Angstroms
   * @param HKL :: vector with H,K,L position of the peak
   * @param goniometer :: a 3x3 rotation matrix
   * @return
   */
  Peak::Peak(Mantid::Geometry::IInstrument_const_sptr m_inst, int m_DetectorID, double m_Wavelength, Mantid::Kernel::V3D HKL, Mantid::Kernel::Matrix<double> goniometer) :
    m_H(HKL[0]), m_K(HKL[1]), m_L(HKL[2]),
    m_Intensity(0), m_SigmaIntensity(0), m_BinCount(0),
    m_GoniometerMatrix(goniometer),
    m_InverseGoniometerMatrix(goniometer),
    m_RunNumber(0)
  {
    if(fabs(m_InverseGoniometerMatrix.Invert())<1e-8) throw std::invalid_argument("Peak::ctor(): Goniometer matrix must non-singular.");
    this->setInstrument(m_inst);
    this->setDetectorID(m_DetectorID);
    this->setWavelength(m_Wavelength);
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor making a Peak from IPeak interface
   *
   * @param ipeak :: const reference to an IPeak object
   * @return
   */
  Peak::Peak(const Peak & peak):
    m_inst(peak.m_inst),
    m_det(peak.m_det),
    m_BankName(peak.m_BankName),
    m_DetectorID(peak.m_DetectorID),
    m_H(peak.m_H),
    m_K(peak.m_K),
    m_L(peak.m_L),
    m_Intensity(peak.m_Intensity),
    m_SigmaIntensity(peak.m_SigmaIntensity),
    m_BinCount(peak.m_BinCount),
    m_InitialEnergy(peak.m_InitialEnergy),
    m_FinalEnergy(peak.m_FinalEnergy),
    m_GoniometerMatrix(peak.m_GoniometerMatrix),
    m_InverseGoniometerMatrix(peak.m_InverseGoniometerMatrix),
    m_RunNumber(peak.m_RunNumber),
    m_Row(peak.m_Row),
    m_Col(peak.m_Col),
    sourcePos(peak.sourcePos),
    samplePos(peak.samplePos),
    detPos(peak.detPos)
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor making a Peak from IPeak interface
   *
   * @param ipeak :: const reference to an IPeak object
   * @return
   */
  Peak::Peak(const API::IPeak & ipeak):
  m_H(ipeak.getH()),
  m_K(ipeak.getK()),
  m_L(ipeak.getL()),
  m_Intensity(ipeak.getIntensity()),
  m_SigmaIntensity(ipeak.getSigmaIntensity()),
  m_BinCount(ipeak.getBinCount()),
  m_InitialEnergy(ipeak.getInitialEnergy()),
  m_FinalEnergy(ipeak.getFinalEnergy()),
  m_GoniometerMatrix(ipeak.getGoniometerMatrix()),
  m_InverseGoniometerMatrix(ipeak.getGoniometerMatrix()),
  m_RunNumber(ipeak.getRunNumber())
  {
    if(fabs(m_InverseGoniometerMatrix.Invert())<1e-8) throw std::invalid_argument("Peak::ctor(): Goniometer matrix must non-singular.");
    setInstrument(ipeak.getInstrument());
    detid_t id = ipeak.getDetectorID();
    if (id >= 0)
    {
      setDetectorID(id);
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  Peak::~Peak()
  {
  }


  //----------------------------------------------------------------------------------------------
  /** Set the incident wavelength of the neutron. Calculates the energy from this.
   * Assumes elastic scattering.
   *
   * @param wavelength :: wavelength in Angstroms.
   */
  void Peak::setWavelength(double wavelength)
  {
    // Velocity of the neutron (non-relativistic)
    double velocity = PhysicalConstants::h / (wavelength * 1e-10 * PhysicalConstants::NeutronMass );
    // Energy in J of the neutron
    double energy = PhysicalConstants::NeutronMass * velocity * velocity / 2.0;
    // Convert to meV
    m_InitialEnergy = energy / PhysicalConstants::meV;
    m_FinalEnergy = m_InitialEnergy;
  }


  //----------------------------------------------------------------------------------------------
  /** Set the instrument (and save the source/sample pos).
   * Call setDetectorID AFTER this call.
   *
   * @param inst :: Instrument sptr to use
   */
  void Peak::setInstrument(Mantid::Geometry::IInstrument_const_sptr inst)
  {
    m_inst = inst;
    if (!inst) throw std::runtime_error("Peak::setInstrument(): No instrument is set!");

    // Cache some positions
    const Geometry::IObjComponent_sptr sourceObj = m_inst->getSource();
    if (sourceObj == NULL)
      throw Exception::InstrumentDefinitionError("Peak::setInstrument(): Failed to get source component from instrument");
    const Geometry::IObjComponent_sptr sampleObj = m_inst->getSample();
    if (sampleObj == NULL)
      throw Exception::InstrumentDefinitionError("Peak::setInstrument(): Failed to get sample component from instrument");

    sourcePos = sourceObj->getPos();
    samplePos = sampleObj->getPos();
  }

  //----------------------------------------------------------------------------------------------
  /** Set the detector ID and look up and cache values related to it.
   *
   * @param m_DetectorID :: ID of detector at the center of the peak
   */
  void Peak::setDetectorID(int m_DetectorID)
  {
    this->m_DetectorID = m_DetectorID;
    if (!m_inst) throw std::runtime_error("Peak::setInstrument(): No instrument is set!");
    this->m_det = m_inst->getDetector(this->m_DetectorID);
    if (!m_det) throw std::runtime_error("Peak::setInstrument(): No detector was found!");

    detPos = m_det->getPos();

    // We now look for the row/column. -1 if not found.
    m_Row = -1;
    m_Col = -1;

    // Go up 2 parents to find the rectangular detector
    IComponent_const_sptr parent = m_det->getParent();
    if (!parent) return;
    m_BankName = parent->getName(); // Use the parent by default
    parent = parent->getParent();
    if (!parent) return;
    RectangularDetector_const_sptr retDet = boost::dynamic_pointer_cast<const RectangularDetector>(parent);
    if (!retDet) return;
    m_BankName = retDet->getName(); // Use the grand-parent for rectangular detectors

    std::pair<int,int> xy = retDet->getXYForDetectorID(m_DetectorID);
    m_Row = xy.second;
    m_Col = xy.first;
  }


  // -------------------------------------------------------------------------------------
  /** Calculate the neutron wavelength (in angstroms) at the peak
   * (Note for inelastic scattering - it is the wavelength corresponding to the final energy)*/
  double Peak::getWavelength() const
  {
    // Energy in J of the neutron
    double energy =  PhysicalConstants::meV * m_FinalEnergy;
    // v = sqrt(2.0 * E / m)
    double velocity = sqrt(2.0*energy/PhysicalConstants::NeutronMass);
    // wavelength = h / mv
    double wavelength = PhysicalConstants::h / (PhysicalConstants::NeutronMass * velocity);
    // Return it in angstroms
    return wavelength * 1e10;
  }

  // -------------------------------------------------------------------------------------
  /** Calculate the time of flight (in microseconds) of the neutrons for this peak,
   * using the geometry of the detector  */
  double Peak::getTOF() const
  {
    // First, get the neutron traveling distances
    double L1 = this->getL1();
    double L2 = this->getL2();
    // Energy in J of the neutron
    double Ei =  PhysicalConstants::meV * m_InitialEnergy;
    double Ef =  PhysicalConstants::meV * m_FinalEnergy;
    // v = sqrt(2 * E / m)
    double vi = sqrt(2.0*Ei/PhysicalConstants::NeutronMass);
    double vf = sqrt(2.0*Ef/PhysicalConstants::NeutronMass);
    // Time of flight in seconds = distance / speed
    double tof = L1/vi+L2/vf;
    // Return in microsecond units
    return tof * 1e6;
  }

  // -------------------------------------------------------------------------------------
  /** Calculate the d-spacing of the peak, in 1/Angstroms  */
  double Peak::getDSpacing() const
  {
    // The detector is at 2 theta scattering angle
    V3D beamDir = samplePos - sourcePos;
    V3D detDir = detPos - samplePos;

    double two_theta = detDir.angle(beamDir);
    
    // In general case (2*pi/d)^2=k_i^2+k_f^2-2*k_i*k_f*cos(two_theta)
    // E_i,f=k_i,f^2*hbar^2/(2 m)
    return 1e10*PhysicalConstants::h/sqrt(2.0*PhysicalConstants::NeutronMass*PhysicalConstants::meV)
        / sqrt(m_InitialEnergy+m_FinalEnergy-2.0*sqrt(m_InitialEnergy*m_FinalEnergy)*cos(two_theta));
  }

  //----------------------------------------------------------------------------------------------
  /** Return the Q change (of the lattice, k_i - k_f) for this peak.
   * The Q is in the Lab frame: the goniometer rotation was NOT taken out.
   *
   * Note: There is no 2*pi factor used, so |Q| = 1/wavelength.
   * */
  Mantid::Kernel::V3D Peak::getQLabFrame() const
  {
    // Normalized beam direction
    V3D beamDir = samplePos - sourcePos;
    beamDir /= beamDir.norm();
    // Normalized detector direction
    V3D detDir = (detPos - samplePos);
    detDir /= detDir.norm();

    // Energy in J of the neutron
    double ei =  PhysicalConstants::meV * m_InitialEnergy;
    // v = sqrt(2.0 * E / m)
    double vi = sqrt(2.0*ei/PhysicalConstants::NeutronMass);
    // wavelength = h / mv
    double wi = PhysicalConstants::h / (PhysicalConstants::NeutronMass * vi);
    // in angstroms
    wi*= 1e10;
    //wavevector=1/wavelength
    double wvi=1.0/wi;
    // Now calculate the wavevector of the scattered neutron
    double wvf = 1.0 / this->getWavelength();
    // And Q in the lab frame 
    return beamDir*wvi-detDir*wvf;
  }

  //----------------------------------------------------------------------------------------------
  /** Return the Q change (of the lattice, k_i - k_f) for this peak.
   * The Q is in the Sample frame: the goniometer rotation WAS taken out. */
  Mantid::Kernel::V3D Peak::getQSampleFrame() const
  {
    V3D Qlab = this->getQLabFrame();
    // Multiply by the inverse of the goniometer matrix to get the sample frame
    V3D Qsample = m_InverseGoniometerMatrix * Qlab;
    return Qsample;
  }

  //----------------------------------------------------------------------------------------------
  /** Set the peak using the peak's position in reciprocal space, in the sample frame.
   * The GoniometerMatrix will be used to find the Q in the lab frame, so it should
   * be set beforehand.
   *
   * @param QSampleFrame :: Q of the center of the peak, in reciprocal space
   *        This is in inelastic convention: momentum transfer of the LATTICE!
   *        Also, q does NOT have a 2pi factor = it is equal to 1/wavelength.
   * @param detectorDistance :: distance between the sample and the detector.
   *        Used to give a valid TOF. Default 1.0 meters.
   */
  void Peak::setQSampleFrame(Mantid::Kernel::V3D QSampleFrame, double detectorDistance)
  {
    V3D Qlab = m_GoniometerMatrix * QSampleFrame;
    this->setQLabFrame(Qlab, detectorDistance);
  }


  //----------------------------------------------------------------------------------------------
  /** Set the peak using the peak's position in reciprocal space, in the lab frame.
   * The detector position will be determined.
   * DetectorID, row and column will be set to -1 since they are not (necessarily) found.
   *
   * @param QLabFrame :: Q of the center of the peak, in reciprocal space.
   *        This is in inelastic convention: momentum transfer of the LATTICE!
   *        Also, q does NOT have a 2pi factor = it is equal to 1/wavelength (in Angstroms).
   * @param detectorDistance :: distance between the sample and the detector.
   *        Used to give a valid TOF. Default 1.0 meters.
   */
  void Peak::setQLabFrame(Mantid::Kernel::V3D QLabFrame, double detectorDistance)
  {
    // Clear out the detector = we can't know them
    m_DetectorID = -1;
    m_det = IDetector_sptr();
    m_Row = -1;
    m_Col = -1;
    m_BankName = "None";

    // The q-vector direction of the peak is = goniometer * ub * hkl_vector
    V3D q = QLabFrame;

    /* The incident neutron wavevector is in the +Z direction, ki = 1/wl (in z direction).
     * In the inelastic convention, q = ki - kf.
     * The final neutron wavector kf = -qx in x; -qy in y; and (-qz+1/wl) in z.
     * AND: norm(kf) = norm(ki) = 1.0/wavelength
     * THEREFORE: 1/wl = norm(q)^2 / (2*qz)
     */
    double norm_q = q.norm();

    if (norm_q == 0.0)
      throw std::invalid_argument("Peak::setQLabFrame(): Q cannot be 0,0,0.");
    if (q.Z() == 0.0)
      throw std::invalid_argument("Peak::setQLabFrame(): Q cannot be 0 in the Z (beam) direction.");

    double one_over_wl = (norm_q*norm_q) / (2.0 * q.Z());
    double wl = 1.0/one_over_wl;
    if (wl < 0.0)
    {
      std::ostringstream mess;
      mess << "Peak::setQLabFrame(): Wavelength found was negative (" << wl << " Ang)! This Q is not physical.";
      throw std::invalid_argument(mess.str());
    }

    // This is the scattered direction, kf = (-qx, -qy, 1/wl-qz)
    V3D beam = q * -1.0;
    beam.setZ(one_over_wl - q.Z());
    beam.normalize();

    // Save the wavelength
    this->setWavelength(wl);

    // Use the given detector distance to find the detector position.
    detPos = samplePos + beam * detectorDistance;
  }


  /** After creating a peak using the Q in the lab frame,
   * the detPos is set to the direction of the detector (but the detector is unknown)
   *
   * Using the instrument set in the peak, perform ray tracing
   * to find the exact detector.
   *
   * @return true if the detector ID was found.
   */
  bool Peak::findDetector()
  {
    // Scattered beam direction
    V3D oldDetPos = detPos;
    V3D beam = detPos - samplePos;
    beam.normalize();

    // Create a ray tracer
    InstrumentRayTracer tracker( boost::const_pointer_cast<IInstrument>(m_inst) );
    tracker.traceFromSample(beam);
    IDetector_sptr det = tracker.getDetectorResult();
    if (det)
    {
      // Set the detector ID, the row, col, etc.
      this->setDetectorID(det->getID());
      // HOWEVER, we retain the old detector position because we assume that is a little more precise.
      detPos = samplePos + beam * detPos.norm();
      return true;
    }
    return false;
  }



  //----------------------------------------------------------------------------------------------
  /** Return a shared ptr to the detector at center of peak. */
  Mantid::Geometry::IDetector_const_sptr Peak::getDetector() const
  {    return m_det;  }

  /** Return a shared ptr to the instrument for this peak. */
  Mantid::Geometry::IInstrument_const_sptr Peak::getInstrument() const
  {    return m_inst;  }

  //----------------------------------------------------------------------------------------------
  /** Return the run number this peak was measured at. */
  int Peak::getRunNumber() const
  {    return m_RunNumber;  }

  /** Set the run number that measured this peak
   * @param m_RunNumber :: the run number   */
  void Peak::setRunNumber(int m_RunNumber)
  {    this->m_RunNumber = m_RunNumber;  }

  //----------------------------------------------------------------------------------------------
  /** Get the ID of the detector at the center of the peak  */
  int Peak::getDetectorID() const
  {    return m_DetectorID;  }

  //----------------------------------------------------------------------------------------------
  /** Get the final neutron energy */
  double Peak::getFinalEnergy() const
  {    return m_FinalEnergy;  }

  /** Get the initial (incident) neutron energy */
  double Peak::getInitialEnergy() const
  {    return m_InitialEnergy; }

  //----------------------------------------------------------------------------------------------
  /** Get the H index of the peak */
  double Peak::getH() const
  {    return m_H;  }

  /** Get the K index of the peak */
  double Peak::getK() const
  {    return m_K;  }

  /** Get the L index of the peak */
  double Peak::getL() const
  {    return m_L;  }

  /** Return the HKL vector */
  Mantid::Kernel::V3D Peak::getHKL()
  {
    return V3D(m_H, m_K, m_L);
  }

  //----------------------------------------------------------------------------------------------
  /** Set the H index of this peak
   * @param m_H :: index to set   */
  void Peak::setH(double m_H)
  {    this->m_H = m_H;  }

  /** Set the K index of this peak
   * @param m_K :: index to set   */
  void Peak::setK(double m_K)
  {    this->m_K = m_K;  }

  /** Set the L index of this peak
   * @param m_L :: index to set   */
  void Peak::setL(double m_L)
  {    this->m_L = m_L;  }

  /** Set all three H,K,L indices of the peak */
  void Peak::setHKL(double H, double K, double L)
  {
    m_H = H;
    m_K = K;
    m_L = L;
  }

  /** Set all HKL
   *
   * @param HKL :: vector with x,y,z -> h,k,l
   */
  void Peak::setHKL(Mantid::Kernel::V3D HKL)
  {
    m_H = HKL.X();
    m_K = HKL.Y();
    m_L = HKL.Z();
  }

  //----------------------------------------------------------------------------------------------
  /** Return the # of counts in the bin at its peak*/
  double Peak::getBinCount() const
  {    return m_BinCount;  }

  /** Return the integrated peak intensity */
  double Peak::getIntensity() const
  {    return m_Intensity;  }

  /** Return the error on the integrated peak intensity */
  double Peak::getSigmaIntensity() const
  {    return m_SigmaIntensity;  }

  /** Set the integrated peak intensity
   * @param m_Intensity :: intensity value   */
  void Peak::setIntensity(double m_Intensity)
  {    this->m_Intensity = m_Intensity;  }

  /** Set the # of counts in the bin at its peak
   * @param m_BinCount :: counts  */
  void Peak::setBinCount(double m_BinCount)
  {    this->m_BinCount = m_BinCount;  }

  /** Set the error on the integrated peak intensity
   * @param m_SigmaIntensity :: intensity error value   */
  void Peak::setSigmaIntensity(double m_SigmaIntensity)
  {    this->m_SigmaIntensity = m_SigmaIntensity;  }

  /** Set the final energy
   * @param m_FinalEnergy :: final energy in meV   */
  void Peak::setFinalEnergy(double m_FinalEnergy)
  {    this->m_FinalEnergy = m_FinalEnergy;  }

  /** Set the initial energy
   * @param m_InitialEnergy :: initial energy in meV   */
  void Peak::setInitialEnergy(double m_InitialEnergy)
  {    this->m_InitialEnergy = m_InitialEnergy;  }


  // -------------------------------------------------------------------------------------
  /** Get the goniometer rotation matrix at which this peak was measured. */
  Mantid::Kernel::Matrix<double> Peak::getGoniometerMatrix() const
  {
    return this->m_GoniometerMatrix;
  }

  /** Set the goniometer rotation matrix at which this peak was measured.
   * @param goniometerMatrix :: 3x3 matrix that represents the rotation matrix of the goniometer
   * @throw std::invalid_argument if matrix is not 3x3*/
  void Peak::setGoniometerMatrix(Mantid::Kernel::Matrix<double> goniometerMatrix)
  {
    if ((goniometerMatrix.numCols() != 3) || (goniometerMatrix.numRows() != 3))
      throw std::invalid_argument("Peak::setGoniometerMatrix(): Goniometer matrix must be 3x3.");
    this->m_GoniometerMatrix = goniometerMatrix;
    // Calc the inverse rotation matrix
    m_InverseGoniometerMatrix = m_GoniometerMatrix;
    if(fabs(m_InverseGoniometerMatrix.Invert())<1e-8) throw std::invalid_argument("Peak::setGoniometerMatrix(): Goniometer matrix must non-singular.");
  }


  // -------------------------------------------------------------------------------------
  /** Find the name of the bank that is the parent of the detector. This works
   * best for RectangularDetector instruments (goes up two levels)
   * @return name of the bank.
   */
  std::string Peak::getBankName() const
  {
    return m_BankName;
  }

  // -------------------------------------------------------------------------------------
  /** For RectangularDetectors only, returns the row (y) of the pixel of the detector.
  * Returns -1 if it could not find it. */
  int Peak::getRow() const
  {   return m_Row;  }

  // -------------------------------------------------------------------------------------
  /** For RectangularDetectors only, returns the column (x) of the pixel of the detector.
   * Returns -1 if it could not find it. */
  int Peak::getCol() const
  {   return m_Col;  }

  // -------------------------------------------------------------------------------------
  /** Return the detector position vector */
  Mantid::Kernel::V3D Peak::getDetPos() const
  {
    return detPos;
  }

  // -------------------------------------------------------------------------------------
  /** Return the L1 flight path length (source to sample), in meters. */
  double Peak::getL1() const
  {
    return (samplePos - sourcePos).norm();
  }

  // -------------------------------------------------------------------------------------
  /** Return the L2 flight path length (sample to detector), in meters. */
  double Peak::getL2() const
  {
    return (detPos - samplePos).norm();
  }

} // namespace Mantid
} // namespace DataObjects

