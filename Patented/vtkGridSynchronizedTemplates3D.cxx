/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$



Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

    THIS CLASS IS PATENT PENDING.

    Application of this software for commercial purposes requires 
    a license grant from Kitware. Contact:
        Ken Martin
        Kitware
        469 Clifton Corporate Parkway,
        Clifton Park, NY 12065
        Phone:1-518-371-3971 
    for more information.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <math.h>
#include "vtkStructuredPoints.h"
#include "vtkCharArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkShortArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkIntArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkLongArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGridSynchronizedTemplates3D.h"
#include "vtkSynchronizedTemplates3D.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkGridSynchronizedTemplates3D* vtkGridSynchronizedTemplates3D::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkGridSynchronizedTemplates3D");
  if(ret)
    {
    return (vtkGridSynchronizedTemplates3D*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkGridSynchronizedTemplates3D;
}






//----------------------------------------------------------------------------
// Description:
// Construct object with initial scalar range (0,1) and single contour value
// of 0.0. The ImageRange are set to extract the first k-plane.
vtkGridSynchronizedTemplates3D::vtkGridSynchronizedTemplates3D()
{
  int idx;

  this->ContourValues = vtkContourValues::New();
  this->ComputeNormals = 1;
  this->ComputeGradients = 0;
  this->ComputeScalars = 1;

  this->ExecuteExtent[0] = this->ExecuteExtent[1] 
    = this->ExecuteExtent[2] = this->ExecuteExtent[3] 
    = this->ExecuteExtent[4] = this->ExecuteExtent[5] = 0;

  this->MinimumPieceSize[0] = 10;
  this->MinimumPieceSize[1] = 10;
  this->MinimumPieceSize[2] = 10;

  this->Threader = vtkMultiThreader::New();
  this->NumberOfThreads = this->Threader->GetNumberOfThreads();
  for (idx = 0; idx < VTK_MAX_THREADS; ++idx)
    {
    this->Threads[idx] = NULL;
    }
}

//----------------------------------------------------------------------------
vtkGridSynchronizedTemplates3D::~vtkGridSynchronizedTemplates3D()
{
  this->ContourValues->Delete();
  this->Threader->Delete();
}


//----------------------------------------------------------------------------
void vtkGridSynchronizedTemplates3D::SetInputMemoryLimit(long limit)
{
  vtkErrorMacro( << "This filter no longer supports a memory limit." );
  vtkErrorMacro( << "This filter no longer initiates streaming." );
  vtkErrorMacro( << "Please use a .... after this filter to achieve similar functionality." );
}


//----------------------------------------------------------------------------
// Description:
// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
unsigned long vtkGridSynchronizedTemplates3D::GetMTime()
{
  unsigned long mTime=this->vtkStructuredGridToPolyDataFilter::GetMTime();
  unsigned long mTime2=this->ContourValues->GetMTime();

  mTime = ( mTime2 > mTime ? mTime2 : mTime );
  return mTime;
}

//----------------------------------------------------------------------------
// Close to central differences for a grid as I could get.
// Given a linear gradient assumption find gradient that minimizes
// error squared for + and - (*3) neighbors).
template <class T>
static void ComputeGridPointGradient(int i, int j, int k, int inExt[6], 
                            int incY, int incZ, T *sc, float *pt, float g[3])
{
  float N[6][3];
  double NtN[3][3], NtNi[3][3];
  double *NtN2[3], *NtNi2[3];
  double tmpDoubleArray[3];
  int tmpIntArray[3];
  float s[6], Nts[3], sum;
  int count = 0;
  T *s2;
  float *p2;

  if (i == 2 && k == 2)
    {
    count = 0;
    }

  // x-direction
  if (i > inExt[0])
    {
    p2 = pt - 3;
    s2 = sc - 1;
    N[count][0] = p2[0] - pt[0];
    N[count][1] = p2[1] - pt[1];
    N[count][2] = p2[2] - pt[2];
    s[count] = (float)(*s2) - (float)(*sc);
    ++count;
    }
  if (i < inExt[1])
    {
    p2 = pt + 3;
    s2 = sc + 1;
    N[count][0] = p2[0] - pt[0];
    N[count][1] = p2[1] - pt[1];
    N[count][2] = p2[2] - pt[2];
    s[count] = (float)(*s2) - (float)(*sc);
    ++count;
    }

  // y-direction
  if (j > inExt[2])
    {
    p2 = pt - 3*incY;
    s2 = sc - incY;
    N[count][0] = p2[0] - pt[0];
    N[count][1] = p2[1] - pt[1];
    N[count][2] = p2[2] - pt[2];
    s[count] = (float)(*s2) - (float)(*sc);
    ++count;
    }
  if (j < inExt[3])
    {
    p2 = pt + 3*incY;
    s2 = sc + incY;
    N[count][0] = p2[0] - pt[0];
    N[count][1] = p2[1] - pt[1];
    N[count][2] = p2[2] - pt[2];
    s[count] = (float)(*s2) - (float)(*sc);
    ++count;
    }

  // z-direction
  if (k > inExt[4])
    {
    p2 = pt - 3*incZ;
    s2 = sc - incZ;
    N[count][0] = p2[0] - pt[0];
    N[count][1] = p2[1] - pt[1];
    N[count][2] = p2[2] - pt[2];
    s[count] = (float)(*s2) - (float)(*sc);
    ++count;
    }
  if (k < inExt[5])
    {
    p2 = pt + 3*incZ;
    s2 = sc + incZ;
    N[count][0] = p2[0] - pt[0];
    N[count][1] = p2[1] - pt[1];
    N[count][2] = p2[2] - pt[2];
    s[count] = (float)(*s2) - (float)(*sc);
    ++count;
    }

  // compute transpose(N)N.
  // since this will be a symetric matrix, we could make the
  // computation a little more efficient.
  for (i = 0; i < 3; ++i)
    {
    for (j = 0; j < 3; ++j)
      {
      sum = 0.0;
      for (k = 0; k < count; ++k)
        {
        sum += N[k][i] * N[k][j];
        }
      NtN[i][j] = sum;
      }
    }
  // compute the inverse of NtN
  // We have to setup a double** for the invert matrix call (@#$%!&%$!) 
  NtN2[0] = &(NtN[0][0]);
  NtN2[1] = &(NtN[1][0]);
  NtN2[2] = &(NtN[2][0]);
  NtNi2[0] = &(NtNi[0][0]);
  NtNi2[1] = &(NtNi[1][0]);
  NtNi2[2] = &(NtNi[2][0]);
  if (vtkMath::InvertMatrix(NtN2, NtNi2, 3, tmpIntArray, tmpDoubleArray) == 0)
    {
    vtkGenericWarningMacro("Cannot compute gradient of grid");
    return;
    }

  // compute transpose(N)s.
  for (i = 0; i < 3; ++i)
    {
    sum = 0.0;
    for (j = 0; j < count; ++j)
      {
      sum += N[j][i] * s[j];
      }
    Nts[i] = sum;
    }
    
  // now compute gradient
  for (i = 0; i < 3; ++i)
    {
    sum = 0.0;
    for (j = 0; j < 3; ++j)
      {
      sum += NtNi[j][i] * Nts[j];
      }
    g[i] = sum;
    }
} 


   


//----------------------------------------------------------------------------
#define VTK_CSP3PA(i2,j2,k2,s,p, grad, norm) \
if (NeedGradients) \
  { \
  if (!g0) \
    { \
    ComputeGridPointGradient(i, j, k, inExt, incY, incZ, s0, p0, n0); \
    g0 = 1; \
    } \
  ComputeGridPointGradient(i2, j2, k2, inExt, incY, incZ, s, p, n1); \
  for (jj=0; jj<3; jj++) \
    { \
    grad[jj] = n0[jj] + t * (n1[jj] - n0[jj]); \
    } \
  if (ComputeNormals) \
    { \
    norm[0] = -grad[0];  norm[1] = -grad[1];  norm[2] = -grad[2]; \
    vtkMath::Normalize(norm); \
    }   \
  }


//----------------------------------------------------------------------------
// Contouring filter specialized for images
template <class T>
static void ContourGrid(vtkGridSynchronizedTemplates3D *self,
			int vtkNotUsed(threadId),
			int *exExt, T *scalars, vtkPoints *newPts,
			vtkScalars *newScalars, vtkCellArray *polys,
			vtkNormals *newNormals, vtkVectors *newGradients)
{
  int *inExt = self->GetInput()->GetExtent();
  int xdim = exExt[1] - exExt[0] + 1;
  int ydim = exExt[3] - exExt[2] + 1;
  float n0[3], n1[3];  // used in gradient macro
  float *values = self->GetValues();
  int numContours = self->GetNumberOfContours();
  float *inPtPtrX, *inPtPtrY, *inPtPtrZ;
  float *p0, *p1, *p2, *p3;
  T *inPtrX, *inPtrY, *inPtrZ;
  T *s0, *s1, *s2, *s3;
  int XMin, XMax, YMin, YMax, ZMin, ZMax;
  int incY, incZ;
  vtkPoints *inPts = self->GetInput()->GetPoints();
  float t;
  int *isect1Ptr, *isect2Ptr;
  int ptIds[3];
  int *tablePtr;
  int v0, v1, v2, v3;
  int idx, vidx;
  float value;
  int i, j, k;
  int zstep, yisectstep;
  int offsets[12];
  int ComputeNormals = self->GetComputeNormals();
  int ComputeGradients = self->GetComputeGradients();
  int ComputeScalars = self->GetComputeScalars();
  int NeedGradients = ComputeGradients || ComputeNormals;
  int jj, g0;
  // add point info all together to eliminated locks
  float x1[3], x2[3], x3[3];
  float grad1[3], grad2[3], grad3[3];
  float norm1[3], norm2[3], norm3[3];
  int *idPtr1, *idPtr2, *idPtr3;
  
  // this is an exploded execute extent.
  XMin = exExt[0];
  XMax = exExt[1];
  YMin = exExt[2];
  YMax = exExt[3];
  ZMin = exExt[4];
  ZMax = exExt[5];
  // to skip over an x row of the input.
  incY = inExt[1]-inExt[0]+1;
  // to skip over an xy slice of the input.
  incZ = (inExt[3]-inExt[2]+1)*incY;

  // Kens increments, probably to do with edge array
  zstep = xdim*ydim;
  yisectstep = xdim*3;
  // compute offsets probably how to get to the edges in the edge array.
  offsets[0] = -xdim*3;
  offsets[1] = -xdim*3 + 1;
  offsets[2] = -xdim*3 + 2;
  offsets[3] = -xdim*3 + 4;
  offsets[4] = -xdim*3 + 5;
  offsets[5] = 0;
  offsets[6] = 2;
  offsets[7] = 5;
  offsets[8] = (zstep - xdim)*3;
  offsets[9] = (zstep - xdim)*3 + 1;
  offsets[10] = (zstep - xdim)*3 + 4;
  offsets[11] = zstep*3;
    
  // allocate storage array
  int *isect1 = new int [xdim*ydim*3*2];
  // set impossible edges to -1
  for (i = 0; i < ydim; i++)
    {
    isect1[(i+1)*xdim*3-3] = -1;
    isect1[(i+1)*xdim*3*2-3] = -1;
    }
  for (i = 0; i < xdim; i++)
    {
    isect1[((ydim-1)*xdim + i)*3 + 1] = -1;
    isect1[((ydim-1)*xdim + i)*3*2 + 1] = -1;
    }
  
  
  //fprintf(stderr, "%d: -------- Extent %d, %d, %d, %d, %d, %d\n", threadId,
  //	  exExt[0], exExt[1], exExt[2], exExt[3], exExt[4], exExt[5]);

  // for each contour
  for (vidx = 0; vidx < numContours; vidx++)
    {
    value = values[vidx];
    //  skip any slices which are overlap for computing gradients.
    inPtPtrZ = inPts->GetPoint((ZMin - inExt[4]) * incZ +
                               (YMin - inExt[2]) * incY +
                               (XMin - inExt[0]));
    inPtrZ = scalars + ((ZMin - inExt[4]) * incZ +
                        (YMin - inExt[2]) * incY +
                        (XMin - inExt[0]));
    p2 = inPtPtrZ;
    s2 = inPtrZ;
    v2 = (*s2 < value ? 0 : 1);

    //==================================================================
    for (k = ZMin; k <= ZMax; k++)
      {
      // swap the buffers
      if (k%2)
        {
	      offsets[8] = (zstep - xdim)*3;
        offsets[9] = (zstep - xdim)*3 + 1;
        offsets[10] = (zstep - xdim)*3 + 4;
        offsets[11] = zstep*3;
        isect1Ptr = isect1;
        isect2Ptr = isect1 + xdim*ydim*3;
        }
      else
        { 
        offsets[8] = (-zstep - xdim)*3;
        offsets[9] = (-zstep - xdim)*3 + 1;
        offsets[10] = (-zstep - xdim)*3 + 4;
        offsets[11] = -zstep*3;
        isect1Ptr = isect1 + xdim*ydim*3;
        isect2Ptr = isect1;
        }

      inPtPtrY = inPtPtrZ;
      inPtrY = inPtrZ;
      for (j = YMin; j <= YMax; j++)
        {
        p1 = inPtPtrY;
        s1 = inPtrY;
        v1 = (*s1 < value ? 0 : 1);
        inPtPtrX = inPtPtrY;
        inPtrX = inPtrY;
        for (i = XMin; i <= XMax; i++)
	  {
          p0 = p1;
	  s0 = s1;
	  v0 = v1;
          // this flag keeps up from computing gradient for grid point 0 twice.
	  g0 = 0;
          if (i < XMax)
	    {
	    p1 = (inPtPtrX + 3);
	    s1 = (inPtrX + 1);
	    v1 = (*s1 < value ? 0 : 1);
	    if (v0 ^ v1)
	      {
	      t = (value - (float)(*s0)) / ((float)(*s1) - (float)(*s0));
              x1[0] = p0[0] + t*(p1[0] - p0[0]);
              x1[1] = p0[1] + t*(p1[1] - p0[1]);
              x1[2] = p0[2] + t*(p1[2] - p0[2]);
              idPtr1 = isect2Ptr;
              VTK_CSP3PA(i+1,j,k,s1,p1,grad1,norm1);
              }
            else
              {
              idPtr1 = NULL;
              *isect2Ptr = -1;
              }
            }
          if (j < YMax)
	    {
	    p2 = (inPtPtrX + incY*3);
	    s2 = (inPtrX + incY);
	    v2 = (*s2 < value ? 0 : 1);
	    if (v0 ^ v2)
	      {
	      t = (value - (float)(*s0)) / ((float)(*s2) - (float)(*s0));
	      x2[0] = p0[0] + t*(p2[0] - p0[0]);
	      x2[1] = p0[1] + t*(p2[1] - p0[1]);
	      x2[2] = p0[2] + t*(p2[2] - p0[2]);
	      idPtr2 = (isect2Ptr + 1);
	      VTK_CSP3PA(i,j+1,k,s2,p2,grad2,norm2);
	      }
	    else
	      {
              idPtr2 = NULL;
	      *(isect2Ptr + 1) = -1;
	      }
	    }
	  if (k < ZMax)
	    {
	    p3 = (inPtPtrX + incZ*3);
	    s3 = (inPtrX + incZ);
	    v3 = (*s3 < value ? 0 : 1);
	    if (v0 ^ v3)
	      {
	      t = (value - (float)(*s0)) / ((float)(*s3) - (float)(*s0));
              x3[0] = p0[0] + t*(p3[0] - p0[0]);
              x3[1] = p0[1] + t*(p3[1] - p0[1]);
              x3[2] = p0[2] + t*(p3[2] - p0[2]);
              idPtr3 = isect2Ptr + 2;
	      VTK_CSP3PA(i,j,k+1,s3,p3,grad3,norm3);
	      }
	    else
	      {
              idPtr3 = NULL;
	      *(isect2Ptr + 2) = -1;
	      }
	    }
	  
          // add all of the points at once to minimize locks
          if (idPtr1 != NULL)
            {
            *idPtr1 = newPts->InsertNextPoint(x1);
            if (ComputeScalars) 
              { 
              newScalars->InsertNextScalar(value); 
              }
            if (ComputeGradients) 
              {
              newGradients->InsertNextVector(grad1); 
              }
            if (ComputeNormals)
              {
	      newNormals->InsertNextNormal(norm1);
              }
            }
          if (idPtr2 != NULL)
            {
            *idPtr2 = newPts->InsertNextPoint(x2);
            if (ComputeScalars) 
              { 
              newScalars->InsertNextScalar(value); 
              }
            if (ComputeGradients) 
              {
              newGradients->InsertNextVector(grad2); 
              }
            if (ComputeNormals)
              {
	      newNormals->InsertNextNormal(norm2);
              }
            }
          if (idPtr3 != NULL)
            {
            *idPtr3 = newPts->InsertNextPoint(x3);
            if (ComputeScalars) 
              { 
              newScalars->InsertNextScalar(value); 
              }
            if (ComputeGradients) 
              {
              newGradients->InsertNextVector(grad3); 
              }
            if (ComputeNormals)
              {
	      newNormals->InsertNextNormal(norm3);
              }
            }
	  
	  // now add any polys that need to be added
	  // basically look at the isect values, 
	  // form an index and lookup the polys
	  if (j > YMin && i < XMax && k > ZMin)
	    {
	    idx = (v0 ? 4096 : 0);
	    idx = idx + (*(isect1Ptr - yisectstep) > -1 ? 2048 : 0);
	    idx = idx + (*(isect1Ptr -yisectstep +1) > -1 ? 1024 : 0);
	    idx = idx + (*(isect1Ptr -yisectstep +2) > -1 ? 512 : 0);
	    idx = idx + (*(isect1Ptr -yisectstep +4) > -1 ? 256 : 0);
	    idx = idx + (*(isect1Ptr -yisectstep +5) > -1 ? 128 : 0);
	    idx = idx + (*(isect1Ptr) > -1 ? 64 : 0);
	    idx = idx + (*(isect1Ptr + 2) > -1 ? 32 : 0);
	    idx = idx + (*(isect1Ptr + 5) > -1 ? 16 : 0);
	    idx = idx + (*(isect2Ptr -yisectstep) > -1 ? 8 : 0);
	    idx = idx + (*(isect2Ptr -yisectstep +1) > -1 ? 4 : 0);
	    idx = idx + (*(isect2Ptr -yisectstep +4) > -1 ? 2 : 0);
	    idx = idx + (*(isect2Ptr) > -1 ? 1 : 0);
	    
	    tablePtr = VTK_SYNCHONIZED_TEMPLATES_3D_TABLE_2 
	      + VTK_SYNCHONIZED_TEMPLATES_3D_TABLE_1[idx];
            // to protect data against multiple threads
	    while (*tablePtr != -1)
	      {
	      ptIds[0] = *(isect1Ptr + offsets[*tablePtr]);
	      tablePtr++;
	      ptIds[1] = *(isect1Ptr + offsets[*tablePtr]);
	      tablePtr++;
	      ptIds[2] = *(isect1Ptr + offsets[*tablePtr]);
	      tablePtr++;
	      polys->InsertNextCell(3,ptIds);
	      }
	    }
	  inPtPtrX += 3;
	  ++inPtrX;
	  isect2Ptr += 3;
	  isect1Ptr += 3;
	  }
        inPtPtrY += 3*incY;
        inPtrY += incY;
	}
      inPtPtrZ += 3*incZ;
      inPtrZ += incZ;
      }
    }

  delete [] isect1;
}

//----------------------------------------------------------------------------
void vtkGridSynchronizedTemplates3D::InitializeOutput(int *ext,vtkPolyData *o)
{
  vtkPoints *newPts;
  vtkCellArray *newPolys;
  vtkScalars *newScalars;
  vtkNormals *newNormals;
  vtkVectors *newGradients;
  long estimatedSize;

  estimatedSize = (int) pow ((double) 
      ((ext[1]-ext[0]+1)*(ext[3]-ext[2]+1)*(ext[5]-ext[4]+1)), .75);
  if (estimatedSize < 1024)
    {
    estimatedSize = 1024;
    }
  newPts = vtkPoints::New();
  newPts->Allocate(estimatedSize,estimatedSize);
  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(estimatedSize,3));

  if (this->ComputeNormals)
    {
    newNormals = vtkNormals::New();
    newNormals->Allocate(estimatedSize,estimatedSize/2);
    }
  else
    {
    newNormals = NULL;
    }
  
  if (this->ComputeGradients)
    {
    newGradients = vtkVectors::New();
    newGradients->Allocate(estimatedSize,estimatedSize/2);
    }
  else
    {
    newGradients = NULL;
    }

  if (this->ComputeScalars)
    {
    newScalars = vtkScalars::New();
    newScalars->Allocate(estimatedSize,estimatedSize/2);
    }
  else
    {
    newScalars = NULL;
    }

  o->SetPoints(newPts);
  newPts->Delete();

  o->SetPolys(newPolys);
  newPolys->Delete();

  if (newScalars)
    {
    o->GetPointData()->SetScalars(newScalars);
    newScalars->Delete();
    }
  if (newGradients)
    {
    o->GetPointData()->SetVectors(newGradients);
    newGradients->Delete();
    }
  if (newNormals)
    {
    o->GetPointData()->SetNormals(newNormals);
    newNormals->Delete();
    }
}

