/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkObject.h"
#include "vtkTclUtil.h"
#include "vtkSetGet.h"


vtkTclInterpStruct *vtkGetInterpStruct(Tcl_Interp *interp)
{
  vtkTclInterpStruct *is = (vtkTclInterpStruct *)Tcl_GetAssocData(interp,"vtk",NULL);
  if (!is)
    {
    vtkGenericWarningMacro("unable to find interp struct");
    }
  return is;
}

VTKTCL_EXPORT int vtkTclInDelete(Tcl_Interp *interp)
{  
  vtkTclInterpStruct *is = vtkGetInterpStruct(interp);
  if (is)
    {
    return is->InDelete;
    }
  return 0;
}


// just another way into DeleteCommand
VTKTCL_EXPORT void vtkTclDeleteObjectFromHash(vtkObject *obj, void *cd, void *)
{
  vtkTclCommandArgStruct *as = (vtkTclCommandArgStruct *)cd;
  char temps[80];
  Tcl_HashEntry *entry;
  char *temp;
  vtkTclInterpStruct *is = vtkGetInterpStruct(as->Interp);
  
  // lookup the objects name
  sprintf(temps,"%p",obj);
  entry = Tcl_FindHashEntry(&is->PointerLookup,temps); 
  temp = (char *)(Tcl_GetHashValue(entry));

  Tcl_DeleteCommand(as->Interp,temp);
}

// we do no error checking in this.  We assume that if we were called
// then tcl must have been able to find the command function and object
VTKTCL_EXPORT void vtkTclGenericDeleteObject(ClientData cd)
{
  char temps[80];
  Tcl_HashEntry *entry;
  int (*command)(ClientData, Tcl_Interp *,int, char *[]);
  char *args[2];
  char *temp;
  vtkObject *tobject;
  int error;
  vtkTclCommandArgStruct *as = (vtkTclCommandArgStruct *)cd;
  Tcl_Interp *interp = as->Interp;
  vtkTclInterpStruct *is = vtkGetInterpStruct(interp);

  /* set up the args */
  args[1] = "Delete";

  // lookup the objects name
  sprintf(temps,"%p",as->Pointer);
  entry = Tcl_FindHashEntry(&is->PointerLookup,temps); 
  temp = (char *)(Tcl_GetHashValue(entry));
  args[0] = temp;
  
  // first we clear the delete callback since we will
  // always remove this object from the hash regardless
  // of if it has really been freed.
  tobject = (vtkObject *)vtkTclGetPointerFromObject(temp,"vtkObject",
						    interp, error);
  tobject->RemoveObserver(as->Tag);
  as->Tag = 0;
  
  // get the command function and invoke the delete operation
  entry = Tcl_FindHashEntry(&is->CommandLookup,temp);
  command = (int (*)(ClientData,Tcl_Interp *,int,char *[]))
    Tcl_GetHashValue(entry);
  
  // do we need to delete the c++ obj
  if (strncmp(temp,"vtkTemp",7))
    {
    is->InDelete = 1;
    command(cd,interp,2,args);
    is->InDelete = 0;
    }

  // the actual C++ object may not be freed yet. So we 
  // force it to be free from the hash table.
  Tcl_DeleteHashEntry(entry);
  entry = Tcl_FindHashEntry(&is->PointerLookup,temps); 
  Tcl_DeleteHashEntry(entry);
  entry = Tcl_FindHashEntry(&is->InstanceLookup,temp);
  Tcl_DeleteHashEntry(entry);
  delete as;
  
  if (is->DebugOn)
    {
    vtkGenericWarningMacro("vtkTcl Attempting to free object named " << temp);
    }
  if (temp)
    {
    free(temp);
    }
}

