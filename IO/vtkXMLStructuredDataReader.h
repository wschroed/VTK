/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLStructuredDataReader - Superclass for structured data XML readers.
// .SECTION Description
// vtkXMLStructuredDataReader provides functionality common to all
// structured data format readers.

// .SECTION See Also
// vtkXMLImageDataReader vtkXMLStructuredGridReader
// vtkXMLRectilinearGridReader

#ifndef __vtkXMLStructuredDataReader_h
#define __vtkXMLStructuredDataReader_h

#include "vtkXMLDataReader.h"


class VTK_IO_EXPORT vtkXMLStructuredDataReader : public vtkXMLDataReader
{
public:
  vtkTypeRevisionMacro(vtkXMLStructuredDataReader,vtkXMLDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);  
  
  // Description:
  // Get the number of points in the output.
  virtual vtkIdType GetNumberOfPoints();
  
  // Description:
  // Get the number of cells in the output.
  virtual vtkIdType GetNumberOfCells();
  
  // Description:
  // Get/Set whether the reader gets a whole slice from disk when only
  // a rectangle inside it is needed.  This mode reads more data than
  // necessary, but prevents many short reads from interacting poorly
  // with the compression and encoding schemes.
  vtkSetMacro(WholeSlices, int);
  vtkGetMacro(WholeSlices, int);
  vtkBooleanMacro(WholeSlices, int);
  
protected:
  vtkXMLStructuredDataReader();
  ~vtkXMLStructuredDataReader();
  
  virtual void SetOutputExtent(int* extent)=0;
  int ReadPrimaryElement(vtkXMLDataElement* ePrimary);
  
  // Pipeline execute data driver.  Called by vtkXMLReader.
  void ReadXMLData();
  
  // Internal representation of pieces in the file that may have come
  // from a streamed write.
  int* PieceExtents;
  int* PiecePointDimensions;
  int* PiecePointIncrements;
  int* PieceCellDimensions;
  int* PieceCellIncrements;
  
  // Whether to read in whole slices mode.
  int WholeSlices;
  
  // The update extent and corresponding increments and dimensions.
  int UpdateExtent[6];
  int PointDimensions[3];
  int CellDimensions[3];
  int PointIncrements[3];
  int CellIncrements[3];
  
  // The extent currently being read.
  int SubExtent[6];
  int SubPointDimensions[3];
  int SubCellDimensions[3];
  
  // Override methods from superclass.
  void SetupPieces(int numPieces);
  void DestroyPieces();
  int ReadArrayForPoints(vtkXMLDataElement* da, vtkDataArray* outArray);
  int ReadArrayForCells(vtkXMLDataElement* da, vtkDataArray* outArray);
  
  // Internal utility methods.
  int ReadPiece(vtkXMLDataElement* ePiece);
  int ReadSubExtent(int* inExtent, int* inDimensions, int* inIncrements,
                    int* outExtent, int* outDimensions, int* outIncrements,
                    int* subExtent, int* subDimensions, vtkXMLDataElement* da,
                    vtkDataArray* array);
  
private:
  vtkXMLStructuredDataReader(const vtkXMLStructuredDataReader&);  // Not implemented.
  void operator=(const vtkXMLStructuredDataReader&);  // Not implemented.
};

#endif