//----------------------------------------------------------------------------
// Contouring filter specialized for images (or slices from images)
void vtkGridSynchronizedTemplates3D::ThreadedExecute(int *exExt, int threadId)
{
  vtkStructuredGrid *input= this->GetInput();
  vtkPointData *pd = input->GetPointData();
  vtkPoints *newPts;
  vtkCellArray *newPolys;
  vtkScalars *inScalars = pd->GetScalars();
  vtkPolyData *output = this->GetOutput();
  int *dims = this->GetInput()->GetDimensions();
  int *inUpdateExtent = input->GetUpdateExtent();
  long dataSize;
  vtkScalars *newScalars;
  vtkNormals *newNormals;
  vtkVectors *newGradients;
  
  // The first thread always writes into the output.
  // Other threads must create a temporary polydata as output.
  if (threadId > 0)
    {
    output = this->Threads[threadId] = vtkPolyData::New();
    this->InitializeOutput(exExt, output);
    }

  vtkDebugMacro(<< "Executing 3D structured contour");
  
  if ( inScalars == NULL )
    {
    vtkErrorMacro(<<"Scalars must be defined for contouring");
    return;
    }

  if ( input->GetDataDimension() != 3 )
    {
    vtkErrorMacro(<<"3D structured contours requires 3D data");
    return;
    }

  //
  // Check dimensionality of data and get appropriate form
  //
  dataSize = (exExt[1]-exExt[0]+1) * (exExt[3]-exExt[2]+1)
                * (exExt[5]-exExt[4]+1);

  newPts = output->GetPoints();
  newPolys = output->GetPolys();
  newScalars = output->GetPointData()->GetScalars();
  newNormals = output->GetPointData()->GetNormals();
  newGradients = output->GetPointData()->GetVectors();

  //
  // Check data type and execute appropriate function
  //
  if (inScalars->GetNumberOfComponents() == 1 )
    {
    void *scalars = inScalars->GetVoidPointer(0);
    switch (inScalars->GetDataType())
      {
      vtkTemplateMacro9(ContourGrid, this, threadId, exExt, (VTK_TT *)scalars, 
                        newPts, newScalars, newPolys, newNormals, 
                        newGradients);
      }//switch
    }
  else //multiple components - have to convert
    {
    vtkScalars *image = vtkScalars::New();
    image->Allocate(dataSize);
    inScalars->GetScalars(0,dataSize,image);
    float *scalars = ((vtkFloatArray *)image->GetData())->GetPointer(0);
    ContourGrid(this, threadId, exExt, scalars, newPts, newScalars, newPolys,
		 newNormals, newGradients);
    image->Delete();
    }
}


