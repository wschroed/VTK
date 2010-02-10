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
#include "vtkDeformPointSet.h"

#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkMeanValueCoordinatesInterpolator.h"


vtkCxxRevisionMacro(vtkDeformPointSet, "$Revision$");
vtkStandardNewMacro(vtkDeformPointSet);

//----------------------------------------------------------------------------
vtkDeformPointSet::vtkDeformPointSet()
{
  this->InitializeWeights = 0;
  this->SetNumberOfInputPorts(2);
  
  // Prepare cached data
  this->InitialNumberOfControlMeshPoints = 0;
  this->InitialNumberOfControlMeshCells = 0;
  this->InitialNumberOfPointSetPoints = 0;
  this->InitialNumberOfPointSetCells = 0;
  this->Weights = vtkDoubleArray::New();
}

//----------------------------------------------------------------------------
vtkDeformPointSet::~vtkDeformPointSet()
{
  if ( this->Weights )
    {
    this->Weights->Delete();
    this->Weights = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkDeformPointSet::SetControlMeshConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}
 
//----------------------------------------------------------------------------
void vtkDeformPointSet::SetControlMesh(vtkPolyData *input)
{
  this->SetInput(1, input);
}

//----------------------------------------------------------------------------
vtkPolyData *vtkDeformPointSet::GetControlMesh()
{
  if (this->GetNumberOfInputConnections(1) < 1)
    {
    return NULL;
    }
  
  return vtkPolyData::SafeDownCast(
    this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
int vtkDeformPointSet::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *cmeshInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet *input = vtkPointSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *cmesh = vtkPolyData::SafeDownCast(
    cmeshInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPointSet *output = vtkPointSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!cmesh)
    {
    return 0;
    }

  // Pass the input attributes to the ouput
  output->CopyStructure( input );
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  // Gather initial information
  vtkIdType numberOfPointSetPoints = input->GetNumberOfPoints();
  vtkIdType numberOfPointSetCells = input->GetNumberOfCells();
  vtkPoints *inPts = input->GetPoints();
  vtkPoints *cmeshPts = cmesh->GetPoints();
  if ( !inPts || !cmeshPts )
    {
    return 0;
    }
  vtkCellArray *cmeshPolys = cmesh->GetPolys();
  vtkIdType numberOfControlMeshPoints = cmeshPts->GetNumberOfPoints();
  vtkIdType numberOfControlMeshCells = cmeshPolys->GetNumberOfCells();
  vtkIdType numTriangles = cmeshPolys->GetNumberOfConnectivityEntries() / 4;
  if ( numTriangles != numberOfControlMeshCells )
    {
    vtkErrorMacro("Control mesh must be a closed, manifold triangular mesh");
    return 0;
    }

  // We will be modifying the points
  vtkPoints *outPts = input->GetPoints()->NewInstance();
  outPts->SetDataType(input->GetPoints()->GetDataType());
  outPts->SetNumberOfPoints(numberOfPointSetPoints);
  output->SetPoints(outPts);

  // Start by determing whether weights must be computed or not
  int abort=0;
  vtkIdType progressInterval=(numberOfPointSetPoints/10 + 1);
  int workLoad=1;
  double x[3], *weights;
  vtkIdType ptId, pid;
  if ( this->InitializeWeights || 
       this->InitialNumberOfControlMeshPoints != numberOfControlMeshPoints ||
       this->InitialNumberOfControlMeshCells != numberOfControlMeshCells ||
       this->InitialNumberOfPointSetPoints != numberOfPointSetPoints ||
       this->InitialNumberOfPointSetCells != numberOfPointSetCells )
    {
    workLoad = 2;
    // reallocate the weights
    this->Weights->Reset();
    this->Weights->SetNumberOfComponents(numberOfControlMeshPoints);
    this->Weights->SetNumberOfTuples(numberOfPointSetPoints);

    // compute the interpolation weights
    for (ptId=0; ptId < numberOfPointSetPoints && !abort; ++ptId)
      {
      if ( ! (ptId % progressInterval) ) 
        {
        vtkDebugMacro(<<"Processing #" << ptId);
        this->UpdateProgress (ptId/(workLoad*numberOfPointSetPoints));
        abort = this->GetAbortExecute();
        }

      inPts->GetPoint(ptId, x);
      weights = this->Weights->GetTuple(ptId);
      vtkMeanValueCoordinatesInterpolator::
        ComputeInterpolationWeights(x,cmeshPts,cmeshPolys,weights);
      }
    
    // prepare for next execution
    this->InitializeWeights = 0;
    this->InitialNumberOfControlMeshPoints = numberOfControlMeshPoints;
    this->InitialNumberOfControlMeshCells = numberOfControlMeshCells;
    this->InitialNumberOfPointSetPoints = numberOfPointSetPoints;
    this->InitialNumberOfPointSetCells = numberOfPointSetCells;
    }
  
  // Okay weights are computed, now interpolate
  double xx[3];
  for (ptId=0; ptId < numberOfPointSetPoints && !abort; ++ptId)
    {
    if ( ! (ptId % progressInterval) ) 
      {
      vtkDebugMacro(<<"Processing #" << ptId);
      this->UpdateProgress (ptId/(workLoad*numberOfPointSetPoints));
      abort = this->GetAbortExecute();
      }

    weights = this->Weights->GetTuple(ptId);
    
    x[0] = x[1] = x[2] = 0.0;
    for ( pid=0; pid < numberOfControlMeshPoints; ++pid )
      {
      cmeshPts->GetPoint(pid,xx);
      x[0] += weights[pid] * xx[0];
      x[1] += weights[pid] * xx[1];
      x[2] += weights[pid] * xx[2];
      }
    outPts->SetPoint(ptId,x);
    }
       
  // clean up and get out
  outPts->Delete();
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkDeformPointSet::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *cmeshInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  outInfo->CopyEntry(cmeshInfo, 
                     vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->CopyEntry(cmeshInfo, 
                     vtkStreamingDemandDrivenPipeline::TIME_RANGE());

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
               6);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
               inInfo->Get(
                 vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES()));

  return 1;
}

//----------------------------------------------------------------------------
int vtkDeformPointSet::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *cmeshInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int usePiece = 0;

  // What ever happend to CopyUpdateExtent in vtkDataObject?
  // Copying both piece and extent could be bad.  Setting the piece
  // of a structured data set will affect the extent.
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (output &&
      (!strcmp(output->GetClassName(), "vtkUnstructuredGrid") ||
       !strcmp(output->GetClassName(), "vtkPolyData")))
    {
    usePiece = 1;
    }
  
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
  
  if (usePiece)
    {
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
    }
  else
    {
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()), 6);
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkDeformPointSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  vtkDataObject *cmesh = this->GetControlMesh();
  os << indent << "Control Mesh: " << cmesh << "\n";

  os << indent << "Initialize Weights: " 
     << (this->InitializeWeights ? "true" : "false") << "\n";

}
