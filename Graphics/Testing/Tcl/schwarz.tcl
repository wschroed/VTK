package require vtktcl

vtkPoints points
  points InsertNextPoint 2 4 0
  points InsertNextPoint 2.6 2.6 0
  points InsertNextPoint 4 2 0
  points InsertNextPoint 1.4 4 1.4
  points InsertNextPoint 2 3 1
  points InsertNextPoint 3 2 1
  points InsertNextPoint 4 1.4 1.4
  points InsertNextPoint 0 4 2
  points InsertNextPoint 1 3 2
  points InsertNextPoint 2 2 2
  points InsertNextPoint 3 1 2
  points InsertNextPoint 4 0 2
  points InsertNextPoint 0 2.6 2.6
  points InsertNextPoint 1 2 3
  points InsertNextPoint 2 1 3
  points InsertNextPoint 2.6 0 2.6
  points InsertNextPoint 0 2 4
  points InsertNextPoint 1.4 1.4 4
  points InsertNextPoint 2 0 4

vtkCellArray faces
  faces InsertNextCell 3
  faces InsertCellPoint 0
  faces InsertCellPoint 3
  faces InsertCellPoint 4
  faces InsertNextCell 3
  faces InsertCellPoint 0
  faces InsertCellPoint 4
  faces InsertCellPoint 1
  faces InsertNextCell 3
  faces InsertCellPoint 1
  faces InsertCellPoint 4
  faces InsertCellPoint 5
  faces InsertNextCell 3
  faces InsertCellPoint 1
  faces InsertCellPoint 5
  faces InsertCellPoint 2
  faces InsertNextCell 3
  faces InsertCellPoint 2
  faces InsertCellPoint 5
  faces InsertCellPoint 6
  faces InsertNextCell 3
  faces InsertCellPoint 3
  faces InsertCellPoint 7
  faces InsertCellPoint 8
  faces InsertNextCell 3
  faces InsertCellPoint 3
  faces InsertCellPoint 8
  faces InsertCellPoint 4
  faces InsertNextCell 3
  faces InsertCellPoint 4
  faces InsertCellPoint 8
  faces InsertCellPoint 9
  faces InsertNextCell 3
  faces InsertCellPoint 4
  faces InsertCellPoint 9
  faces InsertCellPoint 5
  faces InsertNextCell 3
  faces InsertCellPoint 5
  faces InsertCellPoint 9
  faces InsertCellPoint 10
  faces InsertNextCell 3
  faces InsertCellPoint 5
  faces InsertCellPoint 10
  faces InsertCellPoint 6
  faces InsertNextCell 3
  faces InsertCellPoint 6
  faces InsertCellPoint 10
  faces InsertCellPoint 11
  faces InsertNextCell 3
  faces InsertCellPoint 7
  faces InsertCellPoint 12
  faces InsertCellPoint 8
  faces InsertNextCell 3
  faces InsertCellPoint 8
  faces InsertCellPoint 12
  faces InsertCellPoint 13
  faces InsertNextCell 3
  faces InsertCellPoint 8
  faces InsertCellPoint 13
  faces InsertCellPoint 9
  faces InsertNextCell 3
  faces InsertCellPoint 9
  faces InsertCellPoint 13
  faces InsertCellPoint 14
  faces InsertNextCell 3
  faces InsertCellPoint 9
  faces InsertCellPoint 14
  faces InsertCellPoint 10
  faces InsertNextCell 3
  faces InsertCellPoint 10
  faces InsertCellPoint 14
  faces InsertCellPoint 15
  faces InsertNextCell 3
  faces InsertCellPoint 10
  faces InsertCellPoint 15
  faces InsertCellPoint 11
  faces InsertNextCell 3
  faces InsertCellPoint 12
  faces InsertCellPoint 16
  faces InsertCellPoint 13
  faces InsertNextCell 3
  faces InsertCellPoint 13
  faces InsertCellPoint 16
  faces InsertCellPoint 17
  faces InsertNextCell 3
  faces InsertCellPoint 13
  faces InsertCellPoint 17
  faces InsertCellPoint 14
  faces InsertNextCell 3
  faces InsertCellPoint 14
  faces InsertCellPoint 17
  faces InsertCellPoint 18
  faces InsertNextCell 3
  faces InsertCellPoint 14
  faces InsertCellPoint 18
  faces InsertCellPoint 15
  