//----------------------------------------------------------------------------
void vtkGridSynchronizedTemplates3D::ExecuteInformation()
{
  // Most of this code is for estimating the whole size - this
  // needs to be changed to update size and moved to another method
  // Just the last line which sets the outputs maximum number of 
  // pieces is still needed here.

  //  vtkStructuredGrid *input = this->GetInput();
  //  int *ext, dims[3];
  //  long numPts, numTris;
  //  long sizePt, sizeTri;

  //  // swag at the output memory size
  //  // Outside surface.
  //  ext = input->GetWholeExtent();
  //  dims[0] = ext[1]-ext[0]+1;
  //  dims[1] = ext[3]-ext[2]+1;
  // dims[2] = ext[5]-ext[4]+1;  
  //  numPts = 2 * (dims[0]*dims[1] + dims[0]*dims[2] + dims[1]*dims[2]);
  //  numTris = numPts * 2;
  // Determine the memory for each point and triangle.
  //  sizeTri = 4 * sizeof(int);
  //  sizePt = 3 * sizeof(float);
  //  if (this->ComputeNormals)
  //    {
  //    sizePt += 3 * sizeof(float);
  //    }
  //  if (this->ComputeGradients)
  //    {
  //    sizePt += 3 * sizeof(float);
  //    }
  //  if (this->ComputeScalars)
  //    {
  //    sizePt += sizeof(float);
  //    }
  //  // Set the whole output estimated memory size in kBytes.
  //  // be careful not to overflow.
  //  numTris = numTris / 1000;
  //  if (numTris == 0)
  //   {
  //    numTris = 1;
  //    }
  //  numPts = numPts / 1000;
  //  if (numPts == 0)
  //    {
  //    numPts = 1;
  //    }
  //  this->GetOutput()->SetEstimatedWholeMemorySize(
  //			numTris*sizeTri + numPts*sizePt);

  this->GetOutput()->SetMaximumNumberOfPieces(100000);
    
}

