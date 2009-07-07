/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkDenseArray - Contiguous storage for N-way arrays.
//
// .SECTION Description
// vtkDenseArray is a concrete vtkArray implementation that stores values
// using a contiguous block of memory.  Values are stored with fortran ordering,
// meaning that if you iterated over the memory block, the left-most coordinates
// would vary the fastest.
//
// In addition to the retrieval and update methods provided by vtkTypedArray,
// vtkDenseArray provides methods to:
//
// Fill the entire array with a specific value.
//
// Retrieve a pointer to the storage memory block.
//
// .SECTION See Also
// vtkArray, vtkTypedArray, vtkSparseArray
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkDenseArray_h
#define __vtkDenseArray_h

#include "vtkArrayCoordinates.h"
#include "vtkObjectFactory.h"
#include "vtkTypedArray.h"
#include "vtkTypeTemplate.h"

template<typename T>
class vtkDenseArray :
  public vtkTypeTemplate<vtkDenseArray<T>, vtkTypedArray<T> >
{
public:
  using vtkTypedArray<T>::GetValue;
  using vtkTypedArray<T>::SetValue;

  static vtkDenseArray<T>* New();
  void PrintSelf(ostream &os, vtkIndent indent);
  
  // vtkArray API
  bool IsDense();
  vtkArrayExtents GetExtents();
  vtkIdType GetNonNullSize();
  void GetCoordinatesN(const vtkIdType n, vtkArrayCoordinates& coordinates);
  vtkArray* DeepCopy();

  // vtkTypedArray API
  const T& GetValue(const vtkArrayCoordinates& coordinates);
  const T& GetValueN(const vtkIdType n);
  void SetValue(const vtkArrayCoordinates& coordinates, const T& value);
  void SetValueN(const vtkIdType n, const T& value);

  // vtkDenseArray API

  // Description:
  // Strategy object that contains a block of memory to be used by vtkDenseArray
  // for value storage.  The MemoryBlock object is responsible for freeing
  // memory when destroyed.
  class MemoryBlock
  {
  public:
    virtual ~MemoryBlock();
    // Description:
    // Returns a pointer to the block of memory to be used for storage.
    virtual T* GetAddress() = 0;
  };

  // Description:
  // MemoryBlock implementation that manages internally-allocated memory using
  // new[] and delete[].  Note: HeapMemoryBlock is the default used by vtkDenseArray
  // for its "normal" internal memory allocation.
  class HeapMemoryBlock :
    public MemoryBlock
  {
  public:
    HeapMemoryBlock(const vtkArrayExtents& extents);
    virtual ~HeapMemoryBlock();
    virtual T* GetAddress();

  private:
    T* Storage;
  };

  // Description:
  // MemoryBlock implementation that manages a static (will not be freed) memory block.
  class StaticMemoryBlock :
    public MemoryBlock
  {
  public:
    StaticMemoryBlock(T* const storage);
    virtual T* GetAddress();

  private:
    T* Storage;
  };

  // Description:
  // Initializes the array to use an externally-allocated memory block.  The supplied
  // MemoryBlock must be large enough to store extents.GetSize() values.  The contents of
  // the memory must be stored contiguously with fortran ordering, 
  //
  // Dimension-labels are undefined after calling ExternalStorage() - you should
  // initialize them accordingly.
  //
  // The array will use the supplied memory for storage until the array goes out of
  // scope, is configured to use a different memory block by calling ExternalStorage()
  // again, or is configured to use internally-allocated memory by calling Resize().
  //
  // Note that the array will delete the supplied memory block when it is no longer in use.
  // caller's responsibility to ensure that the memory does not go out-of-scope until
  // the array has been destroyed or is no longer using it.
  void ExternalStorage(const vtkArrayExtents& extents, MemoryBlock* storage);

  // Description:
  // Fills every element in the array with the given value.
  void Fill(const T& value);

  // Description:
  // Returns a value by-reference, which is useful for performance and code-clarity.
  T& operator[](const vtkArrayCoordinates& coordinates);

  // Description:
  // Returns a read-only reference to the underlying storage.  Values are stored
  // contiguously with fortran ordering.
  const T* GetStorage() const;
  
  // Description:
  // Returns a mutable reference to the underlying storage.  Values are stored
  // contiguously with fortran ordering.  Use at your own risk!
  T* GetStorage();

protected:
  vtkDenseArray();
  ~vtkDenseArray();

private:
  vtkDenseArray(const vtkDenseArray&); // Not implemented
  void operator=(const vtkDenseArray&); // Not implemented

  void InternalResize(const vtkArrayExtents& extents);
  void InternalSetDimensionLabel(vtkIdType i, const vtkStdString& label);
  vtkStdString InternalGetDimensionLabel(vtkIdType i);
  vtkIdType MapCoordinates(const vtkArrayCoordinates& coordinates);

  void Reconfigure(const vtkArrayExtents& extents, MemoryBlock* storage);

  typedef vtkDenseArray<T> ThisT;

  // Description:
  // Stores the current array extents (its size along each dimension)
  vtkArrayExtents Extents;
  
  // Description:
  // Stores labels for each array dimension
  vtkstd::vector<vtkStdString> DimensionLabels;

  // Description:
  // Manages array value memory storage.
  MemoryBlock* Storage;
  
  // Description:
  // Stores array values using a contiguous range of memory
  // with constant-time value lookup.
  T* Begin;
  T* End;

  // Description:
  // Stores the strides along each array dimension (used for fast lookups).
  vtkstd::vector<vtkIdType> Strides;
};

#include "vtkDenseArray.txx"

#endif

