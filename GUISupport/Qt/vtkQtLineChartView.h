/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkQtLineChartView - Wraps a vtkQtChartArea into a VTK view.
//
// .SECTION Description
// vtkQtLineChartView is a type vtkQtChartView designed for line charts.
//
// .SECTION See Also
// vtkQtChartView

#ifndef __vtkQtLineChartView_h
#define __vtkQtLineChartView_h

#include "vtkQtChartViewBase.h"

class vtkQtLineChart;
class vtkQtChartSeriesModelCollection;

class QVTK_EXPORT vtkQtLineChartView : public vtkQtChartViewBase
{
public:
  static vtkQtLineChartView *New();
  vtkTypeRevisionMacro(vtkQtLineChartView, vtkQtChartViewBase);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Updates the view.
  virtual void Update();

  // Description:
  // Adds line chart selection handlers to the mouse selection.
  virtual void AddChartSelectionHandlers(vtkQtChartMouseSelection* selector);

  // Description:
  // Gets the line chart series model.
  virtual vtkQtChartSeriesModelCollection* GetChartSeriesModel();

protected:
  vtkQtLineChartView();
  ~vtkQtLineChartView();

protected:
  vtkQtLineChart *LineChart;
  vtkQtChartSeriesModelCollection *LineModel;

private:
  vtkQtLineChartView(const vtkQtLineChartView&);  // Not implemented.
  void operator=(const vtkQtLineChartView&);  // Not implemented.
};

#endif