//----------------------------------------------------------------------------

void vtkGridSynchronizedTemplates3D::ComputeInputUpdateExtents( vtkDataObject *out )

{
  vtkStructuredGrid *input = this->GetInput();
  vtkPolyData *output = (vtkPolyData *)out;
  int piece, numPieces;
  int *wholeExt;
  int ext[6];

  if (!input)
    {
    vtkErrorMacro(<< "Input not set.");
    return;
    }

  wholeExt = input->GetWholeExtent();

  // Get request from output
  output->GetUpdateExtent(piece, numPieces);

  // Start with the whole grid.
  input->GetWholeExtent(ext);  

  // get the extent associated with the piece.
  this->SplitExtent(piece, numPieces, ext);

  // As a side product of this call, ExecuteExtent is set.
  // This is the region that we are really updating, although
  // we may require a larger input region in order to generate
  // it if normals / gradients are being computed

  this->ExecuteExtent[0] = ext[0];
  this->ExecuteExtent[1] = ext[1];
  this->ExecuteExtent[2] = ext[2];
  this->ExecuteExtent[3] = ext[3];
  this->ExecuteExtent[4] = ext[4];
  this->ExecuteExtent[5] = ext[5];

  // expand if we need to compute gradients
  if (this->ComputeGradients || this->ComputeNormals)
    {
    ext[0] -= 1;
    if (ext[0] < wholeExt[0])
      {
      ext[0] = wholeExt[0];
      }
    ext[1] += 1;
    if (ext[1] > wholeExt[1])
      {
      ext[1] = wholeExt[1];
      }

    ext[2] -= 1;
    if (ext[2] < wholeExt[2])
      {
      ext[2] = wholeExt[2];
      }
    ext[3] += 1;
    if (ext[3] > wholeExt[3])
      {
      ext[3] = wholeExt[3];
      }

    ext[4] -= 1;
    if (ext[4] < wholeExt[4])
      {
      ext[4] = wholeExt[4];
      }
    ext[5] += 1;
    if (ext[5] > wholeExt[5])
      {
      ext[5] = wholeExt[5];
      }
    }

  // Set the update extent of the input.
  input->SetUpdateExtent(ext);
}