int vtkCommand(ClientData vtkNotUsed(cd), Tcl_Interp *interp, int argc, char *argv[])
{
  Tcl_HashEntry *entry;
  Tcl_HashSearch search;
  char * tmp;
  vtkTclInterpStruct *is = vtkGetInterpStruct(interp);
  
  if (argc < 2)
    {
    return TCL_OK;
    }

  if (!strcmp(argv[1],"DeleteAllObjects"))
    {
    for (entry = Tcl_FirstHashEntry(&is->PointerLookup,&search); 
	 entry != NULL;
	 entry = Tcl_FirstHashEntry(&is->PointerLookup,&search))
      {
      tmp = strdup((char *)Tcl_GetHashValue(entry));
      if (tmp)
	{
	Tcl_DeleteCommand(interp,tmp);
	}
      if (tmp)
	{
	free(tmp);
	}
      }
    return TCL_OK;
    }
  if (!strcmp(argv[1],"ListAllInstances"))
    {
    for (entry = Tcl_FirstHashEntry(&is->InstanceLookup,&search); 
	 entry != NULL; entry = Tcl_NextHashEntry(&search))
      {
      Tcl_AppendResult(interp,
		       (char *)Tcl_GetHashKey(&is->InstanceLookup,entry),NULL);
      Tcl_AppendResult(interp,"\n",NULL);
      }
    return TCL_OK;
    }
  if (!strcmp(argv[1],"DebugOn"))
    {
    is->DebugOn = 1;
    return TCL_OK;
    }
  if (!strcmp(argv[1],"DebugOff"))
    {
    is->DebugOn = 0;
    return TCL_OK;
    }
  if (!strcmp("ListMethods",argv[1]))
    {
    Tcl_AppendResult(interp,"Methods for vtkCommand:\n",NULL);
    Tcl_AppendResult(interp,"  DebugOn\n",NULL);
    Tcl_AppendResult(interp,"  DebugOff\n",NULL);
    Tcl_AppendResult(interp,"  DeleteAllObjects\n",NULL);
    Tcl_AppendResult(interp,"  ListAllInstances\n",NULL);
    return TCL_OK;
    }

  Tcl_AppendResult(interp,"invalid method for vtkCommand\n",NULL);
  return TCL_ERROR;
}

VTKTCL_EXPORT void
vtkTclGetObjectFromPointer(Tcl_Interp *interp, void *temp1,
                           int (*command)(ClientData, Tcl_Interp *,int, char *[]))
{
  int is_new;
  vtkObject *temp = (vtkObject *)temp1;
  char temps[80];
  char name[80];
  Tcl_HashEntry *entry;
  vtkTclInterpStruct *is = vtkGetInterpStruct(interp);

  /* if it is NULL then return empty string */
  if (!temp)
    {
    Tcl_ResetResult(interp);
    return;
    }
  
  /* return a pointer to a vtk Object */
  if (is->DebugOn)
    {
      vtkGenericWarningMacro("Looking up name for vtk pointer: " << temp);
    }

  /* first we must look up the pointer to see if it already exists */
  sprintf(temps,"%p",temp);
  if ((entry = Tcl_FindHashEntry(&is->PointerLookup,temps))) 
    {
    if (is->DebugOn)
      {
	vtkGenericWarningMacro("Found name: " 
			       << (char *)(Tcl_GetHashValue(entry)) 
			       << " for vtk pointer: " << temp);
      }
    
    /* while we are at it store the name since it is required anyhow */
    Tcl_SetResult(interp, (char *)(Tcl_GetHashValue(entry)), TCL_VOLATILE);
    return;
    }

  /* we must create a new name if it isn't NULL */
  sprintf(name,"vtkTemp%i",is->Number);
  is->Number++;
  
  if (is->DebugOn)
    {
      vtkGenericWarningMacro("Created name: " << name
			     << " for vtk pointer: " << temp);
    }

  // check to see if we can find the command function based on class name
  Tcl_CmdInfo cinf;
  char *tstr = strdup(temp->GetClassName());
  if (Tcl_GetCommandInfo(interp,tstr,&cinf))
    {
    if (cinf.clientData)
      {
      vtkTclCommandStruct *cs = (vtkTclCommandStruct *)cinf.clientData;
      command = cs->CommandFunction;
      }
    }
  if (tstr)
    {
    free(tstr);
    }

  entry = Tcl_CreateHashEntry(&is->InstanceLookup,name,&is_new);
  Tcl_SetHashValue(entry,(ClientData)(temp));
  entry = Tcl_CreateHashEntry(&is->PointerLookup,temps,&is_new);
  Tcl_SetHashValue(entry,(ClientData)(strdup(name)));
  vtkTclCommandArgStruct *as = new vtkTclCommandArgStruct;
  as->Pointer = (void *)temp;
  as->Interp = interp;
  Tcl_CreateCommand(interp,name,command,
		    (ClientData)(as),
		    (Tcl_CmdDeleteProc *)vtkTclGenericDeleteObject);
  entry = Tcl_CreateHashEntry(&is->CommandLookup,name,&is_new);
  Tcl_SetHashValue(entry,(ClientData)command);
  
  // setup the delete callback
  vtkCallbackCommand *cbc = new vtkCallbackCommand;
  cbc->SetCallback(vtkTclDeleteObjectFromHash);
  cbc->SetClientData((void *)as);
  as->Tag = temp->AddObserver(vtkCommand::DeleteEvent, cbc);
  
  Tcl_SetResult(interp, (char *)name, TCL_VOLATILE);
}
      
