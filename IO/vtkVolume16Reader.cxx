/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
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
#include "vtkVolume16Reader.h"
#include "vtkShortScalars.h"

// Description:
// Construct object with NULL file prefix; file pattern "%s.%d"; image range 
// set to (1,1); data origin (0,0,0); data aspect ratio (1,1,1); no data mask;
// header size 0; and byte swapping turned off.
vtkVolume16Reader::vtkVolume16Reader()
{
  this->DataMask = 0x0000;
  this->HeaderSize = 0;
  this->SwapBytes = 0;
  this->DataDimensions[0] = this->DataDimensions[1] = 0;
  this->Transform = NULL;
}

void vtkVolume16Reader::SetFileTypeBigEndian()
{
#ifndef WORDS_BIGENDIAN
  this->SwapBytesOn();
#else
  this->SwapBytesOff();
#endif
}

void vtkVolume16Reader::SetFileTypeLittleEndian()
{
#ifdef WORDS_BIGENDIAN
  this->SwapBytesOn();
#else
  this->SwapBytesOff();
#endif
}


void vtkVolume16Reader::Execute()
{
  vtkScalars *newScalars;
  int first, last;
  int numberSlices;
  int *dim;
  int dimensions[3];
  float aspectRatio[3];
  float origin[3];

  vtkStructuredPoints *output=(vtkStructuredPoints *)this->Output;

  // Validate instance variables
  if (this->FilePrefix == NULL) 
    {
    vtkErrorMacro(<< "FilePrefix is NULL");
    return;
    }

  if (this->HeaderSize < 0) 
    {
    vtkErrorMacro(<< "HeaderSize " << this->HeaderSize << " must be >= 0");
    return;
    }

  dim = this->DataDimensions;

  if (dim[0] <= 0 || dim[1] <= 0) 
    {
    vtkErrorMacro(<< "x, y dimensions " << dim[0] << ", " << dim[1] 
                  << "must be greater than 0.");
    return;
    } 

  if ( (this->ImageRange[1]-this->ImageRange[0]) <= 0 )
    {
    numberSlices = 1;
    newScalars = this->ReadImage(this->ImageRange[0]);
    }
  else
    {
    first = this->ImageRange[0];
    last = this->ImageRange[1];
    numberSlices = last - first + 1;
    newScalars = this->ReadVolume(first, last);
    }

  // calculate dimensions of output from data dimensions and transform
  ComputeTransformedDimensions (dimensions);
  output->SetDimensions(dimensions);

  // calculate aspect ratio of output from data aspect ratio and transform
  this->ComputeTransformedAspectRatio (aspectRatio);

  // calculate origin of output from data origin and transform
  this->ComputeTransformedOrigin (origin);

  // adjust aspect ratio and origin if aspect ratio is negative
  this->AdjustAspectRatioAndOrigin (dimensions, aspectRatio, origin);

  output->SetAspectRatio(aspectRatio);
  output->SetOrigin(origin);
  if ( newScalars ) 
    {
    output->GetPointData()->SetScalars(newScalars);
    newScalars->Delete();
    }
}

vtkStructuredPoints *vtkVolume16Reader::GetImage(int ImageNumber)
{
  vtkScalars *newScalars;
  int *dim;
  int dimensions[3];
  vtkStructuredPoints *result;

  // Validate instance variables
  if (this->FilePrefix == NULL) 
    {
    vtkErrorMacro(<< "FilePrefix is NULL");
    return NULL;
    }

  if (this->HeaderSize < 0) 
    {
    vtkErrorMacro(<< "HeaderSize " << this->HeaderSize << " must be >= 0");
    return NULL;
    }

  dim = this->DataDimensions;

  if (dim[0] <= 0 || dim[1] <= 0) 
    {
    vtkErrorMacro(<< "x, y dimensions " << dim[0] << ", " << dim[1] 
                  << "must be greater than 0.");
    return NULL;
    } 
  
  result = new vtkStructuredPoints();
  newScalars = this->ReadImage(ImageNumber);
  dimensions[0] = dim[0]; dimensions[1] = dim[1];
  dimensions[2] = 1;
  result->SetDimensions(dimensions);
  result->SetAspectRatio(this->DataAspectRatio);
  result->SetOrigin(this->DataOrigin);
  if ( newScalars ) 
    {
    result->GetPointData()->SetScalars(newScalars);
    newScalars->Delete();
    }
  return result;
}