//----------------------------------------------------------------------------

// Assumes UpdateInformation was called first.
int vtkGridSynchronizedTemplates3D::SplitExtent(int piece, int numPieces,
                                                int *ext)
{
  int numPiecesInFirstHalf;
  int size[3], mid, splitAxis;

  // keep splitting until we have only one piece.
  // piece and numPieces will always be relative to the current ext. 
  while (numPieces > 1)
    {
    size[0] = ext[1]-ext[0];
    size[1] = ext[3]-ext[2];
    size[2] = ext[5]-ext[4];
    if (size[2] > size[1] && size[2] > size[0] && 
	size[2]/2 > this->MinimumPieceSize[2])
      {
      splitAxis = 2;
      }
    else if (size[1] > size[0] && size[1]/2 > this->MinimumPieceSize[1])
      {
      splitAxis = 1;
      }
    else if (size[0]/2 > this->MinimumPieceSize[0])
      {
      splitAxis = 0;
      }
    else
      {
      // signal no more splits possible
      splitAxis = -1;
      }

    if (splitAxis == -1)
      {
      // can not split any more.
      if (piece == 0)
        {
        // just return the remaining piece
        numPieces = 1;
        }
      else
        {
        // the rest must be empty
        return 0;
        }
      }
    else
      {
      // split the chosen axis into two pieces.
      numPiecesInFirstHalf = (numPieces / 2);
      mid = (size[splitAxis] * numPiecesInFirstHalf / numPieces) 
	+ ext[splitAxis*2];
      if (piece < numPiecesInFirstHalf)
        {
        // piece is in the first half
        // set extent to the first half of the previous value.
        ext[splitAxis*2+1] = mid;
        // piece must adjust.
        numPieces = numPiecesInFirstHalf;
        }
      else
        {
        // piece is in the second half.
        // set the extent to be the second half. (two halves share points)
        ext[splitAxis*2] = mid;
        // piece must adjust
        numPieces = numPieces - numPiecesInFirstHalf;
        piece -= numPiecesInFirstHalf;
        }
      }
    } // end of while

  return 1;
}

