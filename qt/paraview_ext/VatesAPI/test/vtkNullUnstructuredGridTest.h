// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VTKNULLUNSTRUCTUREDGRID_TEST_H_
#define VTKNULLUNSTRUCTUREDGRID_TEST_H_

#include "MantidVatesAPI/vtkNullUnstructuredGrid.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vtkPoints.h>
#include <vtkUnstructuredGrid.h>

using namespace Mantid::VATES;

class vtkNullUnstructuredGridTest : public CxxTest::TestSuite {
public:
  void testCorrectVtkDataSetIsReturned() {
    vtkNullUnstructuredGrid grid;

    vtkUnstructuredGrid *ugrid = nullptr;

    TSM_ASSERT_THROWS_NOTHING(
        "Should create the unstructured grid without problems.",
        ugrid = grid.createNullData());
    TSM_ASSERT("Should have exactly one point",
               ugrid->GetNumberOfPoints() == 1);
    TSM_ASSERT("Should have exactly one cell", ugrid->GetNumberOfCells() == 1);
    vtkPoints *p = ugrid->GetPoints();
    double coord[3];
    p->GetPoint(0, coord);
    TSM_ASSERT("X should be in the center", coord[0] == 0.0);
    TSM_ASSERT("X should be in the center", coord[1] == 0.0);
    TSM_ASSERT("X should be in the center", coord[2] == 0.0);
  }
};
#endif
