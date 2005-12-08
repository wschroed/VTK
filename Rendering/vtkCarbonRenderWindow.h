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
// .NAME vtkCarbonRenderWindow - Carbon OpenGL rendering window
//
// .SECTION Description
// vtkCarbonRenderWindow is a concrete implementation of the abstract
// class vtkOpenGLRenderWindow. vtkCarbonRenderWindow interfaces to the
// OpenGL graphics library using the Carbon API on Mac OSX.

#ifndef __vtkCarbonRenderWindow_h
#define __vtkCarbonRenderWindow_h

#include "vtkOpenGLRenderWindow.h"

#include <Carbon/Carbon.h> // Carbon and MAC specific
#include <AGL/agl.h> // Carbon and MAC specific

class vtkCarbonRenderWindowInternal;


class VTK_RENDERING_EXPORT vtkCarbonRenderWindow : public vtkOpenGLRenderWindow
{
public:
  static vtkCarbonRenderWindow *New();
  vtkTypeRevisionMacro(vtkCarbonRenderWindow,vtkOpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Begin the rendering process.
  void Start();

  // Description:
  // End the rendering process and display the image.
  void Frame();

  // Description:
  // Specify various window parameters.
  virtual void WindowConfigure();

  // Description:
  // Create the window.
  virtual void WindowInitialize();

  // Description:
  // Initialize the rendering window. This will setup all system-specific
  // resources. This method and Finalize() must be symmetric and it
  // should be possible to call them multiple times, even changing WindowId
  // in-between. This is what WindowRemap does.
  virtual void Initialize();

  // Description:
  // Finalize the rendering window. This will shutdown all system-specifc
  // resources. After having called this, it should be possible to destroy
  // a window that was used for a SetWindowId() call without any ill effects.
  virtual void Finalize();

  // Description:
  // Create a rendering area in memory.
  void SetOffScreenRendering(int);

  // Description:
  // Change the window to fill the entire screen.
  virtual void SetFullScreen(int);

  // Description:
  // Remap the window.
  virtual void WindowRemap();

  // Description:
  // Set the preferred window size to full screen.
  virtual void PrefFullScreen();

  // Description:
  // Set the size of the window.
  virtual void SetSize(int a[2]);
  virtual void SetSize(int,int);

  // Description:
  // Get the current size of the window.
  virtual int *GetSize();

  // Description:
  // Set the position of the window.
  virtual void SetPosition(int*);
  virtual void SetPosition(int,int);

  // Description:
  // Return the scrren size.
  virtual int *GetScreenSize();

  // Description:
  // Get the position in screen coordinates of the window.
  virtual int *GetPosition();

  // Description:
  // Set the name of the window. This appears at the top of the window
  // normally.
  virtual void SetWindowName(const char *);
  
  // Description:
  // Set this RenderWindow's window id to a pre-existing window.
  void SetWindowInfo(char *);

  void SetNextWindowInfo(char *)
     {
      vtkWarningMacro("SetNextWindowInfo not implemented (WindowRemap not implemented).");
     }

  //BTX
  virtual void *GetGenericDisplayId() {return NULL;}
  virtual void *GetGenericWindowId()  {return (void *)this->WindowId;}
  virtual void *GetGenericParentId()  {return (void *)this->ParentId;}
  virtual AGLContext GetContextId();
  virtual void *GetGenericContext()  {return (void*)this->GetContextId();}
  virtual void SetDisplayId(void *) {}

  virtual void* GetGenericDrawable()
    {
      vtkWarningMacro("GetGenericDrawable Method not implemented.");
      return 0;
    }

  void SetParentInfo(char*)
    {
      vtkWarningMacro("SetParentInfo Method not implemented.");
    }

  // Description:
  // Get the HIView window pointer.
  virtual HIViewRef GetWindowId();
  // Set the HIView window pointer.
  void  SetWindowId(void *foo) {this->SetWindowId((HIViewRef)foo);};
  void SetNextWindowId(void*)
    {
       vtkWarningMacro("SetNextWindowId not implemented (WindowRemap not implemented).");
    }

  // Description:
  // Set the parent HIView.
  virtual void SetParentId(HIViewRef);
  void  SetParentId(void *foo) {this->SetParentId((HIViewRef)foo);};
  
  // Description:
  // Set the HIVIew pointer to a pre-existing window.
  virtual void SetWindowId(HIViewRef);

  // Description:
  // Set the root window id.  Use this when using non-HIView GUIs.
  void SetRootWindow(WindowPtr win);
  WindowPtr GetRootWindow();

  //ETX

  // supply base class virtual function
  vtkSetMacro(MultiSamples,int);
  vtkGetMacro(MultiSamples,int);

  // Description:
  // Prescribe that the window be created in a stereo-capable mode. This
  // method must be called before the window is realized. This method
  // overrrides the superclass method since this class can actually check
  // whether the window has been realized yet.
  virtual void SetStereoCapableWindow(int capable);

  // Description:
  // Make this windows OpenGL context the current context.
  void MakeCurrent();

  // Description:
  // If called, allow MakeCurrent() to skip cache-check when called.
  // MakeCurrent() reverts to original behavior of cache-checking
  // on the next render.
  void SetForceMakeCurrent();

  // Description:
  // Check to see if an event is pending for this window.
  // This is a useful check to abort a long render.
  virtual  int GetEventPending();

  // Description:
  // Initialize OpenGL for this window.
  virtual void SetupPalette(void *hDC);
  virtual void SetupPixelFormat(void *hDC, void *dwFlags, int debug, 
                                int bpp=16, int zbpp=16);

  // Description:
  // Get the size of the depth buffer.
  int GetDepthBufferSize();

  // Description:
  // Hide or Show the mouse cursor, it is nice to be able to hide the
  // default cursor if you want VTK to display a 3D cursor instead.
  void HideCursor();
  void ShowCursor();
  
  void UpdateSizeAndPosition(int xPos, int yPos, int xSize, int ySize);

  // Description:
  // Fix the GL region.  The AGL_BUFFER_RECT and AGL_CLIP_REGION will be updated
  void UpdateGLRegion();

  
protected:
  vtkCarbonRenderWindow();
  ~vtkCarbonRenderWindow();

  vtkCarbonRenderWindowInternal* Internal;

  int ApplicationInitialized; // Toolboxen initialized?
  Boolean fAcceleratedMust;   // input: must renderer be accelerated?
  Boolean draggable;          // input: is the window draggable?
  GLint aglAttributes[64];    // input: pixel format attributes always required
                              //   (reset to what was actually allocated)
  SInt32 VRAM;                // input: minimum VRAM; output: actual
                              //   (if successful otherwise input)
  SInt32 textureRAM;          // input: amount of texture RAM required on card;
                              // output: same (used in allocation)
  AGLContext ContextId;
  HIViewRef WindowId;
  HIViewRef ParentId;
  WindowPtr RootWindow;
  int OwnWindow;
  int ScreenSize[2];

  int CursorHidden;
  int ForceMakeCurrent;


 // data and handlers to keep the GL view coincident with the HIView
  EventHandlerUPP RegionEventHandlerUPP;
  EventHandlerRef RegionEventHandler;
  static OSStatus RegionEventProcessor(EventHandlerCallRef er, EventRef event, void*);

  void InitializeApplication();

  void CreateAWindow(int x, int y, int width, int height);
  void DestroyWindow();
  
  void CreateOffScreenWindow(int x, int y);
  void DestroyOffScreenWindow();
  void ResizeOffScreenWindow(int x, int y);

private:
  vtkCarbonRenderWindow(const vtkCarbonRenderWindow&);  // Not implemented.
  void operator=(const vtkCarbonRenderWindow&);  // Not implemented.
};

#endif