// Description:
// Read a slice of volume data.
vtkScalars *vtkVolume16Reader::ReadImage(int sliceNumber)
{
  vtkShortScalars *scalars = NULL;
  short *pixels;
  FILE *fp;
  int numPts;
  int status;
  char filename[1024];

  // build the file name
  sprintf (filename, this->FilePattern, this->FilePrefix, sliceNumber);
  if ( !(fp = fopen(filename,"rb")) )
    {
    vtkErrorMacro(<<"Can't open file: " << filename);
    return NULL;
    }

  numPts = this->DataDimensions[0] * this->DataDimensions[1];

  // create the short scalars
  scalars = new vtkShortScalars(numPts);

  // get a pointer to the data
  pixels = scalars->WritePtr(0, numPts);

  // read the image data
  status = Read16BitImage (fp, pixels, this->DataDimensions[0], this->DataDimensions[1], this->HeaderSize, this->SwapBytes);

  // close the file
  fclose (fp);

  // check the status of the read
  if (status == 0) 
    {
    scalars->Delete();
    return NULL;
    }
  else 
    return scalars;
}

// Description:
// Read a volume of data.
vtkScalars *vtkVolume16Reader::ReadVolume(int first, int last)
{
  vtkShortScalars *scalars = NULL;
  short *pixels;
  short *slice;
  FILE *fp;
  int numPts;
  int fileNumber;
  int status=0;
  int numberSlices = last - first + 1;
  char filename[1024];
  int dimensions[3];
  int bounds[6];

  // calculate the number of points per image
  numPts = this->DataDimensions[0] * this->DataDimensions[1];

  // compute transformed dimensions
  this->ComputeTransformedDimensions (dimensions);

  // compute transformed bounds
  this->ComputeTransformedBounds (bounds);

  // get memory for slice
  slice = new short[numPts];

  // create the short scalars for all of the images
  scalars = new vtkShortScalars(numPts * numberSlices);

  // get a pointer to the scalar data
  pixels = scalars->WritePtr(0, numPts *numberSlices);

  vtkDebugMacro (<< "Creating scalars with " << numPts * numberSlices << " points.");

  // build each file name and read the data from the file
  for (fileNumber = first; fileNumber <= last; fileNumber++) 
    {
    // build the file name
    sprintf (filename, this->FilePattern, this->FilePrefix, fileNumber);
    if ( !(fp = fopen(filename,"rb")) )
      {
      vtkErrorMacro(<<"Can't find file: " << filename);
      return NULL;
      }

    vtkDebugMacro ( << "Reading " << filename );

    // read the image data
    status = Read16BitImage (fp, slice, this->DataDimensions[0], this->DataDimensions[1], this->HeaderSize, this->SwapBytes);

    fclose (fp);

    if (status == 0) break;

    // transform slice
    TransformSlice (slice, pixels, fileNumber - first, dimensions, bounds);
    }

  delete []slice;
  if (status == 0) 
    {
    scalars->Delete();
    return NULL;
    }
  else return scalars;
}

