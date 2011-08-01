#ifndef BOXCONTROLLER_H_
#define BOXCONTROLLER_H_

#include "MantidAPI/DiskMRU.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidNexus/NeXusFile.hpp"
#include <boost/shared_ptr.hpp>
#include <vector>

namespace Mantid
{
namespace MDEvents
{



  /** This class is used by MDBox and MDGridBox in order to intelligently
   * determine optimal behavior. It informs:
   *  - When a MDBox needs to split into a MDGridBox.
   *  - How the splitting will occur.
   *  - When a MDGridBox should use Tasks to parallelize adding events
   *
   * @author Janik Zikovsky
   * @date Feb 21, 2011
   */
  class DLLExport BoxController
  {
  public:
    //-----------------------------------------------------------------------------------
    /** Constructor
     *
     * @param nd :: number of dimensions
     * @return BoxController instance
     */
    BoxController(size_t nd)
    :nd(nd), m_maxId(0), m_file(NULL)
    {
      // TODO: Smarter ways to determine all of these values
      m_maxDepth = 5;
      m_addingEvents_eventsPerTask = 1000;
      m_addingEvents_numTasksPerBlock = Kernel::ThreadPool::getNumPhysicalCores() * 5;
      m_splitInto.resize(this->nd, 1);
      resetNumBoxes();
    }

    /// Serialize
    std::string toXMLString() const;

    /// De-serializing XML
    void fromXMLString(const std::string & xml);

    /// Equality operator
    bool operator==(const BoxController & other) const;

    //-----------------------------------------------------------------------------------
    /** Get # of dimensions
     * @return # of dimensions
     */
    size_t getNDims() const
    {
      return nd;
    }

    //-----------------------------------------------------------------------------------
    /** @return the next available box Id.
     * Call when creating a MDBox to give it an ID. */
    size_t getNextId()
    {
      return m_maxId++;
    }

    //-----------------------------------------------------------------------------------
    /** @return the maximum (not-inclusive) ID number anywhere in the workspace. */
    size_t getMaxId() const
    {
      return m_maxId;
    }

    //-----------------------------------------------------------------------------------
    /** Set the new maximum ID number anywhere in the workspace.
     * Should only be called when loading a file. */
    void setMaxId(size_t newMaxId)
    {
      m_maxId = newMaxId;
    }

    //-----------------------------------------------------------------------------------
    /** @return the mutex for avoiding simultaneous assignments of box Ids. */
    inline Kernel::Mutex & getIdMutex()
    {
      return m_idMutex;
    }

    //-----------------------------------------------------------------------------------
    /** Return true if the MDBox should split, given :
     *
     * @param numPoints :: # of points in the box
     * @param depth :: recursion depth of the box
     * @return bool, true if it should split
     */
    bool willSplit(size_t numPoints, size_t depth) const
    {
      return (numPoints > m_SplitThreshold) && (depth < m_maxDepth);
    }

    //-----------------------------------------------------------------------------------
    /** Return the splitting threshold, in # of events */
    size_t getSplitThreshold() const
    {
      return m_SplitThreshold;
    }

    /** Set the splitting threshold
     * @param threshold :: # of points at which the MDBox splits
     */
    void setSplitThreshold(size_t threshold)
    {
      m_SplitThreshold = threshold;
    }

    //-----------------------------------------------------------------------------------
    /** Return into how many to split along a dimension
     *
     * @param dim :: index of the dimension to split
     * @return the dimension will be split into this many even boxes.
     */
    size_t getSplitInto(size_t dim) const
    {
//      if (dim >= nd)
//        throw std::invalid_argument("BoxController::setSplitInto() called with too high of a dimension index.");
      return m_splitInto[dim];
    }

    /// Return how many boxes (total) a MDGridBox will contain.
    size_t getNumSplit() const
    {
      return m_numSplit;
    }

