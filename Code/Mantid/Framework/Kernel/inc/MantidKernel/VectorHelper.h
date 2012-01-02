#ifndef MANTID_KERNEL_VECTORHELPER_H_
#define MANTID_KERNEL_VECTORHELPER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/cow_ptr.h"
#include <vector>
#include <functional>
#include <cmath>
#include <stdexcept>

namespace Mantid
{
namespace Kernel
{
/*
    A collection of functions for use with vectors

    @author Laurent C Chapon, Rutherford Appleton Laboratory
    @date 16/12/2008

    Copyright &copy; 2007-2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
namespace VectorHelper
{
  int MANTID_KERNEL_DLL createAxisFromRebinParams(const std::vector<double>& params, std::vector<double>& xnew);

  void MANTID_KERNEL_DLL rebin(const std::vector<double>& xold, const std::vector<double>& yold, const std::vector<double>& eold,
        const std::vector<double>& xnew, std::vector<double>& ynew, std::vector<double>& enew, 
        bool distribution, bool addition = false);

  // New method to rebin Histogram data, should be faster than previous one
  void MANTID_KERNEL_DLL rebinHistogram(const std::vector<double>& xold, const std::vector<double>& yold, const std::vector<double>& eold,
                                const std::vector<double>& xnew, std::vector<double>& ynew, std::vector<double>& enew,bool addition);

  /// Convert an array of bin boundaries to bin centre values.
  void MANTID_KERNEL_DLL convertToBinCentre(const std::vector<double> & bin_edges, std::vector<double> & bin_centres);
  bool MANTID_KERNEL_DLL isConstantValue(const MantidVec &arra);

  template <typename NumT>
  MANTID_KERNEL_DLL std::vector<NumT> splitStringIntoVector(std::string listString);

  MANTID_KERNEL_DLL int getBinIndex(std::vector<double>& bins, const double X );
  // Linearly interpolate between a set of Y values. Assumes the values are set for the calculated nodes
  MANTID_KERNEL_DLL void linearlyInterpolateY(const std::vector<double> & x, std::vector<double> & y, const double stepSize);


  //-------------------------------------------------------------------------------------
  /** Return the length of the vector (in the physical sense),
   * the sqrt of the sum of the squares of the components
   *
   * @param x :: input vector, x should be float or double.
   * @return length of the vector
   */
  template<typename T>
  T lengthVector(const std::vector<T> & x)
  {
    T total = 0;
    for (size_t i=0; i<x.size(); i++)
      total += x[i] * x[i];
    // Length is sqrt
    total = sqrt(total);
    return total;
  }

  //-------------------------------------------------------------------------------------
  /** Normalize a vector of any size to unity, using the sum of the squares of the components
   *
   * @param x :: input vector, x should be float or double. Length 1+
   * @return the vector, normalized to 1.
   */
  template<typename T>
  std::vector<T> normalizeVector(const std::vector<T> & x)
  {
    // Ignore 0-sized vectors
    if (x.size() == 0) return x;
    std::vector<T> out(x.size(), 0);
    // Length is sqrt
    T length = lengthVector(x);
    for (size_t i=0; i<x.size(); i++)
      out[i] = x[i] / length;
    return out;
  }

  //-------------------------------------------------------------------------------------
  /** Generic method to convert an iterator to an array of type T.
   * Data type is converted at the same type.
   * @param begin :: iterator at the beginning of the data
   * @param end :: iterator at the end of the data
   * @param dims_array :: array to hold size of dimensions
   * @return :: a pointer to an array of type T.
   */
  template< typename T, typename _ForwardIterator >
  T * iteratorToArray(_ForwardIterator begin, _ForwardIterator end, int * dims_array)
  {
    // Create the output array
    int num = static_cast<int>(std::distance(begin, end));
    dims_array[0] = num;
    T * out = new T[num];
    std::copy(begin, end, out);
    return out;
  }

  /** Generic method to convert an iterator to an array of type T.
   * Data type is converted at the same type.
   * @param begin :: iterator at the beginning of the data
   * @param end :: iterator at the end of the data
   * @param dims_array :: array to hold size of dimensions
   * @return :: a pointer to an array of type T.
   */
  template< typename T, typename _ForwardIterator >
  T * iteratorToArray(_ForwardIterator begin, _ForwardIterator end, size_t * dims_array)
  {
    // Create the output array
    size_t num = std::distance(begin, end);
    dims_array[0] = num;
    T * out = new T[num];
    std::copy(begin, end, out);
    return out;
  }

  //! Functor used for computing the sum of the square values of a vector, using the accumulate algorithm
  template <class T> struct SumGaussError: public std::binary_function<T,T,T>
  {
    SumGaussError(){}
    /// Sums the arguments in quadrature
    inline T operator()(const T& l, const T& r) const
    {
      return sqrt(l*l+r*r);
    }
  };

  /// Functor to deal with the increase in the error when adding (or substracting) a number of counts. More generally add errors in quadrature using the square of one of the errors (variance = error^2)
  template <class T> struct AddVariance: public std::binary_function<T,T,T>
  {
    AddVariance(){}
    /// adds the square of the left-hand argument to the right hand argument and takes the square root
    T operator()(const T& r, const T& x) const
    {
      return sqrt(r*r+x);
    }
  };

  /// Functor to accumulate a sum of squares
  template <class T> struct SumSquares: public std::binary_function<T,T,T>
  {
    SumSquares(){}
    /// Adds the square of the right-hand argument to the left hand one
    T operator()(const T& r, const T& x) const
    {
      return (r+x*x);
    }
  };

  /// Functor giving the product of the squares of the arguments
  template <class T> struct TimesSquares: public std::binary_function<T,T,T>
  {
    TimesSquares(){}
    /// Multiplies the squares of the arguments
    T operator()(const T& l, const T& r) const
    {
      return (r*r*l*l);
    }
  };

  /// Square functor
  template <class T> struct Squares: public std::unary_function<T,T>
  {
    Squares(){}
    /// Returns the square of the argument
    T operator()(const T& x) const
    {
      return (x*x);
    }
  };

  /// Log functor
  template <class T> struct Log: public std::unary_function<T,T>
  {
    Log(){}
    /// Returns the logarithm of the argument
    /// @throws std::range_error if x <= 0
    T operator()(const T& x) const
    {
      if (x <= 0 ) throw std::range_error("Attempt to take logarithm of zero or negative number.");
      return std::log(x);
    }
  };

  // Non-throwing version of the Log functor
  template <class T> struct LogNoThrow: public std::unary_function<T,T>
  {
    LogNoThrow(){}
    // Returns the logarithm of the argument
    T operator()(const T& x) const
    {
      return std::log(x);
    }
  };

  /// Divide functor with result reset to 0 if the denominator is null
  template <class T> struct DividesNonNull: public std::binary_function<T,T,T>
  {
    DividesNonNull(){}
    /// Returns l/r if r is non-zero, otherwise returns l.
    T operator()(const T& l, const T& r) const
    {
      if (std::fabs(r)<1e-12) return l;
      return l/r;
    }
  };

  /// A binary functor to compute the simple average of 2 numbers
  template <class T> struct SimpleAverage: public std::binary_function<T,T,T>
  {
    SimpleAverage() {}
    /// Return the average of the two arguments
    T operator()(const T & x, const T & y) const
    {
      return 0.5 * (x + y);
    }
  };



} // namespace VectorHelper
} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_VECTORHELPER_H_*/
