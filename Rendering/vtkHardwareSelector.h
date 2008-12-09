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
// .NAME vtkHardwareSelector - manager for OpenGL-based selection.
// .SECTION Description
// vtkHardwareSelector is a helper that orchestrates color buffer based
// selection. This relies on OpenGL. 
// vtkHardwareSelector can be used to select visible cells or points within a
// given rectangle of the RenderWindow.
// To use it, call in order:
// \li SetRenderer() - to select the renderer in which we
// want to select the cells/points.
// \li SetArea() - to set the rectangular region in the render window to select
// in.
// \li SetFieldAssociation() -  to select the attribute to select i.e.
// cells/points etc. 
// \li Finally, call Select().
// Select will cause the attached vtkRenderer to render in a special color mode,
// where each cell/point is given it own color so that later inspection of the 
// Rendered Pixels can determine what cells are visible. Select() returns a new
// vtkSelection instance with the cells/points selected.
//
// Limitations:
// Antialiasing will break this class. If your graphics card settings force
// their use this class will return invalid results.
//
// Currently only cells from PolyDataMappers can be selected from. When 
// vtkRenderer::Selector is non-null vtkPainterPolyDataMapper uses the
// vtkHardwareSelectionPolyDataPainter which make appropriate calls to
// BeginRenderProp(), EndRenderProp(), RenderAttributeId() to render colors
// correctly. Until alternatives to vtkHardwareSelectionPolyDataPainter
// exist that can do a similar coloration of other vtkDataSet types, only
// polygonal data can be selected. If you need to select other data types,
// consider using vtkDataSetMapper and turning on it's PassThroughCellIds 
// feature, or using vtkFrustumExtractor.
//
// Only Opaque geometry in Actors is selected from. Assemblies and LODMappers 
// are not currently supported. 
//
// During selection, visible datasets that can not be selected from are
// temporarily hidden so as not to produce invalid indices from their colors.
//
// .SECTION See Also
// vtkIdentColoredPainter

#ifndef __vtkHardwareSelector_h
#define __vtkHardwareSelector_h

#include "vtkObject.h"

class vtkRenderer;
class vtkSelection;
class vtkProp;
class vtkTextureObject;

