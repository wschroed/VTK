/* this is a CMake loadable command to wrap vtk objects into Java */

#include "cmCPluginAPI.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define SINGLE_FILE_BUILD

/* do almost everything in the initial pass */
static int InitialPass(void *inf, void *mf, int argc, char *argv[])
{
  cmLoadedCommandInfo *info = (cmLoadedCommandInfo *)inf;
  int i;
  int newArgc;
  int msize;
  int estimated;
  char **newArgv;
  int numClasses = 0;
  char **classes = 0;
  char **dependencies = 0;
  int numDep = 0;
  int numWrapped = 0;
  const char *cdir = info->CAPI->GetCurrentDirectory(mf);
  const char *def = 0;
  char *newName;
  char *target = 0;
  const char *javac = info->CAPI->GetDefinition(mf,"JAVA_COMPILE");
  const char *jar = info->CAPI->GetDefinition(mf,"JAVA_ARCHIVE");
  void *cfile = 0;
  char *args[5];
  char *jargs[5];
  int depends = 0;
  char **sargs = 0;
  int sargsCnt = 0;
  char **srcList = 0;
  int srcCnt = 0;
  char* javaPath = 0;
  const char *libpath = info->CAPI->GetDefinition(mf,"LIBRARY_OUTPUT_PATH");
  const char *vtkpath = info->CAPI->GetDefinition(mf,"VTK_BINARY_DIR");
  const char *startTempFile;
  const char *endTempFile;

  if(argc < 3 )
    {
    info->CAPI->SetError(info, "called with incorrect number of arguments");
    return 0;
    }

  /* Now check and see if the value has been stored in the cache */
  /* already, if so use that value and don't look for the program */
  if(!info->CAPI->IsOn(mf,"VTK_WRAP_JAVA"))
    {
    info->CAPI->FreeArguments(newArgc, newArgv);
    return 1;
    }

  info->CAPI->ExpandSourceListArguments(mf, argc, (const char**)argv, 
                                        &newArgc, &newArgv, 2);
  
  /* keep the library name */
  target = strdup(newArgv[0]);

  javaPath = (char *)malloc(strlen(vtkpath) + 20);
  sprintf(javaPath, "%s/java", vtkpath);

  args[0] = strdup("-classpath");
  args[1] = strdup(javaPath);

  classes = (char**) malloc( newArgc * sizeof(char*));
  numClasses = 0;
  
  /* get the classes for this lib */
  for(i = 1; i < newArgc; ++i)
    {   
    const char *srcName = newArgv[i];
    char *className = 0;
    char *srcNameWe;
    char *srcPath;

    srcNameWe = info->CAPI->GetFilenameWithoutExtension(srcName);
    srcPath   = info->CAPI->GetFilenamePath(srcName);
    
    className = (char *)malloc(strlen(srcPath) + strlen(srcNameWe) + 20);
    sprintf(className,"%s/%s.class",srcPath,srcNameWe);
    
    args[2] = strdup(srcName);

#ifndef _WIN32
    /* On Unix we can just call javac ... *.java */
    sprintf(args[2], "%s/*.java", srcPath);
#endif
    info->CAPI->AddCustomCommand(mf, srcName, javac, 3, (const char**)args, 
                                 0, 0, 1, (const char**)&className, target);
    
    free(args[2]);
    info->CAPI->Free(srcNameWe);
    info->CAPI->Free(srcPath);
    classes[numClasses++] = strdup(className);
    free(className);
    }
  free(args[0]);
  free(args[1]);

  info->CAPI->AddCustomCommand(mf, target, "", 0, 0,
                               numClasses, (const char**)classes, 0, 0, target);
  
  for (i = 0; i < numClasses; i ++ )
    {
    free(classes[i]);
    }
  free(classes);

  free(javaPath);

  free(target);

  info->CAPI->FreeArguments(newArgc, newArgv);
  return 1;
}
  
  
static void FinalPass(void *inf, void *mf) 
{
}

static void Destructor(void *inf) 
{
}

static const char* GetTerseDocumentation() 
{
  return "Create Java Archive.";
}

static const char* GetFullDocumentation()
{
  return
    "VTK_WRAP_JAVA(target SourceLists ...)";
}

void CM_PLUGIN_EXPORT VTK_GENERATE_JAVA_DEPENDENCIESInit(cmLoadedCommandInfo *info)
{
  info->InitialPass = InitialPass;
  info->FinalPass = FinalPass;
  info->Destructor = Destructor;
  info->GetTerseDocumentation = GetTerseDocumentation;
  info->GetFullDocumentation = GetFullDocumentation;  
  info->m_Inherited = 0;
  info->Name = "VTK_GENERATE_JAVA_DEPENDENCIES";
}




