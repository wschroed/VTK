/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSFile: vtkImageDecomposeFilter.cxx,v $
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all Files associated with the software unless
explicitly disclaimed in individual Files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include <math.h>

#include "vtkImageDecomposeFilter.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageDecomposeFilter* vtkImageDecomposeFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageDecomposeFilter");
  if(ret)
    {
    return (vtkImageDecomposeFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageDecomposeFilter;
}





//----------------------------------------------------------------------------
// Construct an instance of vtkImageDecomposeFilter fitler.
vtkImageDecomposeFilter::vtkImageDecomposeFilter()
{
  this->Dimensionality = 3;
  this->SetNumberOfIterations(3);
}


//----------------------------------------------------------------------------
void vtkImageDecomposeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageIterateFilter::PrintSelf(os,indent);

  os << indent << "Dimensionality: " << this->Dimensionality << "\n";
}

//----------------------------------------------------------------------------
void vtkImageDecomposeFilter::SetDimensionality(int dim)
{  
  if (this->Dimensionality == dim)
    {
    return;
    }
  
  if (dim < 1 || dim > 3)
    {
    vtkErrorMacro("SetDimensionality: Bad dim: " << dim);
    return;
    }
  
  this->Dimensionality = dim;
  this->SetNumberOfIterations(dim);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkImageDecomposeFilter::SetFilteredAxes(int axis)
{
  if (axis != 0)
    {
    vtkErrorMacro("If only one axis is specified, it must be X");
    return;
    }
  this->SetDimensionality(1);
}

//----------------------------------------------------------------------------
void vtkImageDecomposeFilter::SetFilteredAxes(int axis0, int axis1)
{
  if (axis0 != 0 || axis1 != 1)
    {
    vtkErrorMacro("If only two axes are specified, they must be X, Y");
    return;
    }
  this->SetDimensionality(2);
}


//----------------------------------------------------------------------------
void vtkImageDecomposeFilter::SetFilteredAxes(int axis0, int axis1, int axis2)
{
  if (axis0 != 0 || axis1 != 1 || axis2 != 2)
    {
    vtkErrorMacro("Axes must be order X, Y, Z");
    return;
    }
  this->SetDimensionality(3);
}



//----------------------------------------------------------------------------
void vtkImageDecomposeFilter::PermuteIncrements(int *increments, int &inc0, 
						int &inc1, int &inc2)
{
  switch (this->Iteration)
    {
    case 0:
      inc0 = increments[0];
      inc1 = increments[1];
      inc2 = increments[2];
      break;
    case 1:
      inc1 = increments[0];
      inc0 = increments[1];
      inc2 = increments[2];
      break;
    case 2:
      inc1 = increments[0];
      inc2 = increments[1];
      inc0 = increments[2];
      break;
    }
}


//----------------------------------------------------------------------------
void vtkImageDecomposeFilter::PermuteExtent(int *extent, int &min0, int &max0, 
					    int &min1, int &max1,
					    int &min2, int &max2)
{
  switch (this->Iteration)
    {
    case 0:
      min0 = extent[0];       max0 = extent[1];
      min1 = extent[2];       max1 = extent[3];
      min2 = extent[4];       max2 = extent[5];
      break;
    case 1:
      min1 = extent[0];       max1 = extent[1];
      min0 = extent[2];       max0 = extent[3];
      min2 = extent[4];       max2 = extent[5];
      break;
    case 2:
      min1 = extent[0];       max1 = extent[1];
      min2 = extent[2];       max2 = extent[3];
      min0 = extent[4];       max0 = extent[5];
      break;
    }
}


















