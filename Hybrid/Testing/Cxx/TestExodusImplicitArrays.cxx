#include "vtkAbstractArray.h"
#include "vtkExodusIIReader.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdTypeArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPExodusIIReader.h"
#include "vtkSmartPointer.h"

#include <vtkTestUtilities.h>
#include <vtkTesting.h>

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New ();
  


int TestExodusImplicitArrays(int argc, char *argv[])
{
  const char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/disk_out_ref.ex2");
  VTK_CREATE (vtkExodusIIReader, reader);
  reader->SetFileName (fname);
  reader->GenerateImplicitElementIdArrayOn ();
  reader->GenerateImplicitNodeIdArrayOn ();
  reader->Update ();

  vtkMultiBlockDataSet *mb = reader->GetOutput ();

  if (!mb)
    {
    return vtkTesting::FAILED;
    }
  vtkMultiBlockDataSet *elems = vtkMultiBlockDataSet::SafeDownCast (mb->GetBlock (0));

  if (!elems) 
    {
    return vtkTesting::FAILED;
    }
  vtkDataObject *obj = elems->GetBlock (0);

  if (!obj)
    {
    return vtkTesting::FAILED;
    }
  vtkIdTypeArray *ie = vtkIdTypeArray::SafeDownCast (
                  obj->GetAttributes (vtkDataSet::CELL)->GetAbstractArray ("ImplicitElementId"));
  vtkIdTypeArray *in = vtkIdTypeArray::SafeDownCast (
                  obj->GetAttributes (vtkDataSet::POINT)->GetAbstractArray ("ImplicitNodeId"));
  if (!ie || !in)
    {
    return vtkTesting::FAILED;
    }

  for (int id = 0; id < ie->GetNumberOfTuples (); id ++)
    {
    if (ie->GetValue (id) != (id + 1))
      {
      return vtkTesting::FAILED;
      }
    }

  if (in->GetValue (0) != 143 || in->GetValue (1) != 706 || in->GetValue (2) != 3173)
    {
    return vtkTesting::FAILED;
    }

  return vtkTesting::PASSED;
}