vtkPolyData model
  model SetPolys faces
  model SetPoints points

vtkMath rn

vtkUnsignedCharArray cellColors
#cellColors SetName "test"
  cellColors SetNumberOfComponents 3
  cellColors SetNumberOfTuples [model GetNumberOfCells]
for { set i 0 } { $i < [model GetNumberOfCells] } { incr i } {
    cellColors InsertComponent $i 0 [rn Random 100 255]
    cellColors InsertComponent $i 1 [rn Random 100 255]
    cellColors InsertComponent $i 2 [rn Random 100 255]
}

[model GetCellData] SetScalars cellColors

vtkTransform t0
  t0 Identity
vtkTransformPolyDataFilter tf0
  tf0 SetTransform t0
  tf0 SetInput model

vtkTransform t1
  t1 Identity
  t1 RotateZ 90
vtkTransformPolyDataFilter tf1
  tf1 SetTransform t1
  tf1 SetInput model

vtkTransform t2
  t2 Identity
  t2 RotateZ 180
vtkTransformPolyDataFilter tf2
  tf2 SetTransform t2
  tf2 SetInput model

vtkTransform t3
  t3 Identity
  t3 RotateZ 270
vtkTransformPolyDataFilter tf3
  tf3 SetTransform t3
  tf3 SetInput model

vtkAppendPolyData af
  af AddInput [tf0 GetOutput]
  af AddInput [tf1 GetOutput]
  af AddInput [tf2 GetOutput]
  af AddInput [tf3 GetOutput]

vtkTransform t4
  t4 Identity
  t4 RotateX 180
vtkTransformPolyDataFilter tf4
  tf4 SetTransform t4
  tf4 SetInput [af GetOutput]

vtkAppendPolyData af2
  af2 AddInput [af GetOutput]
  af2 AddInput [tf4 GetOutput]

vtkTransform t5
  t5 Identity
  t5 Translate 0 0 -8
vtkTransformPolyDataFilter tf5
  tf5 SetTransform t5
  tf5 SetInput [af2 GetOutput]

vtkAppendPolyData af3
  af3 AddInput [af2 GetOutput]
  af3 AddInput [tf5 GetOutput]

vtkTransform t6
  t6 Identity
  t6 Translate 0 -8 0
vtkTransformPolyDataFilter tf6
  tf6 SetTransform t6
  tf6 SetInput [af3 GetOutput]

vtkAppendPolyData af4
  af4 AddInput [af3 GetOutput]
  af4 AddInput [tf6 GetOutput]

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkCleanPolyData clean
  clean SetTolerance .001
  clean SetInput model
  clean SetInput [af2 GetOutput]
  clean SetInput [af3 GetOutput]
  clean SetInput [af4 GetOutput]

vtkButterflySubdivisionFilter subdivide
  subdivide SetInput [clean GetOutput]
  subdivide SetNumberOfSubdivisions 3

vtkDataSetMapper mapper
   mapper SetInput [subdivide GetOutput]

vtkActor surface
    surface SetMapper mapper

vtkFeatureEdges fe
  fe SetInput [subdivide GetOutput]
  fe SetFeatureAngle 100

vtkStripper feStripper
  feStripper SetInput [fe GetOutput]

vtkTubeFilter feTubes
  feTubes SetInput [feStripper GetOutput]
  feTubes SetRadius .1

vtkPolyDataMapper feMapper
  feMapper SetInput [feTubes GetOutput]

vtkActor edges
  edges SetMapper feMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor surface
ren1 AddActor edges
vtkProperty backP
  backP SetDiffuseColor 1 1 .3
surface SetBackfaceProperty backP

[edges GetProperty] SetDiffuseColor .2 .2 .2
[surface GetProperty] SetDiffuseColor 1 .4 .3
[surface GetProperty] SetSpecular .4
[surface GetProperty] SetDiffuse .8
[surface GetProperty] SetSpecularPower 40

ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

set cam1 [ren1 GetActiveCamera]
$cam1 Azimuth 90
ren1 ResetCamera
$cam1 Zoom 1.5
iren Initialize

# prevent the tk window from showing up then start the event loop
wm withdraw .


