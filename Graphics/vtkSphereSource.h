/*=========================================================================

  Program:   Visualization Library
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlSphereSource - create a sphere centered at the origin
// .SECTION Description
// vlSphereSource creates a polygonal sphere of specified radius centered 
// at the origin. The resolution (polygonal discretization) in both the
// latitude (phi) and longitude (theta) diections can be specified. It is
// also possible to create partial spheres by specifying maximum phi
// and theta angles.

#ifndef __vlSphereSource_h
#define __vlSphereSource_h

#include "PolySrc.hh"

#define MAX_RESOLUTION 512

class vlSphereSource : public vlPolySource 
{
public:
  vlSphereSource(int res=8);
  char *GetClassName() {return "vlSphereSource";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetClampMacro(Radius,float,0.0,LARGE_FLOAT);
  vlGetMacro(Radius,float);

  vlSetClampMacro(ThetaResolution,int,4,MAX_RESOLUTION);
  vlGetMacro(ThetaResolution,int);

  vlSetClampMacro(PhiResolution,int,4,MAX_RESOLUTION);
  vlGetMacro(PhiResolution,int);

  vlSetClampMacro(Theta,float,0.0,360.0);
  vlGetMacro(Theta,float);

  vlSetClampMacro(Phi,float,0.0,180.0);
  vlGetMacro(Phi,float);

protected:
  void Execute();
  float Radius;
  float Theta;
  float Phi;
  int ThetaResolution;
  int PhiResolution;

};

#endif