    //-----------------------------------------------------------------------------------
    /** Set the way splitting will be done
     * @param num :: amount in which to split
     * */
    void setSplitInto(size_t num)
    {
      m_splitInto.clear();
      m_splitInto.resize(nd, num);
      calcNumSplit();
    }

    //-----------------------------------------------------------------------------------
    /** Set the way splitting will be done
     *
     * @param dim :: dimension to set
     * @param num :: amount in which to split
     */
    void setSplitInto(size_t dim, size_t num)
    {
      if (dim >= nd)
        throw std::invalid_argument("BoxController::setSplitInto() called with too high of a dimension index.");
      m_splitInto[dim] = num;
      calcNumSplit();
    }

    //-----------------------------------------------------------------------------------
    /** When adding events, how many events per task should be done?
     *
     * @param m_addingEvents_eventsPerTask :: events per task
     */
    void setAddingEvents_eventsPerTask(size_t m_addingEvents_eventsPerTask)
    {
      this->m_addingEvents_eventsPerTask = m_addingEvents_eventsPerTask;
    }
    /// @return When adding events, how many events per task should be done?
    size_t getAddingEvents_eventsPerTask() const
    {
      return m_addingEvents_eventsPerTask;
    }

    /** When adding events, how many events tasks per block should be done?
     *
     * @param m_addingEvents_numTasksPerBlock :: tasks/block
     */
    void setAddingEvents_numTasksPerBlock(size_t m_addingEvents_numTasksPerBlock)
    {
      this->m_addingEvents_numTasksPerBlock = m_addingEvents_numTasksPerBlock;
    }

    /// @return When adding events, how many tasks per block should be done?
    size_t getAddingEvents_numTasksPerBlock() const
    {
      return m_addingEvents_numTasksPerBlock;
    }

    //-----------------------------------------------------------------------------------
    /** Get parameters for adding events to a MDGridBox, trying to optimize parallel CPU use.
     *
     * @param[out] eventsPerTask :: the number of events that should be added by a single task object.
     *    This should be large enough to avoid overhead without being
     *    too large, making event lists too long before splitting
     * @param[out] numTasksPerBlock :: the number of tasks (of size eventsPerTask) to be allocated
     *    before the grid boxes should be re-split. Having enough parallel tasks will
     *    help the CPU be used fully.
     */
    void getAddingEventsParameters(size_t & eventsPerTask, size_t & numTasksPerBlock) const
    {
      // TODO: Smarter values here depending on nd, etc.
      eventsPerTask = m_addingEvents_eventsPerTask;
      numTasksPerBlock = m_addingEvents_numTasksPerBlock;
    }


    //-----------------------------------------------------------------------------------
    /** @return the max recursion depth allowed for grid box splitting. */
    size_t getMaxDepth() const
    {
      return m_maxDepth;
    }

    /** Sets the max recursion depth allowed for grid box splitting.
     * NOTE! This resets numMDBoxes stats!
     * @param value :: the max depth for splitting
     *  */
    void setMaxDepth(size_t value)
    {
      m_maxDepth = value;
      resetNumBoxes();
    }

    //-----------------------------------------------------------------------------------
    /** Determine when would be a good time to split MDBoxes into MDGridBoxes.
     * This is to be called while adding events. Splitting boxes too frequently
     * would be a slow-down, but keeping the boxes split at an earlier stage
     * should help scalability for later adding, so there is a balance to get.
     *
     * @param eventsAdded :: How many events were added since the last split?
     * @param numMDBoxes :: How many un-split MDBoxes are there (total) in the workspace
     * @return true if the boxes should get split.
     */
    bool shouldSplitBoxes(size_t eventsAdded, size_t numMDBoxes) const
    {
      // Avoid divide by zero
      if(numMDBoxes == 0)
        return false;

      // Return true if the average # of events per box is big enough to split.
      return ((eventsAdded / numMDBoxes) > m_SplitThreshold);
    }