VTKTCL_EXPORT void *vtkTclGetPointerFromObject(const char *name,
					       const char *result_type,
					       Tcl_Interp *interp, 
					       int &error)
{
  Tcl_HashEntry *entry;
  ClientData temp;
  int (*command)(ClientData, Tcl_Interp *,int, char *[]);
  char *args[3];
  char temps[256];
  vtkTclInterpStruct *is = vtkGetInterpStruct(interp);

  /* check for empty string, empty string is the same as passing NULL */
  if (name[0] == '\0')
    {
    return NULL;
    }
  
  // object names cannot start with a number
  if ((name[0] >= '0')&&(name[0] <= '9'))
    {
    error = 1;
    return NULL;
    }

  if ((entry = Tcl_FindHashEntry(&is->InstanceLookup,name)))
    {
    temp = (ClientData)Tcl_GetHashValue(entry);
    }
  else
    {
    sprintf(temps,"vtk bad argument, could not find object named %s\n", name);
    Tcl_AppendResult(interp,temps,NULL);
    error = 1;
    return NULL;
    }

  /* now handle the typecasting, get the command proc */
  if ((entry = Tcl_FindHashEntry(&is->CommandLookup,name)))
    {
    command = (int (*)(ClientData,Tcl_Interp *,int,char *[]))Tcl_GetHashValue(entry);
    }
  else
    {
    sprintf(temps,"vtk bad argument, could not find command process for %s.\n", name);
    Tcl_AppendResult(interp,temps,NULL);
    error = 1;
    return NULL;
    }

  /* set up the args */
  args[0] = "DoTypecasting";
  args[1] = strdup(result_type);
  args[2] = NULL;
  vtkTclCommandArgStruct foo;
  foo.Pointer = temp;
  foo.Interp = interp;
  if (command((ClientData)&foo,(Tcl_Interp *)NULL,3,args) == TCL_OK)
    {
    free (args[1]);
    return (void *)(args[2]);
    }
  else
    {
    Tcl_Interp *i;
    i = Tcl_CreateInterp();
    // provide more diagnostic info
    args[0] = "Dummy";
    free (args[1]);
    args[1] = "GetClassName";
    args[2] = NULL;
    command((ClientData)&foo,i,2,args);

    sprintf(temps,"vtk bad argument, type conversion failed for object %s.\nCould not type convert %s which is of type %s, to type %s.\n", name, name, i->result, result_type);
    Tcl_AppendResult(interp,temps,NULL);
    error = 1;
    Tcl_DeleteInterp(i);
    return NULL;
    }

}

VTKTCL_EXPORT void vtkTclVoidFunc(void *arg)
{
  int res;

  vtkTclVoidFuncArg *arg2;

  arg2 = (vtkTclVoidFuncArg *)arg;

  res = Tcl_GlobalEval(arg2->interp, arg2->command);

  if (res == TCL_ERROR)
    {
    if (Tcl_GetVar(arg2->interp,"errorInfo",0))
      {
      vtkGenericWarningMacro("Error returned from vtk/tcl callback:\n" <<
			     arg2->command << endl <<
			     Tcl_GetVar(arg2->interp,"errorInfo",0) <<
			     " at line number " << arg2->interp->errorLine);
      }
    else
      {
      vtkGenericWarningMacro("Error returned from vtk/tcl callback:\n" <<
			     arg2->command << endl <<
			     " at line number " << arg2->interp->errorLine);
      }
    }
}

VTKTCL_EXPORT void vtkTclVoidFuncArgDelete(void *arg)
{
  vtkTclVoidFuncArg *arg2;

  arg2 = (vtkTclVoidFuncArg *)arg;
  
  // free the string and then structure
  delete [] arg2->command;
  delete arg2;
}

