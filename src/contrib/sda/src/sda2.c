/*$Id: sda2.c,v 1.24 2001/08/07 03:04:49 balay Exp $*/
/*
    Simplified interface to PETSC DA (distributed array) object. 
   This is for a user who is not using PETSc Vecs (vectors).
*/

#include "src/contrib/sda/src/sda.h"    /*I   "da.h"   I*/

extern int DALocalToLocalCreate(DA);

struct _SDA {
  DA  da;
  Vec gvec,lvec,Gvec;
};

#undef __FUNCT__  
#define __FUNCT__ "SDAArrayView"
int SDAArrayView(SDA da,PetscScalar *values,PetscViewer v)
{
  int ierr;

  PetscFunctionBegin;
  ierr = VecPlaceArray(da->lvec,values);CHKERRQ(ierr);
  if (!da->Gvec) {
    ierr = DACreateGlobalVector(da->da,&da->Gvec);CHKERRQ(ierr);
  }
  ierr = DALocalToGlobalBegin(da->da,da->lvec,da->Gvec);CHKERRQ(ierr);
  ierr = DALocalToGlobalEnd(da->da,da->lvec,da->Gvec);CHKERRQ(ierr);
  ierr = VecView(da->Gvec,v);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "SDACreate1d"
/*@C
    SDACreate1d - Creates a one-dimensional regular array that is
    distributed across some processors. This is the simplified 
    interface, must be used with SDAXXX operations, NOT DAXXX operations.

   Input Parameters:
.  comm - MPI communicator
.  wrap - type of periodicity should the array have, if any
$         DA_NONPERIODIC, DA_XPERIODIC
.  M - global dimension of the array
.  w - number of degress of freedom per node
.  s - stencil width
.  lc - array containing number of nodes in X direction on each processor, or PETSC_NULL

   Output Parameter:
.  sda - the resulting array object

  Level: beginner

.keywords: distributed array, create, two-dimensional

.seealso: SDADestroy(), SDACreate2d(), SDACreate3d()
@*/
int SDACreate1d(MPI_Comm comm,DAPeriodicType wrap,int M,int w,int s,int *lc,SDA *sda)
{
  int           ierr;
  DA            da;
  char          **args;
  int           argc = 0;
  Vec           tmp;

  PetscInitialize(&argc,&args,0,0);
  PetscFunctionBegin;

  ierr = PetscNew(struct _SDA,sda);CHKERRQ(ierr);
  ierr = DACreate1d(comm,wrap,M,w,s,lc,&da);CHKERRQ(ierr);
  (*sda)->da = da;

  /* set up two dummy work vectors for the vector scatter */
  ierr = DACreateLocalVector(da,&(*sda)->gvec);CHKERRQ(ierr);
  /* we free the actual space in the vectors because it is not 
     needed since the user provides her/his own with SDA */
  ierr = VecReplaceArray((*sda)->gvec,PETSC_NULL);CHKERRQ(ierr);
  ierr = VecDuplicate((*sda)->gvec,&(*sda)->lvec);CHKERRQ(ierr);
  ierr = VecReplaceArray((*sda)->lvec,PETSC_NULL);CHKERRQ(ierr);

  /* destroy global vector */
  ierr = DACreateGlobalVector(da,&tmp);CHKERRQ(ierr);
  ierr = VecDestroy(tmp);CHKERRQ(ierr);
  (*sda)->Gvec = 0;

  /* free scatters in DA never needed by user */
  ierr = DALocalToLocalCreate(da);CHKERRQ(ierr);
  /* ierr = VecScatterDestroy(da->ltog);CHKERRQ(ierr);da->ltog = 0; */
  /* ierr = VecScatterDestroy(da->gtol);CHKERRQ(ierr);da->gtol = 0;  */

  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "SDACreate2d"
/*@C
    SDACreate2d - Creates a two-dimensional regular array that is
    distributed across some processors. This is the simplified 
    interface, must be used with SDAXXX operations, NOT DAXXX operations.

   Input Parameters:
.  comm - MPI communicator
.  wrap - type of periodicity should the array have, if any
$         DA_NONPERIODIC, DA_XPERIODIC, 
$         DA_YPERIODIC, DA_XYPERIODIC  
.  stencil_type - stencil type either DA_STENCIL_BOX or DA_STENCIL_STAR
.  M,N - global dimension in each direction of the array
.  m,n - corresponding number of processors in each dimension 
         (or PETSC_DECIDE to have calculated)
.  w - number of degress of freedom per node
.  s - stencil width
.  lx, ly - arrays containing the number of nodes in each cell along
$           the x and y coordinates, or PETSC_NULL

   Output Parameter:
.  inra - the resulting array object

  Level: beginner

.keywords: distributed array, create, two-dimensional

.seealso: DADestroy(), DAView(), DACreate1d(), DACreate3d()
@*/
int SDACreate2d(MPI_Comm comm,DAPeriodicType wrap,DAStencilType stencil_type,
                int M,int N,int m,int n,int w,int s,int *lx,int *ly,SDA *sda)
{
  int           ierr;
  DA            da;
  char          **args;
  int           argc = 0;
  Vec           tmp;

  PetscInitialize(&argc,&args,0,0);
  PetscFunctionBegin;

  ierr = PetscNew(struct _SDA,sda);CHKERRQ(ierr);
  ierr = DACreate2d(comm,wrap,stencil_type,M,N,m,n,w,s,lx,ly,&da);CHKERRQ(ierr);
  (*sda)->da = da;

  /* set up two dummy work vectors for the vector scatter */
  ierr = DACreateLocalVector(da,&(*sda)->gvec);CHKERRQ(ierr);
  /* we free the actual space in the vectors because it is not 
     needed since the user provides her/his own with SDA */
  ierr = VecReplaceArray((*sda)->gvec,PETSC_NULL);CHKERRQ(ierr);
  ierr = VecDuplicate((*sda)->gvec,&(*sda)->lvec);CHKERRQ(ierr);
  ierr = VecReplaceArray((*sda)->lvec,PETSC_NULL);CHKERRQ(ierr);

  /* destroy global vector */
  ierr = DACreateGlobalVector(da,&tmp);CHKERRQ(ierr);
  ierr = VecDestroy(tmp);CHKERRQ(ierr);
  (*sda)->Gvec = 0;

  /* free scatters in DA never needed by user */
  ierr = DALocalToLocalCreate(da);CHKERRQ(ierr);
  /*ierr = VecScatterDestroy(da->ltog);CHKERRQ(ierr);da->ltog = 0; */
  /*ierr = VecScatterDestroy(da->gtol);CHKERRQ(ierr);da->gtol = 0;*/

  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "SDACreate3d"
/*@C
    SDACreate3d - Creates a three-dimensional regular array that is
    distributed across some processors. This is the simplified 
    interface, must be used with SDAXXX operations, NOT DAXXX operations.

   Input Parameters:
.  comm - MPI communicator
.  wrap - type of periodicity should the array have, if any
$         DA_NONPERIODIC, DA_XPERIODIC, 
$         DA_YPERIODIC, DA_XYPERIODIC  
.  stencil_type - stencil type either DA_STENCIL_BOX or DA_STENCIL_STAR
.  M,N - global dimension in each direction of the array
.  m,n - corresponding number of processors in each dimension 
         (or PETSC_DECIDE to have calculated)
.  w - number of degress of freedom per node
.  s - stencil width
.  lx, ly, lz - arrays containing the number of nodes in each cell along
$           the x, y, and z coordinates, or PETSC_NUL

   Output Parameter:
.  inra - the resulting array object

  Level: beginner

.keywords: distributed array, create, two-dimensional

.seealso: DADestroy(), DAView(), DACreate1d(), DACreate3d()
@*/
int SDACreate3d(MPI_Comm comm,DAPeriodicType wrap,DAStencilType stencil_type,int M,
                int N,int P,int m,int n,int p,int w,int s,int *lx,int *ly,int *lz,SDA *sda)
{
  int           ierr;
  DA            da;
  Vec           tmp;
  char          **args;
  int           argc = 0;

  PetscInitialize(&argc,&args,0,0);
  PetscFunctionBegin;

  ierr = PetscNew(struct _SDA,sda);CHKERRQ(ierr);
  ierr = DACreate3d(comm,wrap,stencil_type,M,N,P,m,n,p,w,s,lx,ly,lz,&da);CHKERRQ(ierr);
  (*sda)->da = da;

  /* set up two dummy work vectors for the vector scatter */
  ierr = DACreateLocalVector(da,&(*sda)->gvec);CHKERRQ(ierr);
  /* we free the actual space in the vectors because it is not 
     needed since the user provides her/his own with SDA */
  ierr = VecReplaceArray((*sda)->gvec,PETSC_NULL);CHKERRQ(ierr);
  ierr = VecDuplicate((*sda)->gvec,&(*sda)->lvec);CHKERRQ(ierr);
  ierr = VecReplaceArray((*sda)->lvec,PETSC_NULL);CHKERRQ(ierr);

  /* destroy global vector */
  ierr = DACreateGlobalVector(da,&tmp);CHKERRQ(ierr);
  ierr = VecDestroy(tmp);CHKERRQ(ierr);
  (*sda)->Gvec = 0;
  /* free scatters in DA never needed by user */
  ierr = DALocalToLocalCreate(da);CHKERRQ(ierr);
  /*ierr = VecScatterDestroy(da->ltog);CHKERRQ(ierr);da->ltog = 0;*/
  /*ierr = VecScatterDestroy(da->gtol);CHKERRQ(ierr);da->gtol = 0;*/

  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "SDADestroy"
/*@C
  SDADestroy - Destroys simple distributed array.

  Input parameters:
    sda - distributed array

  Level: beginner
@*/
int SDADestroy(SDA sda)
{
  int ierr;

  PetscFunctionBegin;
  ierr = VecDestroy(sda->gvec);CHKERRQ(ierr);
  ierr = VecDestroy(sda->lvec);CHKERRQ(ierr); 
  if (sda->Gvec) {ierr = VecDestroy(sda->Gvec);CHKERRQ(ierr);}
  ierr = DADestroy(sda->da);CHKERRQ(ierr);
  ierr = PetscFree(sda);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "SDALocalToLocalBegin"
/*@C
   SDALocalToLocalBegin - Maps from a local representation (including 
       ghostpoints) to another where the ghostpoints in the second are
       set correctly. Must be followed by SDALocalToLocalEnd().

   Input Parameters:
.  da - the distributed array context
.  g - the original vector
.  mode - one of INSERT_VALUES or ADD_VALUES

   Output Parameter:
.  l  - the vector with correct ghost values

  Level: intermediate

.keywords: distributed array, global to local, begin

.seealso: SDALocalToLocalEnd(), SDACreate2d()
@*/
int SDALocalToLocalBegin(SDA sda,PetscScalar *g,InsertMode mode,PetscScalar *l)
{
  int ierr;
  DA  da = sda->da;
  Vec gvec = sda->gvec,lvec = sda->lvec;

  PetscFunctionBegin;
  ierr = VecPlaceArray(gvec,g);CHKERRQ(ierr);
  ierr = VecPlaceArray(lvec,l);CHKERRQ(ierr);
  ierr = DALocalToLocalBegin(da,gvec,mode,lvec);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "SDALocalToLocalEnd"
/*@C
   SDALocalToLocalEnd - Maps from a local representation (including 
       ghostpoints) to another where the ghostpoints in the second are
       set correctly. Must be preceeded by SDALocalToLocalBegin().

   Input Parameters:
.  da - the distributed array context
.  g - the original vector
.  mode - one of INSERT_VALUES or ADD_VALUES

   Output Parameter:
.  l  - the vector with correct ghost values

  Level: intermediate

.keywords: distributed array, global to local, end

.seealso: SDALocalToLocalBegin(), SDACreate2d()
@*/
int SDALocalToLocalEnd(SDA sda,PetscScalar *g,InsertMode mode,PetscScalar *l)
{
  int ierr;
  DA  da = sda->da;
  Vec gvec = sda->gvec,lvec = sda->lvec;

  PetscFunctionBegin;
  ierr = VecPlaceArray(gvec,g);CHKERRQ(ierr);
  ierr = VecPlaceArray(lvec,l);CHKERRQ(ierr);
  ierr = DALocalToLocalEnd(da,gvec,mode,lvec);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
 
#undef __FUNCT__  
#define __FUNCT__ "SDAGetCorners"
/*@C
   SDAGetCorners - Returns the global (x,y,z) indices of the lower left
   corner of the local region, excluding ghost points.

   Input Parameter:
.  da - the distributed array

   Output Parameters:
.  x,y,z - the corner indices
$    y and z are optional (used for 2D and 3D problems)
.  m,n,p - widths in the corresponding directions
$    n and p are optional (used for 2D and 3D problems)

   Note:
   Any of y, z, n, and p should be set to PETSC_NULL if not needed.

  Level: intermediate

.keywords: distributed array, get, corners, nodes, local indices

.seealso: SDAGetGhostCorners()
@*/
int SDAGetCorners(SDA da,int *x,int *y,int *z,int *m,int *n,int *p)
{
  int ierr;

  PetscFunctionBegin;
  ierr = DAGetCorners(da->da,x,y,z,m,n,p);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__  
#define __FUNCT__ "SDAGetGhostCorners"
/*@C
    SDAGetGhostCorners - Returns the global (x,y,z) indices of the lower left
    corner of the local region, including ghost points.

   Input Parameter:
.  da - the distributed array

   Output Parameters:
.  x,y,z - the corner indices
$    y and z are optional (used for 2D and 3D problems)
.  m,n,p - widths in the corresponding directions
$    n and p are optional (used for 2D and 3D problems)

   Note:
   Any of y, z, n, and p should be set to PETSC_NULL if not needed.

  Level: intermediate

.keywords: distributed array, get, ghost, corners, nodes, local indices

.seealso: SDAGetCorners()
@*/
int SDAGetGhostCorners(SDA da,int *x,int *y,int *z,int *m,int *n,int *p)
{
  int ierr;

  PetscFunctionBegin;
  ierr = DAGetGhostCorners(da->da,x,y,z,m,n,p);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