    //-----------------------------------------------------------------------------------
    /** Call to track the number of MDBoxes are contained in the MDEventWorkspace
     * This should be called when a MDBox gets split into a MDGridBox.
     * The number of MDBoxes at [depth] is reduced by one
     * The number of MDBoxes at [depth+1] is increased by however many the splitting gives.
     * Also tracks the number of MDGridBoxes.
     *
     * @param depth :: the depth of the MDBox that is being split into MDGrid boxes.
     */
    void trackNumBoxes(size_t depth)
    {
      m_mutexNumMDBoxes.lock();
      if (m_numMDBoxes[depth] > 0)
      {
        m_numMDBoxes[depth]--;
      }
      m_numMDGridBoxes[depth]++;
      m_numMDBoxes[depth + 1] += m_numSplit;
      m_mutexNumMDBoxes.unlock();
    }

    /** Return the vector giving the number of MD Boxes as a function of depth */
    const std::vector<size_t> & getNumMDBoxes() const
    {
      return m_numMDBoxes;
    }

    /** Return the vector giving the MAXIMUM number of MD Boxes as a function of depth */
    const std::vector<double> & getMaxNumMDBoxes() const
    {
      return m_maxNumMDBoxes;
    }

    /** Return the total number of MD Boxes, irrespective of depth */
    size_t getTotalNumMDBoxes() const
    {
      size_t total = 0;
      for (size_t depth=0; depth<m_numMDBoxes.size(); depth++)
      {
        total += m_numMDBoxes[depth];
      }
      return total;
    }

    /** Return the total number of MDGridBox'es, irrespective of depth */
    size_t getTotalNumMDGridBoxes() const
    {
      size_t total = 0;
      for (size_t depth=0; depth<m_numMDGridBoxes.size(); depth++)
      {
        total += m_numMDGridBoxes[depth];
      }
      return total;
    }

    /** Return the average recursion depth of gridding.
     * */
    double getAverageDepth() const
    {
      double total = 0;
      double maxNumberOfFinestBoxes = m_maxNumMDBoxes[m_maxNumMDBoxes.size()-1];
      for (size_t depth=0; depth<m_numMDBoxes.size(); depth++)
      {
        // Add up the number of MDBoxes at that depth, weighed by their volume in units of the volume of the finest possible box.
        // I.e. a box at level 1 is 100 x bigger than a box at level 2, so it counts 100x more.
        total += double(depth * m_numMDBoxes[depth]) * (maxNumberOfFinestBoxes / m_maxNumMDBoxes[depth]) ;
      }
      return total / maxNumberOfFinestBoxes;
    }

    /** Reset the number of boxes tracked in m_numMDBoxes */
    void resetNumBoxes()
    {
      m_mutexNumMDBoxes.lock();
      m_numMDBoxes.clear();
      m_numMDBoxes.resize(m_maxDepth + 1, 0); // Reset to 0
      m_numMDGridBoxes.resize(m_maxDepth + 1, 0); // Reset to 0
      m_numMDBoxes[0] = 1; // Start at 1 at depth 0.
      resetMaxNumBoxes(); // Also the maximums
      m_mutexNumMDBoxes.unlock();
    }


    //-----------------------------------------------------------------------------------
    /** @return the open NeXus file handle */
    ::NeXus::File * getFile() const
    { return m_file; }

    /** Sets the open Nexus file to use with file-based back-end
     * @param file :: file handle */
    void setFile(::NeXus::File * file)
    { m_file = file; }

    //-----------------------------------------------------------------------------------
    /** Return the disk MRU for disk caching */
//    const Mantid::API::DiskMRU & getDiskMRU() const
//    { return m_diskMRU; }
//
//    Mantid::API::DiskMRU & getDiskMRU()
//    { return m_diskMRU; }