VTKTCL_EXPORT void vtkTclListInstances(Tcl_Interp *interp, ClientData arg)
{
  Tcl_HashSearch srch;
  Tcl_HashEntry *entry;
  int first = 1;
  vtkTclInterpStruct *is = vtkGetInterpStruct(interp);

  // iteratively search hash table for command function
  entry = Tcl_FirstHashEntry(&is->CommandLookup, &srch);
  if (!entry) 
    {
    Tcl_ResetResult(interp);
    return;
    }
  while (entry)
    {
    if (Tcl_GetHashValue(entry) == arg)
      {
      if (first)
	{
	first = 0;
	Tcl_AppendResult(interp,Tcl_GetHashKey(&is->CommandLookup,entry),NULL);
	}
      else
	{
	Tcl_AppendResult(interp, " ", Tcl_GetHashKey(&is->CommandLookup,entry), 
			 NULL);
	}
      }
    entry = Tcl_NextHashEntry(&srch);
    }
}

  
int vtkTclNewInstanceCommand(ClientData cd, Tcl_Interp *interp,
			     int argc, char *argv[])
{
  int (*command)(ClientData, Tcl_Interp *,int, char *[]);
  Tcl_HashEntry *entry;
  int is_new;
  char temps[80];
  vtkTclCommandStruct *cs = (vtkTclCommandStruct *)cd;
  Tcl_CmdInfo cinf;
  vtkTclInterpStruct *is = vtkGetInterpStruct(interp);

  if (argc != 2)
    {
    Tcl_SetResult(interp, "vtk object creation requires one argument, a name.", TCL_VOLATILE);
    return TCL_ERROR;
    }

  if ((argv[1][0] >= '0')&&(argv[1][0] <= '9'))
    { 
    Tcl_SetResult(interp, argv[1], TCL_VOLATILE);
    Tcl_AppendResult(interp, ": vtk object cannot start with a numeric.", NULL);
    return TCL_ERROR;
    }

  if (Tcl_FindHashEntry(&is->InstanceLookup,argv[1]))
    { 
    Tcl_SetResult(interp, argv[1], TCL_VOLATILE);
    Tcl_AppendResult(interp,
		     ": a vtk object with that name already exists.",
		     NULL);
    return TCL_ERROR;
    }

  // Make sure we are not clobbering a built in command
  if (Tcl_GetCommandInfo(interp,argv[1],&cinf))
    {
    vtkGenericWarningMacro(<< argv[1] << ": a tcl/tk command with that name already exists.");
    }

  ClientData temp;
  if (!strcmp("ListInstances",argv[1]))
    {
    vtkTclListInstances(interp,(ClientData) cs->CommandFunction);
    return TCL_OK;
    }
  temp = cs->NewCommand();

  entry = Tcl_CreateHashEntry(&is->InstanceLookup,argv[1],&is_new);
  Tcl_SetHashValue(entry,temp);
  sprintf(temps,"%p",(void *)temp);
  entry = Tcl_CreateHashEntry(&is->PointerLookup,temps,&is_new);
  Tcl_SetHashValue(entry,(ClientData)(strdup(argv[1])));
  
  // check to see if we can find the command function based on class name
  char *tstr = strdup(((vtkObject *)temp)->GetClassName());
  if (Tcl_GetCommandInfo(interp,tstr,&cinf))
    {
    if (cinf.clientData)
      {
      vtkTclCommandStruct *cs2 = (vtkTclCommandStruct *)cinf.clientData;
      command = cs2->CommandFunction;
      }
    else
      {
      command = cs->CommandFunction;
      }
    }
  else
    {
    command = cs->CommandFunction;
    }
  if (tstr)
    {
    free(tstr);
    }

  vtkTclCommandArgStruct *as = new vtkTclCommandArgStruct;
  as->Pointer = (void *)temp;
  as->Interp = interp;
  Tcl_CreateCommand(interp,argv[1],command,
                    (ClientData)as,
                    (Tcl_CmdDeleteProc *)vtkTclGenericDeleteObject);
  entry = Tcl_CreateHashEntry(&is->CommandLookup,argv[1],&is_new);
  Tcl_SetHashValue(entry,(ClientData)(cs->CommandFunction));
  
  // setup the delete callback
  vtkCallbackCommand *cbc = new vtkCallbackCommand;
  cbc->SetCallback(vtkTclDeleteObjectFromHash);
  cbc->SetClientData((void *)as);
  as->Tag = ((vtkObject *)temp)->AddObserver(vtkCommand::DeleteEvent, cbc);

  Tcl_SetResult(interp, argv[1], TCL_VOLATILE);
  return TCL_OK;
}

void vtkTclDeleteCommandStruct(ClientData cd)
{
  vtkTclCommandStruct *cs = (vtkTclCommandStruct *)cd;
  delete cs;
}

void vtkTclCreateNew(Tcl_Interp *interp, char *cname,
                     ClientData (*NewCommand)(),
                     int (*CommandFunction)(ClientData cd,
					    Tcl_Interp *interp,
					    int argc, char *argv[]))
{
  vtkTclCommandStruct *cs = new vtkTclCommandStruct;
  cs->NewCommand = NewCommand;
  cs->CommandFunction = CommandFunction;
  Tcl_CreateCommand(interp,cname,vtkTclNewInstanceCommand,
                   (ClientData *)cs,
                   (Tcl_CmdDeleteProc *)vtkTclDeleteCommandStruct);
}