int vtkVolume16Reader:: Read16BitImage (FILE *fp, short *pixels, int xsize, 
                                        int ysize, int skip, int swapBytes)
{
  int numShorts = xsize * ysize;
  int status;

  if (skip) fseek (fp, skip, 0);

  status = fread (pixels, sizeof (short), numShorts, fp);

  if (status && swapBytes) 
    {
    unsigned char *bytes = (unsigned char *) pixels;
    unsigned char tmp;
    int i;
    for (i = 0; i < numShorts; i++, bytes += 2) 
      {
      tmp = *bytes; 
      *bytes = *(bytes + 1); 
      *(bytes + 1) = tmp;
      }
    }

  if (status && this->DataMask != 0x0000 )
    {
    short *dataPtr = pixels;
    int i;
    for (i = 0; i < numShorts; i++, dataPtr++) 
      {
      *dataPtr &= this->DataMask;
      }
    }

  return status;
}

void vtkVolume16Reader::ComputeTransformedAspectRatio (float aspectRatio[3])
{
  if (!this->Transform)
    {
    memcpy (aspectRatio, this->DataAspectRatio, 3 * sizeof (float));
    }
  else
    {
    float transformedAspectRatio[4];
    memcpy (transformedAspectRatio, this->DataAspectRatio, 3 * sizeof (float));
    transformedAspectRatio[3] = 1.0;
    this->Transform->MultiplyPoint (transformedAspectRatio, transformedAspectRatio);

    for (int i = 0; i < 3; i++) aspectRatio[i] = transformedAspectRatio[i];
    vtkDebugMacro("Transformed aspectRatio " << aspectRatio[0] << ", " << aspectRatio[1] << ", " << aspectRatio[2]);
    }
}

void vtkVolume16Reader::ComputeTransformedOrigin (float origin[3])
{
  if (!this->Transform)
    {
    memcpy (origin, this->DataOrigin, 3 * sizeof (float));
    }
  else
    {
    float transformedOrigin[4];
    memcpy (transformedOrigin, this->DataOrigin, 3 * sizeof (float));
    transformedOrigin[3] = 1.0;
    this->Transform->MultiplyPoint (transformedOrigin, transformedOrigin);

    for (int i = 0; i < 3; i++) origin[i] = transformedOrigin[i];
    vtkDebugMacro("Transformed Origin " << origin[0] << ", " << origin[1] << ", " << origin[2]);
    }
}

void vtkVolume16Reader::ComputeTransformedDimensions (int dimensions[3])
{
  float transformedDimensions[4];
  if (!this->Transform)
    {
    dimensions[0] = this->DataDimensions[0];
    dimensions[1] = this->DataDimensions[1];
    dimensions[2] = this->ImageRange[1] - this->ImageRange[0] + 1;
    }
  else
    {
    transformedDimensions[0] = this->DataDimensions[0];
    transformedDimensions[1] = this->DataDimensions[1];
    transformedDimensions[2] = this->ImageRange[1] - this->ImageRange[0] + 1;
    transformedDimensions[3] = 1.0;
    this->Transform->MultiplyPoint (transformedDimensions, transformedDimensions);
    dimensions[0] = (int) transformedDimensions[0];
    dimensions[1] = (int) transformedDimensions[1];
    dimensions[2] = (int) transformedDimensions[2];
    if (dimensions[0] < 0) dimensions[0] = -dimensions[0];
    if (dimensions[1] < 0) dimensions[1] = -dimensions[1];
    if (dimensions[2] < 0) dimensions[2] = -dimensions[2];
    vtkDebugMacro(<< "Transformed dimensions are:" << dimensions[0] << ", "
					     << dimensions[1] << ", "
					     << dimensions[2]);
    }
}