    //-----------------------------------------------------------------------------------
    /** Set the memory-caching parameters for a file-backed
     * MDEventWorkspace.
     *
     * @param bytesInMem :: number of bytes to keep in memory.
     * @param bytesPerEvent :: sizeof(MDEvent) that is in the workspace
     *
     */
    void setCacheParameters(size_t bytesInMem, size_t bytesPerEvent)
    {
      if (bytesPerEvent == 0)
        throw std::invalid_argument("Size of an event cannot be == 0.");
      // Save the values
      m_bytesPerEvent = bytesPerEvent;
      m_numEventsInMem = bytesInMem / m_bytesPerEvent;
    }

//
//    /** Does the cache require that this box be released? */
//    bool releaseBox(size_t boxId, size_t numEvents)
//    {
//    }


    //-----------------------------------------------------------------------------------
  private:
    /// When you split a MDBox, it becomes this many sub-boxes
    void calcNumSplit()
    {
      m_numSplit = 1;
      for(size_t d = 0;d < nd;d++){
        m_numSplit *= m_splitInto[d];
      }
      /// And this changes the max # of boxes too
      resetMaxNumBoxes();
    }

    /// Calculate the vector of the max # of MDBoxes per level.
    void resetMaxNumBoxes()
    {
      // Now calculate the max # of boxes
      m_maxNumMDBoxes.resize(m_maxDepth + 1, 0); // Reset to 0
      m_maxNumMDBoxes[0] = 1;
      for (size_t d=1; d<m_maxNumMDBoxes.size(); d++)
        m_maxNumMDBoxes[d] = m_maxNumMDBoxes[d-1] * double(m_numSplit);
    }


    /// Number of dimensions
    size_t nd;

    /** The maximum ID number of any boxes in the workspace (not inclusive,
     * i.e. maxId = 100 means there the highest ID number is 99.  */
    size_t m_maxId;

    /// Splitting threshold
    size_t m_SplitThreshold;

    /** Maximum splitting depth: don't go further than this many levels of recursion.
     * This avoids infinite recursion and should be set to a value that gives a smallest
     * box size that is a little smaller than the finest desired binning upon viewing.
     */
    size_t m_maxDepth;

    /// Splitting # for all dimensions
    std::vector<size_t> m_splitInto;

    /// When you split a MDBox, it becomes this many sub-boxes
    size_t m_numSplit;

    /// For adding events tasksm_file
    size_t m_addingEvents_eventsPerTask;

    /// For adding events tasks
    size_t m_addingEvents_numTasksPerBlock;

    /// For tracking how many MDBoxes (not MDGridBoxes) are at each recursion level
    std::vector<size_t> m_numMDBoxes;

    /// For tracking how many MDGridBoxes (not MDBoxes) are at each recursion level
    std::vector<size_t> m_numMDGridBoxes;

    /// Mutex for changing the number of MD Boxes.
    Mantid::Kernel::Mutex m_mutexNumMDBoxes;

    /// This is the maximum number of MD boxes there could be at each recursion level (e.g. (splitInto ^ ndims) ^ depth )
    std::vector<double> m_maxNumMDBoxes;

    /// Mutex for getting IDs
    Mantid::Kernel::Mutex m_idMutex;

    /// Filename of the file backend
    std::string m_filename;

    /// Open file handle to the file back-end
    ::NeXus::File * m_file;

//    /// Instance of the disk-caching MRU list.
//    mutable Mantid::API::DiskMRU m_diskMRU;

  public:
    /// Mutex for locking access to the file, for file-back-end MDBoxes.
    Mantid::Kernel::Mutex fileMutex;

  private:
    /// Number of events to keep in memory, when using a file-based back-end.
    size_t m_numEventsInMem;

    /// Number of bytes in a single MDEvent<> of the workspace.
    size_t m_bytesPerEvent;

    /// Minimum number of events in a MDBox to bother to cache to disk.
    size_t m_minEventsToCache;

  };

  /// Shared ptr to BoxController
  typedef boost::shared_ptr<BoxController> BoxController_sptr;

}//namespace MDEvents

}//namespace Mantid



#endif /* SPLITCONTROLLER_H_ */
