#ifndef MANTID_TYPES_TOFEVENT_H
#define MANTID_TYPES_TOFEVENT_H

#include "MantidTypes/Core/DateAndTime.h"
#include <MantidTypes/DllConfig.h>

namespace Mantid {
namespace DataObjects {
class EventList;
class WeightedEvent;
class WeightedEventNoTime;
} // namespace DataObjects
namespace DataHandling {
class LoadEventNexus;
}
namespace Types {
namespace Event {
//==========================================================================================
/** Info about a single neutron detection event:
*
*  - the time of flight of the neutron (can be converted to other units)
*  - the absolute time of the pulse at which it was produced
*/
#pragma pack(push, 4) // Ensure the structure is no larger than it needs to
class MANTID_TYPES_DLL TofEvent {

  /// EventList has the right to mess with TofEvent.
  friend class DataObjects::EventList;
  friend class DataObjects::WeightedEvent;
  friend class DataObjects::WeightedEventNoTime;
  friend class DataHandling::LoadEventNexus; // Needed while the ISIS hack of
                                             // spreading events out in a bin
                                             // remains

protected:
  /** The 'x value' of the event. This will be in a unit available from the
   * UnitFactory.
   *  Initially (prior to any unit conversion on the holding workspace), this
   * will have
   *  the unit of time-of-flight in microseconds.
   */
  double m_tof;

  /**
   * The absolute time of the start of the pulse that generated this event.
   * This is saved as the number of ticks (1 nanosecond if boost is compiled
   * for nanoseconds) since a specified epoch: we use the GPS epoch of Jan 1,
   *1990.
   *
   * 64 bits gives 1 ns resolution up to +- 292 years around 1990. Should be
   *enough.
   */
  Core::DateAndTime m_pulsetime;

public:
  /// Constructor, specifying only the time of flight in microseconds
  TofEvent(double tof);

  /// Constructor, specifying the time of flight in microseconds and the frame
  /// id
  TofEvent(double tof, const Core::DateAndTime pulsetime);

  /// Empty constructor
  TofEvent();

  bool operator==(const TofEvent &rhs) const;
  bool operator<(const TofEvent &rhs) const;
  bool operator<(const double rhs_tof) const;
  bool operator>(const TofEvent &rhs) const;
  bool equals(const TofEvent &rhs, const double tolTof,
              const int64_t tolPulse) const;

  double operator()() const;
  double tof() const;
  Mantid::Types::Core::DateAndTime pulseTime() const;
  double weight() const;
  double error() const;
  double errorSquared() const;

  /// Output a string representation of the event to a stream
  friend std::ostream &operator<<(std::ostream &os, const TofEvent &event);
};
#pragma pack(pop)

//==========================================================================================
// TofEvent inlined member function definitions
//==========================================================================================

/** () operator: return the tof (X value) of the event.
*  This is useful for std operations like comparisons and std::lower_bound
*  @return :: double, the tof (X value) of the event.
*/
inline double TofEvent::operator()() const { return m_tof; }

/** @return The 'x value'. Despite the name, this can be in any unit in the
* UnitFactory.
*  If it is time-of-flight, it will be in microseconds.
*/
inline double TofEvent::tof() const { return m_tof; }

/// Return the pulse time
inline Core::DateAndTime TofEvent::pulseTime() const { return m_pulsetime; }

/// Return the weight of the event - exactly 1.0 always
inline double TofEvent::weight() const { return 1.0; }

/// Return the error of the event - exactly 1.0 always
inline double TofEvent::error() const { return 1.0; }

/// Return the errorSquared of the event - exactly 1.0 always
inline double TofEvent::errorSquared() const { return 1.0; }
} // namespace Event
} // namespace Types
} // namespace Mantid
#endif // MANTID_TYPES_TOFEVENT_H