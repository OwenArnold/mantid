// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef COLORUPDATER_H_
#define COLORUPDATER_H_

#include "MantidVatesSimpleGuiViewWidgets/AutoScaleRangeGenerator.h"
#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include <QPair>

class pqColorMapModel;
class pqDataRepresentation;
class pqPipelineRepresentation;

class vtkObject;

namespace Json {
class Value;
}

namespace Mantid {
namespace Vates {
namespace SimpleGui {

class ColorSelectionWidget;

/**
 *
  This class handles the application of requests from the ColorSelectionWidget.

  @author Michael Reuter
  @date 15/08/2011
 */
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS ColorUpdater {
public:
  /// Object constructor
  ColorUpdater();
  /// Default destructor
  virtual ~ColorUpdater();

  /**
   * Set the color scale back to the original bounds.
   * @return A struct with minimum, maximum and if log scale is used.
   */
  VsiColorScale autoScale();
  /**
   * Set the requested color map on the data.
   * @param repr the representation to change the color map on
   * @param model the color map to use
   */
  void colorMapChange(pqPipelineRepresentation *repr, const Json::Value &model);
  /**
   * Set the data color scale range to the requested bounds.
   * @param min the minimum bound for the color scale
   * @param max the maximum bound for the color scale
   */
  void colorScaleChange(double min, double max);
  /// Get the auto scaling state.
  bool isAutoScale();
  /// Get the logarithmic scaling state.
  bool isLogScale();
  /// Get the maximum color scaling range value.
  double getMaximumRange();
  /// Get the minimum color scaling range value.
  double getMinimumRange();
  /// Initializes the color scale
  void initializeColorScale();
  /**
   * Set logarithmic color scaling on the data.
   * @param state flag to determine whether or not to use log color scaling
   */
  void logScale(int state);
  /// Print internal information.
  void print();
  /// Update the internal state.
  void updateState(ColorSelectionWidget *cs);

  /// To update the VSI min/mas lineEdits when the user uses the Paraview color
  /// editor
  void observeColorScaleEdited(pqPipelineRepresentation *repr,
                               ColorSelectionWidget *cs);

private:
  /// vtkcallback function for color change events coming from the Paraview
  /// color editor
  static void colorScaleEditedCallbackFunc(vtkObject *caller,
                                           long unsigned int eventID,
                                           void *clientData, void *callData);
  /// vtk callback function for user clicks on log-scale in the Paraview color
  /// editor
  static void logScaleClickedCallbackFunc(vtkObject *caller,
                                          long unsigned int eventID,
                                          void *clientData, void *callData);

  void updateLookupTable(
      pqDataRepresentation *representation); ///< Updates the lookup tables.

  bool m_autoScaleState; ///< Holder for the auto scaling state
  bool m_logScaleState;  ///< Holder for the log scaling state
  double m_minScale;     ///< Holder for the minimum color range state
  double m_maxScale;     ///< Holder for the maximum color range state
  AutoScaleRangeGenerator
      m_autoScaleRangeGenerator; ///< Holds a range generator for auto scale.
};
} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid

#endif // COLORUPDATER_H_