//----------------------------------------------------------------------------
void vtkGridSynchronizedTemplates3D::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredGridToPolyDataFilter::PrintSelf(os,indent);

  this->ContourValues->PrintSelf(os,indent);

  os << indent << "Compute Normals: " << (this->ComputeNormals ? "On\n" : "Off\n");
  os << indent << "Compute Gradients: " << (this->ComputeGradients ? "On\n" : "Off\n");
  os << indent << "Compute Scalars: " << (this->ComputeScalars ? "On\n" : "Off\n");
  os << indent << "Number Of Threads: " << this->NumberOfThreads << "\n";
}

//----------------------------------------------------------------------------
VTK_THREAD_RETURN_TYPE vtkGridSyncTempThreadedExecute( void *arg )
{
  vtkGridSynchronizedTemplates3D *self;
  int threadId, threadCount;
  int ext[6], *tmp;
  
  threadId = ((ThreadInfoStruct *)(arg))->ThreadID;
  threadCount = ((ThreadInfoStruct *)(arg))->NumberOfThreads;
  self = (vtkGridSynchronizedTemplates3D *)
            (((ThreadInfoStruct *)(arg))->UserData);


  // we need to breakup the ExecuteExtent based on the threadId/Count
  tmp = self->GetExecuteExtent();
  ext[0] = tmp[0];
  ext[1] = tmp[1];
  ext[2] = tmp[2];
  ext[3] = tmp[3];
  ext[4] = tmp[4];
  ext[5] = tmp[5];
  if (self->SplitExtent(threadId, threadCount, ext))
    {
    self->ThreadedExecute(ext, threadId);
    }
  
  return VTK_THREAD_RETURN_VALUE;
}

