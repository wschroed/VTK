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
// .NAME vtkMCubesReader - read binary marching cubes file
// .SECTION Description
// vtkMCubesReader is a source object that reads binary marching cubes
// files. (Marching cubes is an isosurfacing technique that generates 
// many triangles.) The binary format is supported by W. Lorensen's
// marching cubes program (and the vtkSliceCubes object). The format 
// repeats point coordinates, so this object will merge the points 
// with a vtkLocator object. You can choose to supply the vtkLocator 
// or use the default.

// .SECTION Caveats
// Binary files assumed written in sun/hp/sgi (i.e., Big Endian) form.
//
// Because points are merged when read, degenerate triangles may be removed.
// Thus the number of triangles read may be fewer than the number of triangles
// actually created.
//
// The point merging does not take into account that the same point may have
// different normals. For example, running vtkPolyDataNormals after
// vtkContourFilter may split triangles because of the FeatureAngle
// ivar. Subsequent reading with vtkMCubesReader will merge the points and
// use the first point's normal. For the most part, this is undesirable.
//
// Normals are generated from the gradient of the data scalar values. Hence
// the normals may on occasion point in a direction inconsistent with the
// ordering of the triangle vertices. If this happens, the resulting surface
// may be "black".  Reverse the sense of the FlipNormals boolean flag to
// correct this.

// .SECTION See Also
// vtkContourFilter vtkMarchingCubes vtkSliceCubes vtkLocator

#ifndef __vtkMCubesReader_h
#define __vtkMCubesReader_h

#include "vtkPolyDataSource.h"

#define VTK_FILE_BYTE_ORDER_BIG_ENDIAN 0
#define VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN 1

class vtkPointLocator;

class VTK_IO_EXPORT vtkMCubesReader : public vtkPolyDataSource 
{
public:
  vtkTypeRevisionMacro(vtkMCubesReader,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with FlipNormals turned off and Normals set to true.
  static vtkMCubesReader *New();

  // Description:
  // Specify file name of marching cubes file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Set / get the file name of the marching cubes limits file.
  vtkSetStringMacro(LimitsFileName);
  vtkGetStringMacro(LimitsFileName);

  // Description:
  // Specify a header size if one exists. The header is skipped and not used at this time.
  vtkSetClampMacro(HeaderSize,int,0,VTK_LARGE_INTEGER);
  vtkGetMacro(HeaderSize,int);

  // Description:
  // Specify whether to flip normals in opposite direction. Flipping ONLY
  // changes the direction of the normal vector. Contrast this with flipping
  // in vtkPolyDataNormals which flips both the normal and the cell point
  // order.
  vtkSetMacro(FlipNormals,int);
  vtkGetMacro(FlipNormals,int);
  vtkBooleanMacro(FlipNormals,int);

  // Description:
  // Specify whether to read normals.
  vtkSetMacro(Normals,int);
  vtkGetMacro(Normals,int);
  vtkBooleanMacro(Normals,int);

  // Description:
  // These methods should be used instead of the SwapBytes methods.
  // They indicate the byte ordering of the file you are trying
  // to read in. These methods will then either swap or not swap
  // the bytes depending on the byte ordering of the machine it is
  // being run on. For example, reading in a BigEndian file on a
  // BigEndian machine will result in no swapping. Trying to read
  // the same file on a LittleEndian machine will result in swapping.
  // As a quick note most UNIX machines are BigEndian while PC's
  // and VAX tend to be LittleEndian. So if the file you are reading
  // in was generated on a VAX or PC, SetDataByteOrderToLittleEndian otherwise
  // SetDataByteOrderToBigEndian. 
  void SetDataByteOrderToBigEndian();
  void SetDataByteOrderToLittleEndian();
  int GetDataByteOrder();
  void SetDataByteOrder(int);
  const char *GetDataByteOrderAsString();

  // Description:
  // Turn on/off byte swapping.
  vtkSetMacro(SwapBytes,int);
  vtkGetMacro(SwapBytes,int);
  vtkBooleanMacro(SwapBytes,int);

  // Description:
  // Set / get a spatial locator for merging points. By default, 
  // an instance of vtkMergePoints is used.
  void SetLocator(vtkPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkPointLocator);

  // Description:
  // Create default locator. Used to create one when none is specified.
  void CreateDefaultLocator();
  
  // Description:
  // Return the mtime also considering the locator.
  unsigned long GetMTime();

protected:
  vtkMCubesReader();
  ~vtkMCubesReader();

  void Execute();

  char *FileName;
  char *LimitsFileName;
  vtkPointLocator *Locator;
  int SwapBytes;
  int HeaderSize;
  int FlipNormals;
  int Normals;

private:
  vtkMCubesReader(const vtkMCubesReader&);  // Not implemented.
  void operator=(const vtkMCubesReader&);  // Not implemented.
};

#endif