void vtkVolume16Reader::ComputeTransformedBounds (int bounds[6])
{
  float transformedBounds[4];

  if (!this->Transform)
    {
    bounds[0] = 0;
    bounds[1] = this->DataDimensions[0] - 1;
    bounds[2] = 0;
    bounds[3] = this->DataDimensions[1] - 1;
    bounds[4] = 0;
    bounds[5] = this->ImageRange[1] - this->ImageRange[0];
    }
  else
    {
    transformedBounds[0] = 0;
    transformedBounds[1] = 0;
    transformedBounds[2] = 0;
    transformedBounds[3] = 1.0;
    this->Transform->MultiplyPoint (transformedBounds, transformedBounds);
    bounds[0] = (int) transformedBounds[0];
    bounds[2] = (int) transformedBounds[1];
    bounds[4] = (int) transformedBounds[2];
    transformedBounds[0] = this->DataDimensions[0] - 1;
    transformedBounds[1] = this->DataDimensions[1] - 1;
    transformedBounds[2] = this->ImageRange[1] - this->ImageRange[0];
    transformedBounds[3] = 1.0;
    this->Transform->MultiplyPoint (transformedBounds, transformedBounds);
    bounds[1] = (int) transformedBounds[0];
    bounds[3] = (int) transformedBounds[1];
    bounds[5] = (int) transformedBounds[2];
    // put bounds in correct order
    int tmp;
    for (int i = 0; i < 6; i += 2)
      {
      if (bounds[i + 1] < bounds[i])
        {
        tmp = bounds[i];
        bounds[i] = bounds[i + 1];
        bounds[i + 1] = tmp;
        }
      }
    vtkDebugMacro(<< "Transformed bounds are: "
		<< bounds[0] << ", " << bounds[1] << ", "
                << bounds[2] << ", " << bounds[3] << ", "
		<< bounds[4] << ", " << bounds[5]);
    }
}

void vtkVolume16Reader::AdjustAspectRatioAndOrigin (int dimensions[3], float aspectRatio[3], float origin[3])
{
  for (int i = 0; i < 3; i++)
    {
    if (aspectRatio[i] < 0)
      {
      origin[i] = origin[i] + aspectRatio[i] * (dimensions[i] - 1);
      aspectRatio[i] = -aspectRatio[i];
      }
    }
  vtkDebugMacro("Adjusted aspectRatio " << aspectRatio[0] << ", " << aspectRatio[1] << ", " << aspectRatio[2]);
  vtkDebugMacro("Adjusted origin " << origin[0] << ", " << origin[1] << ", " << origin[2]);
}

void vtkVolume16Reader::TransformSlice (short *slice, short *pixels, int k, int dimensions[3], int bounds[3])
{
  int iSize = this->DataDimensions[0];
  int jSize = this->DataDimensions[1];

  if (!this->Transform)
    {
    memcpy (pixels + iSize * jSize * k, slice, iSize * jSize * sizeof (short));
    }
  else
    {
    float transformedIjk[4], ijk[4];
    int i, j;
    int xyz[3];
    int index;
    int xSize = dimensions[0];
    int xySize = dimensions[0] * dimensions[1];

    // now move slice into pixels

    ijk[2] = k;
    ijk[3] = 1.0;
    for (j = 0; j < jSize; j++)
      {
      ijk[1] = j;
      for (i = 0; i < iSize; i++, slice++)
        {
        ijk[0] = i;
        this->Transform->MultiplyPoint (ijk, transformedIjk);
	xyz[0] = transformedIjk[0] - bounds[0];
	xyz[1] = transformedIjk[1] - bounds[2];
	xyz[2] = transformedIjk[2] - bounds[4];
        index = xyz[0] +
                xyz[1] * xSize +
                xyz[2] * xySize;
	*(pixels + index) = *slice;
        }
      }
    }
}

void vtkVolume16Reader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkVolumeReader::PrintSelf(os,indent);

  os << indent << "HeaderSize: " << this->HeaderSize << "\n";
  os << indent << "SwapBytes: " << this->SwapBytes << "\n";
  os << indent << "Data Dimensions: (" << this->DataDimensions[0] << ", "
                                   << this->DataDimensions[1] << ")\n";
  if ( this->Transform )
    {
    os << indent << "Transform:\n";
    this->Transform->PrintSelf(os,indent.GetNextIndent());
    }
  else
    os << indent << "Transform: (None)\n";
}