//----------------------------------------------------------------------------
void vtkGridSynchronizedTemplates3D::Execute()
{
  int idx, offset, num, ptIdx, newIdx, numCellPts, *cellPts, newCellPts[3];
  vtkPolyData *output = this->GetOutput();
  vtkPoints *outPts;
  vtkPointData *outPD;
  vtkCellArray *outTris;
  vtkScalars *outScalars;
  vtkVectors *outGrads;
  vtkNormals *outNormals;
  vtkPolyData *threadOut;
  vtkPointData *threadPD;
  vtkCellArray *threadTris;
  vtkScalars *threadScalars;
  vtkVectors *threadGrads;
  vtkNormals *threadNormals;
  vtkStructuredGrid *input = this->GetInput();
  int ext[6];
  int numPieces = output->GetUpdateNumberOfPieces();

  input->GetWholeExtent(ext);

  // hack here. Compute the typical input extent.
  // just used for estimated size;
  ext[1] = ((ext[1]-ext[0]+1) / numPieces)-1+ext[0];

  this->InitializeOutput(ext, output);

  outPts = output->GetPoints();
  outPD = output->GetPointData();
  outTris = output->GetPolys();
  outScalars = outPD->GetScalars();
  outGrads = outPD->GetVectors();
  outNormals = outPD->GetNormals();

  if (this->NumberOfThreads == 1)
    {
    // just call the threaded execute directly.
    this->ThreadedExecute(this->GetExecuteExtent(), 0);
    }
  else
    {
    this->Threader->SetNumberOfThreads(this->NumberOfThreads);

    // Setup threading and the invoke threadedExecute
    this->Threader->SetSingleMethod(vtkGridSyncTempThreadedExecute, this);
    this->Threader->SingleMethodExecute();
    
    // Collect all the data into the output.  Now I cannot use append filter
    // because this filter might be streaming.  (Maybe I could if thread
    // 0 wrote to output, and I copied output to a temp polyData...)
    for (idx = 1; idx < this->NumberOfThreads; ++idx)
      {
      threadOut = this->Threads[idx];
      if (threadOut != NULL)
	{
	offset = output->GetNumberOfPoints();
	threadPD = threadOut->GetPointData();
	threadScalars = threadPD->GetScalars();
	threadGrads = threadPD->GetVectors();
	threadNormals = threadPD->GetNormals();
	num = threadOut->GetNumberOfPoints();
	for (ptIdx = 0; ptIdx < num; ++ptIdx)
	  {
	  newIdx = ptIdx + offset;
	  outPts->InsertPoint(newIdx, threadOut->GetPoint(ptIdx));
	  if (outScalars)
	    {
	    outScalars->InsertScalar(newIdx, threadScalars->GetScalar(ptIdx));
	    }
	  if (outGrads)
	    {
	    outGrads->InsertVector(newIdx, threadGrads->GetVector(ptIdx));
	    }
	  if (outNormals)
	    {
	    outNormals->InsertNormal(newIdx, threadNormals->GetNormal(ptIdx));
	    }
	  }
	// copy the triangles.
	threadTris = threadOut->GetPolys();
	threadTris->InitTraversal();
	while (threadTris->GetNextCell(numCellPts, cellPts))
	  {
	  // copy and translate
	  if (numCellPts == 3)
	    {
	    newCellPts[0] = cellPts[0] + offset;
	    newCellPts[1] = cellPts[1] + offset;
	    newCellPts[2] = cellPts[2] + offset;
	    outTris->InsertNextCell(3, newCellPts); 
	    }
	  }
	threadOut->Delete();
	threadOut = this->Threads[idx] = NULL;         
	}
      }
    }

  output->Squeeze();
}


