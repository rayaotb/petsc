#ifndef lint
static char vcid[] = "$Id: filev.c,v 1.35 1996/02/14 15:41:09 curfman Exp bsmith $";
#endif

#include "petsc.h"
#include "pinclude/petscfix.h"
#include <stdarg.h>

struct _Viewer {
  PETSCHEADER
  int         (*flush)(Viewer);
  FILE        *fd;
  int         format;
  char        *outputname;
};

Viewer STDOUT_VIEWER_SELF, STDERR_VIEWER_SELF, STDOUT_VIEWER_WORLD;

int ViewerInitialize_Private()
{
  ViewerFileOpenASCII(MPI_COMM_SELF,"stderr",&STDERR_VIEWER_SELF);
  ViewerFileOpenASCII(MPI_COMM_SELF,"stdout",&STDOUT_VIEWER_SELF);
  ViewerFileOpenASCII(MPI_COMM_WORLD,"stdout",&STDOUT_VIEWER_WORLD);
  return 0;
}

static int ViewerDestroy_File(PetscObject obj)
{
  Viewer v = (Viewer) obj;
  int    rank = 0;
  if (v->type == ASCII_FILES_VIEWER) {MPI_Comm_rank(v->comm,&rank);} 
  if (!rank && v->fd != stderr && v->fd != stdout) fclose(v->fd);
  PLogObjectDestroy(obj);
  PetscHeaderDestroy(obj);
  return 0;
}

int ViewerDestroy_Private()
{
  ViewerDestroy_File((PetscObject)STDERR_VIEWER_SELF);
  ViewerDestroy_File((PetscObject)STDOUT_VIEWER_SELF);
  ViewerDestroy_File((PetscObject)STDOUT_VIEWER_WORLD);
  return 0;
}

int ViewerFlush_File(Viewer v)
{
  int rank;
  MPI_Comm_rank(v->comm,&rank);
  if (rank) return 0;
  fflush(v->fd);
  return 0;  
}

/*@C
    ViewerFileGetPointer - Extracts the file pointer from a viewer.

.   viewer - viewer context
.   fd - file pointer

    Note:
    This routine is not valid in Fortran.

.keywords: Viewer, file, get, pointer

.seealso: ViewerFileOpenASCII()
@*/
int ViewerFileGetPointer(Viewer viewer, FILE **fd)
{
  *fd = viewer->fd;
  return 0;
}


int ViewerFileGetOutputname_Private(Viewer viewer, char **name)
{
  *name = viewer->outputname;
  return 0;
}

int ViewerFileGetFormat_Private(Viewer viewer,int *format)
{
  *format =  viewer->format;
  return 0;
}

/*@C
   ViewerFileOpenASCII - Opens an ASCII file as a viewer.

   Input Parameters:
.  comm - the communicator
.  name - the file name

   Output Parameter:
.  lab - the viewer to use with the specified file

   Notes:
   If a multiprocessor communicator is used (such as MPI_COMM_WORLD), 
   then only the first processor in the group opens the file.  All other 
   processors send their data to the first processor to print. 

   Each processor can instead write its own independent output by
   specifying the communicator MPI_COMM_SELF.

   As shown below, ViewerFileOpenASCII() is useful in conjunction with 
   MatView() and VecView()
$
$    ViewerFileOpenASCII(MPI_COMM_WORLD,"mat.output",&viewer);
$    MatView(matrix,viewer);

   This viewer can be destroyed with ViewerDestroy().

.keywords: Viewer, file, open

.seealso: MatView(), VecView(), ViewerDestroy(), ViewerFileOpenBinary(),
          ViewerFileGetPointer()
@*/
int ViewerFileOpenASCII(MPI_Comm comm,char *name,Viewer *lab)
{
  Viewer v;
  if (comm == MPI_COMM_SELF) {
    PetscHeaderCreate(v,_Viewer,VIEWER_COOKIE,ASCII_FILE_VIEWER,comm);
  } else {
    PetscHeaderCreate(v,_Viewer,VIEWER_COOKIE,ASCII_FILES_VIEWER,comm);
  }
  PLogObjectCreate(v);
  v->destroy     = ViewerDestroy_File;
  v->flush       = ViewerFlush_File;

  if (!PetscStrcmp(name,"stderr")) v->fd = stderr;
  else if (!PetscStrcmp(name,"stdout")) v->fd = stdout;
  else {
    v->fd        = fopen(name,"w"); 
    if (!v->fd) SETERRQ(1,"ViewerFileOpenASCII:Cannot open file");
  }
  v->format        = FILE_FORMAT_DEFAULT;
  v->outputname    = 0;
#if defined(PETSC_LOG)
  PLogObjectState((PetscObject)v,"File: %s",name);
#endif
  *lab           = v;
  return 0;
}

/*@C
   ViewerFileSetFormat - Sets the format for file viewers.

   Input Parameters:
.  v - the viewer
.  format - the format
.  char - optional object name

   Notes:
   Available formats include
$    FILE_FORMAT_DEFAULT - default
$    FILE_FORMAT_MATLAB - Matlab format
$    FILE_FORMAT_IMPL - implementation-specific format
$      (which is in many cases the same as the default)
$    FILE_FORMAT_INFO - basic information about object
$    FILE_FORMAT_INFO_DETAILED 
 
   These formats are most often used for viewing matrices and vectors.
   Currently, the object name is used only in the Matlab format.

.keywords: Viewer, file, set, format

.seealso: ViewerFileOpenASCII(), MatView(), VecView()
@*/
int ViewerFileSetFormat(Viewer v,int format,char *name)
{
  PETSCVALIDHEADERSPECIFIC(v,VIEWER_COOKIE);
  if (v->type == ASCII_FILES_VIEWER || v->type == ASCII_FILE_VIEWER) {
    v->format     = format;
    v->outputname = name;
  }
  return 0;
}