class VTK_RENDERING_EXPORT vtkHardwareSelector : public vtkObject
{
public:
  static vtkHardwareSelector* New();
  vtkTypeRevisionMacro(vtkHardwareSelector, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the renderer to perform the selection on.
  void SetRenderer(vtkRenderer*);
  vtkGetObjectMacro(Renderer, vtkRenderer);

  // Description:
  // Get/Set the area to select as (xmin, ymin, xmax, ymax).
  vtkSetVector4Macro(Area, unsigned int);
  vtkGetVector4Macro(Area, unsigned int);

  // Description:
  // Set the field type to select. Valid values are 
  // \li vtkDataObject::FIELD_ASSOCIATION_POINTS
  // \li vtkDataObject::FIELD_ASSOCIATION_CELLS
  // \li vtkDataObject::FIELD_ASSOCIATION_VERTICES
  // \li vtkDataObject::FIELD_ASSOCIATION_EDGES
  // \li vtkDataObject::FIELD_ASSOCIATION_ROWS
  // Currently only FIELD_ASSOCIATION_POINTS and FIELD_ASSOCIATION_CELLS are
  // supported.
  vtkSetMacro(FieldAssociation, int);
  vtkGetMacro(FieldAssociation, int);

  // Description:
  // Perform the selection. Returns  a new instance of vtkSelection containing
  // the selection on success.
  vtkSelection* Select();

  // Description:
  // It is possible to use the vtkHardwareSelector for a custom picking. (Look
  // at vtkScenePicker). In that case instead of Select() on can use
  // CaptureBuffers() to render the selection buffers and then get information
  // about pixel locations suing GetPixelInformation(). Use ClearBuffers() to
  // clear buffers after one's done with the scene.
  bool CaptureBuffers();
  bool GetPixelInformation(unsigned int display_position[2],
    int& processId,
    vtkIdType& attrId, vtkProp*& prop);
  void ClearBuffers()
    { this->ReleasePixBuffers(); }

  // Description:
  // Called by any vtkMapper or vtkProp subclass to render an attribute's id.
  void RenderAttributeId(vtkIdType attribid);

  // Description:
  // Called by vtkRenderer to render the selection pass.
  // Returns the number of props rendered.
  int Render(vtkRenderer* renderer, vtkProp** propArray, int propArrayCount);

  // Description:
  // Called by the mapper (vtkHardwareSelectionPolyDataPainter) before and after
  // rendering each prop.
  void BeginRenderProp();
  void EndRenderProp();

  // Description:
  // Get/Set the process id. If process id < 0 (default -1), then the
  // PROCESS_PASS is not rendered.
  vtkSetMacro(ProcessID, int);
  vtkGetMacro(ProcessID, int);

  // Description:
  // Get the current pass number.
  vtkGetMacro(CurrentPass, int);

//BTX
  enum PassTypes
    {
    PROCESS_PASS,
    ACTOR_PASS,
    ID_LOW24,
    ID_MID24,
    ID_HIGH16,
    MAX_KNOWN_PASS = ID_HIGH16,
    MIN_KNOWN_PASS = PROCESS_PASS
    };
protected:
  vtkHardwareSelector();
  ~vtkHardwareSelector();

  static void Convert(int id, float tcoord[3])
    {
      tcoord[0] = static_cast<float>((id & 0xff)/255.0);
      tcoord[1] = static_cast<float>(((id & 0xff00) >> 8)/255.0);
      tcoord[2] = static_cast<float>(((id & 0xff0000) >> 16)/255.0);
    }

  int Convert(unsigned long offset, unsigned char* pb)
    {
    if (!pb)
      {
      return 0;
      }

    offset = offset * 3;
    unsigned char rgb[3];
    rgb[0] = pb[offset];
    rgb[1] = pb[offset+1];
    rgb[2] = pb[offset+2];
    int val = 0;
    val |= rgb[2];
    val = val << 8;
    val |= rgb[1];
    val = val << 8;
    val |= rgb[0];
    return val;
    }

  int Convert(int xx, int yy, unsigned char* pb)
    {
    if (!pb)
      {
      return 0;
      }
    int offset = (yy * (this->Area[2]-this->Area[0]) + xx) * 3;
    unsigned char rgb[3];
    rgb[0] = pb[offset];
    rgb[1] = pb[offset+1];
    rgb[2] = pb[offset+2];
    int val = 0;
    val |= rgb[2];
    val = val << 8;
    val |= rgb[1];
    val = val << 8;
    val |= rgb[0];
    return val;
    }

  vtkIdType GetID(int low24, int mid24, int high16)
    {
    vtkIdType val = 0;
    val |= high16;
    val = val << 24;
    val |= mid24;
    val = val << 24;
    val |= low24;
    return val;
    }

  // Description:
  // Returns is the pass indicated is needed.
  virtual bool PassRequired(int pass);

  // Description:
  // After the ACTOR_PASS this return true or false depending upon whether the
  // prop was hit in the ACTOR_PASS. This makes it possible to skip props that
  // are not involved in the selection after the first pass.
  bool IsPropHit(int propid);

  // Description:
  // Internal method that generates the vtkSelection from pixel buffers. 
  virtual vtkSelection* GenerateSelection();

  // Description:
  // Return a unique ID for the prop.
  virtual int GetPropID(int idx, vtkProp* vtkNotUsed(prop))
    { return idx; }

  virtual void BeginSelection();
  virtual void EndSelection();

  void SavePixelBuffer(int passNo);
  void BuildPropHitList(unsigned char* rgbData);

  // Description:
  // Clears all pixel buffers.
  void ReleasePixBuffers();
  vtkRenderer* Renderer;
  unsigned int Area[4];
  int FieldAssociation;
  vtkIdType MaxAttributeId;

  // At most 10 passes.
  unsigned char* PixBuffer[10];
  int ProcessID;
  int CurrentPass;
private:
  vtkHardwareSelector(const vtkHardwareSelector&); // Not implemented.
  void operator=(const vtkHardwareSelector&); // Not implemented.

  int PropID;
  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif


