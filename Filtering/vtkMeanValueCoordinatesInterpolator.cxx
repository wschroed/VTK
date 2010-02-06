/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkMeanValueCoordinatesInterpolator.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMeanValueCoordinatesInterpolator.h"
#include "vtkObjectFactory.h"

#include "vtkPoints.h"
#include "vtkIdList.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkCellArray.h"


vtkCxxRevisionMacro(vtkMeanValueCoordinatesInterpolator, "$Revision: 1.83 $");
vtkStandardNewMacro(vtkMeanValueCoordinatesInterpolator);

// Special class that can iterate over different type of triangle representations
class vtkTriIterator
{
public:
  vtkIdType *Tris;
  vtkIdType *Current;
  vtkIdType Offset;
  vtkIdType NumberOfTriangles;
  vtkIdType Id;

  vtkTriIterator(vtkIdType numIds,vtkIdType offset) :
    Tris(NULL),Current(NULL),Offset(offset),NumberOfTriangles(0),Id(-1) 
    {
      this->NumberOfTriangles = numIds / offset;
    }
  vtkIdType* operator++()
    {
      this->Current += this->Offset;
      this->Id++;
      return this->Current;
    }
  vtkIdType GetNumberOfTriangles()
    {
      return this->NumberOfTriangles;
    }
  vtkIdType GetId()
    {
      return this->Id;
    }
};
  

//----------------------------------------------------------------------------
// Construct object with default tuple dimension (number of components) of 1.
vtkMeanValueCoordinatesInterpolator::
vtkMeanValueCoordinatesInterpolator()
{
}

//----------------------------------------------------------------------------
vtkMeanValueCoordinatesInterpolator::
~vtkMeanValueCoordinatesInterpolator()
{
}

//----------------------------------------------------------------------------
// Templated function to generate weights. The weights and points data types
// are the same. This class actually implements the algorithm. (Note: the input
// types really should be float or double.)
template <class T>
void vtkComputeMVCWeights(T x[3], T *pts, vtkIdType npts, 
                          vtkIdType *tris, vtkTriIterator& iter,
                          T *weights)
{
  //Points are organized {(x,y,z), (x,y,z), ....}
  //Tris are organized {(i,j,k), (i,j,k), ....}
  //Weights per point are computed
  
  // Begin by initializing weights. Too bad we can't use memset.
  for (vtkIdType pid=0; pid < npts; ++pid)
    {
    weights[pid] = static_cast<T>(0.0);
    }
         
  // Now loop over all triangle to compute weights
  vtkIdType tid=iter.GetId(), ntris = iter.GetNumberOfTriangles();
  vtkIdType *tri;
  while ( tid < ntris)
    {
    tri = ++iter;
    //algorithm goes here
    tid = iter.GetId();
    }
}

//----------------------------------------------------------------------------
// Static function to compute weights
void vtkMeanValueCoordinatesInterpolator::
ComputeInterpolationWeights(double x[3], vtkPoints *pts, vtkIdList *tris, vtkDataArray *weights)
{
  // Check the input
  if ( !tris )
    {
    vtkGenericWarningMacro("Did not provide triangles");
    return;
    }
  vtkIdType *t = tris->GetPointer(0);
  // Below the vtkCellArray has three entries per triangle {(i,j,k), (i,j,k), ....}
  vtkTriIterator iter(tris->GetNumberOfIds(),3);

  vtkMeanValueCoordinatesInterpolator::
    ComputeInterpolationWeights(x,pts,t,iter,weights);
}

//----------------------------------------------------------------------------
// Static function to compute weights
void vtkMeanValueCoordinatesInterpolator::
ComputeInterpolationWeights(double x[3], vtkPoints *pts, vtkCellArray *tris, vtkDataArray *weights)
{
  // Check the input
  if ( !tris )
    {
    vtkGenericWarningMacro("Did not provide triangles");
    return;
    }
  vtkIdType *t = tris->GetPointer();
  // Below the vtkCellArray has four entries per triangle {(3,i,j,k), (3,i,j,k), ....}
  vtkTriIterator iter(tris->GetNumberOfConnectivityEntries(),4);

  vtkMeanValueCoordinatesInterpolator::
    ComputeInterpolationWeights(x,pts,t,iter,weights);
}

//----------------------------------------------------------------------------
// Static function to compute weights
void vtkMeanValueCoordinatesInterpolator::
ComputeInterpolationWeights(double x[3], vtkPoints *pts, vtkIdType *tris, 
                            vtkTriIterator& iter, vtkDataArray *weights)
{
  // Check the input
  if ( !pts || !weights)
    {
    vtkGenericWarningMacro("Did not provide proper input");
    return;
    }

  int pType = pts->GetDataType();
  int wType = pts->GetDataType();
  if ( pType != wType || pType != VTK_FLOAT || pType != VTK_DOUBLE )
    {
    vtkGenericWarningMacro("Points and weights should be same type (either float or double)");
    return;
    }
  
  // Prepare the arrays
  vtkIdType numPts = pts->GetNumberOfPoints();
  weights->SetNumberOfComponents(1);
  weights->SetNumberOfTuples(numPts);

  if ( numPts <= 0 )
    {
    return;
    }
  
  void *p = pts->GetVoidPointer(0);
  void *w = weights->GetVoidPointer(0);
  
  // call templated function to compute the weights. Note that we do not
  // use VTK's template macro because we are limiting usage to floats and doubles.
  switch (pts->GetDataType())
    {
    case VTK_FLOAT:
      float xf[3];
      xf[0] = static_cast<float>(x[0]);
      xf[1] = static_cast<float>(x[1]);
      xf[2] = static_cast<float>(x[2]);
      vtkComputeMVCWeights(xf, static_cast<float*>(p), numPts, tris, iter, static_cast<float*>(w));
      break;

    case VTK_DOUBLE:
      vtkComputeMVCWeights(x, static_cast<double*>(p), numPts, tris, iter, static_cast<double*>(w));
      break;

    default:
      break;
    }
}


//----------------------------------------------------------------------------
void vtkMeanValueCoordinatesInterpolator::
PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}
