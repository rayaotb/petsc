
/* $Id: pdvec.c,v 1.94 1998/05/11 21:50:26 bsmith Exp bsmith $ */

/*
     Code for some of the parallel vector primatives.
*/

#include "pinclude/pviewer.h"
#include "src/vec/impls/mpi/pvecimpl.h"   /*I  "vec.h"   I*/


#undef __FUNC__  
#define __FUNC__ "VecGetOwnershipRange_MPI"
int VecGetOwnershipRange_MPI(Vec v,int *low,int* high) 
{
  Vec_MPI *x = (Vec_MPI *) v->data;

  PetscFunctionBegin;
  *low  = x->ownership[x->rank];
  *high = x->ownership[x->rank+1];
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "VecDestroy_MPI"
int VecDestroy_MPI(Vec v)
{
  Vec_MPI *x = (Vec_MPI *) v->data;
  int     ierr;

  PetscFunctionBegin;
#if defined(USE_PETSC_LOG)
  PLogObjectState((PetscObject)v,"Length=%d",x->N);
#endif  
  if (x->stash.array) PetscFree(x->stash.array);
  if (x->array_allocated) PetscFree(x->array_allocated);
  if (v->mapping) {
    ierr = ISLocalToGlobalMappingDestroy(v->mapping); CHKERRQ(ierr);
  }
  /* Destroy local representation of vector if it exists */
  if (x->localrep) {
    ierr = VecDestroy(x->localrep); CHKERRQ(ierr);
    ierr = VecScatterDestroy(x->localupdate);CHKERRQ(ierr);
  }
  PetscFree(v->data);
  PLogObjectDestroy(v);
  PetscHeaderDestroy(v);
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "VecView_MPI_File"
int VecView_MPI_File(Vec xin, Viewer ptr )
{
  Vec_MPI *x = (Vec_MPI *) xin->data;
  int     i, rank, ierr, format;
  FILE    *fd;

  PetscFunctionBegin;
  ierr = ViewerASCIIGetPointer(ptr,&fd); CHKERRQ(ierr);

  MPI_Comm_rank(xin->comm,&rank); 
  PetscSequentialPhaseBegin(xin->comm,1);

  ierr = ViewerGetFormat(ptr,&format); CHKERRQ(ierr);
  if (format != VIEWER_FORMAT_ASCII_COMMON) fprintf(fd,"Processor [%d] \n",rank);
  for ( i=0; i<x->n; i++ ) {
#if defined(USE_PETSC_COMPLEX)
    if (imag(x->array[i]) > 0.0) {
      fprintf(fd,"%g + %g i\n",real(x->array[i]),imag(x->array[i]));
    } else if (imag(x->array[i]) < 0.0) {
      fprintf(fd,"%g - %g i\n",real(x->array[i]),-imag(x->array[i]));
    } else {
      fprintf(fd,"%g\n",real(x->array[i]));
    }
#else
    fprintf(fd,"%g\n",x->array[i]);
#endif
  }
  fflush(fd);
  PetscSequentialPhaseEnd(xin->comm,1);
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "VecView_MPI_Files"
int VecView_MPI_Files(Vec xin, Viewer viewer )
{
  Vec_MPI     *x = (Vec_MPI *) xin->data;
  int         i,rank,len, work = x->n,n,j,size,ierr,format,cnt;
  MPI_Status  status;
  FILE        *fd;
  Scalar      *values;
  char        *outputname;

  PetscFunctionBegin;
  ierr = ViewerASCIIGetPointer(viewer,&fd); CHKERRQ(ierr);
  /* determine maximum message to arrive */
  MPI_Comm_rank(xin->comm,&rank);
  ierr = MPI_Reduce(&work,&len,1,MPI_INT,MPI_MAX,0,xin->comm);CHKERRQ(ierr);
  MPI_Comm_size(xin->comm,&size);

  if (!rank) {
    values = (Scalar *) PetscMalloc( (len+1)*sizeof(Scalar) ); CHKPTRQ(values);
    ierr = ViewerGetFormat(viewer,&format); CHKERRQ(ierr);
    /*
        Matlab format and ASCI format are very similar except 
        Matlab uses %18.16e format while ASCII uses %g
    */
    if (format == VIEWER_FORMAT_ASCII_MATLAB) {
      ierr = ViewerFileGetOutputname_Private(viewer,&outputname); CHKERRQ(ierr);
      fprintf(fd,"%s = [\n",outputname);
      for ( i=0; i<x->n; i++ ) {
#if defined(USE_PETSC_COMPLEX)
        if (imag(x->array[i]) > 0.0) {
          fprintf(fd,"%18.16e + %18.16e i\n",real(x->array[i]),imag(x->array[i]));
        } else if (imag(x->array[i]) < 0.0) {
          fprintf(fd,"%18.16e - %18.16e i\n",real(x->array[i]),-imag(x->array[i]));
        } else {
          fprintf(fd,"%18.16e\n",real(x->array[i]));
        }
#else
        fprintf(fd,"%18.16e\n",x->array[i]);
#endif
      }
      /* receive and print messages */
      for ( j=1; j<size; j++ ) {
        ierr = MPI_Recv(values,len,MPIU_SCALAR,j,47,xin->comm,&status);CHKERRQ(ierr);
        ierr = MPI_Get_count(&status,MPIU_SCALAR,&n); CHKERRQ(ierr);         
        for ( i=0; i<n; i++ ) {
#if defined(USE_PETSC_COMPLEX)
          if (imag(values[i]) > 0.0) {
            fprintf(fd,"%18.16e + %18.16e i\n",real(values[i]),imag(values[i]));
          } else if (imag(values[i]) < 0.0) {
            fprintf(fd,"%18.16e - %18.16e i\n",real(values[i]),-imag(values[i]));
          } else {
            fprintf(fd,"%18.16e\n",real(values[i]));
          }
#else
          fprintf(fd,"%18.16e\n",values[i]);
#endif
        }
      }          
      fprintf(fd,"];\n");
    } else {
      if (format != VIEWER_FORMAT_ASCII_COMMON) fprintf(fd,"Processor [%d]\n",rank);
      cnt = 0;
      for ( i=0; i<x->n; i++ ) {
        if (format == VIEWER_FORMAT_ASCII_INDEX) {
          fprintf(fd,"%d: ",cnt++);
        }
#if defined(USE_PETSC_COMPLEX)
        if (imag(x->array[i]) > 0.0) {
          fprintf(fd,"%g + %g i\n",real(x->array[i]),imag(x->array[i]));
        } else if (imag(x->array[i]) < 0.0) {
          fprintf(fd,"%g - %g i\n",real(x->array[i]),-imag(x->array[i]));
        } else {
          fprintf(fd,"%g\n",real(x->array[i]));
        }
#else
        fprintf(fd,"%g\n",x->array[i]);
#endif
      }
      /* receive and print messages */
      for ( j=1; j<size; j++ ) {
        ierr = MPI_Recv(values,len,MPIU_SCALAR,j,47,xin->comm,&status);CHKERRQ(ierr);
        ierr = MPI_Get_count(&status,MPIU_SCALAR,&n);  CHKERRQ(ierr);        
        if (format != VIEWER_FORMAT_ASCII_COMMON) {
          fprintf(fd,"Processor [%d]\n",j);
        }
        for ( i=0; i<n; i++ ) {
          if (format == VIEWER_FORMAT_ASCII_INDEX) {
            fprintf(fd,"%d: ",cnt++);
          }
#if defined(USE_PETSC_COMPLEX)
          if (imag(values[i]) > 0.0) {
            fprintf(fd,"%g + %g i\n",real(values[i]),imag(values[i]));
          } else if (imag(values[i]) < 0.0) {
            fprintf(fd,"%g - %g i\n",real(values[i]),-imag(values[i]));
          } else {
            fprintf(fd,"%g\n",real(values[i]));
          }
#else
          fprintf(fd,"%g\n",values[i]);
#endif
        }          
      }
    }
    PetscFree(values);
    fflush(fd);
  } else {
    /* send values */
    ierr = MPI_Send(x->array,x->n,MPIU_SCALAR,0,47,xin->comm);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "VecView_MPI_Binary"
int VecView_MPI_Binary(Vec xin, Viewer viewer )
{
  Vec_MPI     *x = (Vec_MPI *) xin->data;
  int         rank,ierr,len, work = x->n,n,j,size, fdes;
  MPI_Status  status;
  Scalar      *values;

  PetscFunctionBegin;
  ierr = ViewerBinaryGetDescriptor(viewer,&fdes); CHKERRQ(ierr);

  /* determine maximum message to arrive */
  MPI_Comm_rank(xin->comm,&rank);
  ierr = MPI_Reduce(&work,&len,1,MPI_INT,MPI_MAX,0,xin->comm);CHKERRQ(ierr);
  MPI_Comm_size(xin->comm,&size);

  if (!rank) {
    ierr = PetscBinaryWrite(fdes,&xin->cookie,1,PETSC_INT,0); CHKERRQ(ierr);
    ierr = PetscBinaryWrite(fdes,&x->N,1,PETSC_INT,0); CHKERRQ(ierr);
    ierr = PetscBinaryWrite(fdes,x->array,x->n,PETSC_SCALAR,0); CHKERRQ(ierr);

    values = (Scalar *) PetscMalloc( (len+1)*sizeof(Scalar) ); CHKPTRQ(values);
    /* receive and print messages */
    for ( j=1; j<size; j++ ) {
      ierr = MPI_Recv(values,len,MPIU_SCALAR,j,47,xin->comm,&status);CHKERRQ(ierr);
      ierr = MPI_Get_count(&status,MPIU_SCALAR,&n); CHKERRQ(ierr);         
      ierr = PetscBinaryWrite(fdes,values,n,PETSC_SCALAR,0); 
    }
    PetscFree(values);
  } else {
    /* send values */
    ierr = MPI_Send(x->array,x->n,MPIU_SCALAR,0,47,xin->comm);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "VecView_MPI_Draw_LG"
int VecView_MPI_Draw_LG(Vec xin,Viewer v  )
{
  Vec_MPI     *x = (Vec_MPI *) xin->data;
  int         i,rank,size, N = x->N,*lens,ierr;
  Draw        draw;
  double      *xx,*yy;
  DrawLG      lg;

  PetscFunctionBegin;
  ierr = ViewerDrawGetDrawLG(v,&lg); CHKERRQ(ierr);
  ierr = DrawLGGetDraw(lg,&draw); CHKERRQ(ierr);
  ierr = DrawCheckResizedWindow(draw);CHKERRQ(ierr);
  MPI_Comm_rank(xin->comm,&rank);
  MPI_Comm_size(xin->comm,&size);
  if (!rank) {
    DrawLGReset(lg);
    xx   = (double *) PetscMalloc( 2*(N+1)*sizeof(double) ); CHKPTRQ(xx);
    for ( i=0; i<N; i++ ) {xx[i] = (double) i;}
    yy   = xx + N;
    lens = (int *) PetscMalloc(size*sizeof(int)); CHKPTRQ(lens);
    for (i=0; i<size; i++ ) {
      lens[i] = x->ownership[i+1] - x->ownership[i];
    }
#if !defined(USE_PETSC_COMPLEX)
    ierr = MPI_Gatherv(x->array,x->n,MPI_DOUBLE,yy,lens,x->ownership,MPI_DOUBLE,0,xin->comm);CHKERRQ(ierr);
#else
    {
      double *xr;
      xr = (double *) PetscMalloc( (x->n+1)*sizeof(double) ); CHKPTRQ(xr);
      for ( i=0; i<x->n; i++ ) {
        xr[i] = real(x->array[i]);
      }
      ierr = MPI_Gatherv(xr,x->n,MPI_DOUBLE,yy,lens,x->ownership,MPI_DOUBLE,0,xin->comm);CHKERRQ(ierr);
      PetscFree(xr);
    }
#endif
    PetscFree(lens);
    ierr = DrawLGAddPoints(lg,N,&xx,&yy);CHKERRQ(ierr);
    PetscFree(xx);
    ierr = DrawLGDraw(lg);CHKERRQ(ierr);
  } else {
#if !defined(USE_PETSC_COMPLEX)
    ierr = MPI_Gatherv(x->array,x->n,MPI_DOUBLE,0,0,0,MPI_DOUBLE,0,xin->comm);CHKERRQ(ierr);
#else
    {
      double *xr;
      xr = (double *) PetscMalloc( (x->n+1)*sizeof(double) ); CHKPTRQ(xr);
      for ( i=0; i<x->n; i++ ) {
        xr[i] = real(x->array[i]);
      }
      ierr = MPI_Gatherv(xr,x->n,MPI_DOUBLE,0,0,0,MPI_DOUBLE,0,xin->comm);CHKERRQ(ierr);
      PetscFree(xr);
    }
#endif
  }
  ierr = DrawSynchronizedFlush(draw);CHKERRQ(ierr);
  DrawPause(draw);
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "VecView_MPI_Draw"
int VecView_MPI_Draw(Vec xin, Viewer v )
{
  Vec_MPI     *x = (Vec_MPI *) xin->data;
  int         i,rank,size,ierr,start,end,format;
  MPI_Status  status;
  double      coors[4],ymin,ymax,xmin,xmax,tmp;
  Draw        draw;
  PetscTruth  isnull;
  DrawAxis    axis;

  PetscFunctionBegin;
  ierr = ViewerDrawGetDraw(v,&draw);CHKERRQ(ierr);
  ierr = DrawIsNull(draw,&isnull); CHKERRQ(ierr); if (isnull) PetscFunctionReturn(0);

  ierr = ViewerGetFormat(v,&format); CHKERRQ(ierr);
  if (format == VIEWER_FORMAT_DRAW_LG) {
    ierr = VecView_MPI_Draw_LG(xin, v ); CHKERRQ(ierr);
    PetscFunctionReturn(0);
  }

  ierr = DrawCheckResizedWindow(draw);CHKERRQ(ierr);
  xmin = 1.e20; xmax = -1.e20;
  for ( i=0; i<x->n; i++ ) {
#if defined(USE_PETSC_COMPLEX)
    if (real(x->array[i]) < xmin) xmin = real(x->array[i]);
    if (real(x->array[i]) > xmax) xmax = real(x->array[i]);
#else
    if (x->array[i] < xmin) xmin = x->array[i];
    if (x->array[i] > xmax) xmax = x->array[i];
#endif
  }
  if (xmin + 1.e-10 > xmax) {
    xmin -= 1.e-5;
    xmax += 1.e-5;
  }
  ierr = MPI_Reduce(&xmin,&ymin,1,MPI_DOUBLE,MPI_MIN,0,xin->comm);CHKERRQ(ierr);
  ierr = MPI_Reduce(&xmax,&ymax,1,MPI_DOUBLE,MPI_MAX,0,xin->comm);CHKERRQ(ierr);
  MPI_Comm_size(xin->comm,&size); 
  MPI_Comm_rank(xin->comm,&rank);
  ierr = DrawAxisCreate(draw,&axis); CHKERRQ(ierr);
  PLogObjectParent(draw,axis);
  if (!rank) {
    DrawClear(draw); DrawFlush(draw);
    ierr = DrawAxisSetLimits(axis,0.0,(double) x->N,ymin,ymax); CHKERRQ(ierr);
    ierr = DrawAxisDraw(axis); CHKERRQ(ierr);
    ierr = DrawGetCoordinates(draw,coors,coors+1,coors+2,coors+3);CHKERRQ(ierr);
  }
  ierr = DrawAxisDestroy(axis);CHKERRQ(ierr);
  ierr = MPI_Bcast(coors,4,MPI_DOUBLE,0,xin->comm);CHKERRQ(ierr);
  if (rank) {ierr = DrawSetCoordinates(draw,coors[0],coors[1],coors[2],coors[3]);CHKERRQ(ierr);}
  /* draw local part of vector */
  ierr = VecGetOwnershipRange(xin,&start,&end);CHKERRQ(ierr);
  if (rank < size-1) { /*send value to right */
    ierr = MPI_Send(&x->array[x->n-1],1,MPI_DOUBLE,rank+1,xin->tag,xin->comm);CHKERRQ(ierr);
  }
  for ( i=1; i<x->n; i++ ) {
#if !defined(USE_PETSC_COMPLEX)
    ierr = DrawLine(draw,(double)(i-1+start),x->array[i-1],(double)(i+start),
                   x->array[i],DRAW_RED);CHKERRQ(ierr);
#else
    ierr = DrawLine(draw,(double)(i-1+start),real(x->array[i-1]),(double)(i+start),
                   real(x->array[i]),DRAW_RED);CHKERRQ(ierr);
#endif
  }
  if (rank) { /* receive value from right */
    ierr = MPI_Recv(&tmp,1,MPI_DOUBLE,rank-1,xin->tag,xin->comm,&status);CHKERRQ(ierr);
#if !defined(USE_PETSC_COMPLEX)
    ierr = DrawLine(draw,(double)start-1,tmp,(double)start,x->array[0],DRAW_RED);CHKERRQ(ierr);
#else
    ierr = DrawLine(draw,(double)start-1,tmp,(double)start,real(x->array[0]),DRAW_RED);CHKERRQ(ierr);
#endif
  }
  ierr = DrawSynchronizedFlush(draw); CHKERRQ(ierr);
  ierr = DrawPause(draw); CHKERRQ(ierr);
  PetscFunctionReturn(0);
}



#undef __FUNC__  
#define __FUNC__ "VecView_MPI_Matlab"
int VecView_MPI_Matlab(Vec xin, Viewer viewer )
{
#if !defined(USE_PETSC_COMPLEX)
  Vec_MPI     *x = (Vec_MPI *) xin->data;
  int         i,rank,size, N = x->N,*lens,ierr;
  double      *xx;
#endif

  PetscFunctionBegin;
#if defined(USE_PETSC_COMPLEX)
  SETERRQ(PETSC_ERR_SUP,0,"Complex not done");
#else
  MPI_Comm_rank(xin->comm,&rank);
  MPI_Comm_size(xin->comm,&size);
  if (!rank) {
    xx = (double *) PetscMalloc( (N+1)*sizeof(double) ); CHKPTRQ(xx);
    lens = (int *) PetscMalloc(size*sizeof(int)); CHKPTRQ(lens);
    for (i=0; i<size; i++ ) {
      lens[i] = x->ownership[i+1] - x->ownership[i];
    }
    ierr = MPI_Gatherv(x->array,x->n,MPI_DOUBLE,xx,lens,x->ownership,MPI_DOUBLE,0,xin->comm);CHKERRQ(ierr);
    PetscFree(lens);
    ierr = ViewerMatlabPutScalar_Private(viewer,N,1,xx);CHKERRQ(ierr);
    PetscFree(xx);
  } else {
    ierr = MPI_Gatherv(x->array,x->n,MPI_DOUBLE,0,0,0,MPI_DOUBLE,0,xin->comm);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
#endif
}

#undef __FUNC__  
#define __FUNC__ "VecView_MPI"
int VecView_MPI(Vec xin,Viewer viewer)
{
  ViewerType  vtype;
  int         ierr;

  PetscFunctionBegin;
  ierr = ViewerGetType(viewer,&vtype);CHKERRQ(ierr);
  if (vtype == ASCII_FILE_VIEWER){
    ierr = VecView_MPI_File(xin,viewer);CHKERRQ(ierr);
  } else if (vtype == ASCII_FILES_VIEWER){
    ierr = VecView_MPI_Files(xin,viewer);CHKERRQ(ierr);
  } else if (vtype == MATLAB_VIEWER) {
    ierr = VecView_MPI_Matlab(xin,viewer);CHKERRQ(ierr);
  } else if (vtype == BINARY_FILE_VIEWER) {
    ierr = VecView_MPI_Binary(xin,viewer);CHKERRQ(ierr);
  } else if (vtype == DRAW_VIEWER) {
    ierr = VecView_MPI_Draw(xin,viewer);CHKERRQ(ierr);
  } else {
    SETERRQ(1,1,"Viewer type not supported for this object");
  }
  PetscFunctionReturn(0);
}

int VecGetSize_MPI(Vec xin,int *N)
{
  Vec_MPI  *x = (Vec_MPI *)xin->data;

  PetscFunctionBegin;
  *N = x->N;
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "VecSetValues_MPI"
int VecSetValues_MPI(Vec xin, int ni, int *ix, Scalar* y,InsertMode addv)
{
  Vec_MPI  *x = (Vec_MPI *)xin->data;
  int      rank = x->rank, *owners = x->ownership, start = owners[rank];
  int      end = owners[rank+1], i, row;
  Scalar   *xx = x->array;

  PetscFunctionBegin;
#if defined(USE_PETSC_BOPT_g)
  if (x->insertmode == INSERT_VALUES && addv == ADD_VALUES) { 
   SETERRQ(PETSC_ERR_ARG_WRONGSTATE,0,"You have already inserted values; you cannot now add");
  }
  else if (x->insertmode == ADD_VALUES && addv == INSERT_VALUES) { 
   SETERRQ(PETSC_ERR_ARG_WRONGSTATE,0,"You have already added values; you cannot now insert");
  }
#endif
  x->insertmode = addv;

  if (addv == INSERT_VALUES) {
    for ( i=0; i<ni; i++ ) {
      if ( (row = ix[i]) >= start && row < end) {
        xx[row-start] = y[i];
      }
      else if (!x->stash.donotstash) {
#if defined(USE_PETSC_BOPT_g)
        if (ix[i] < 0 || ix[i] >= x->N) SETERRQ(PETSC_ERR_ARG_OUTOFRANGE,0,"Out of range");
#endif
        if (x->stash.n == x->stash.nmax) { /* cache is full */
          int    *idx, nmax = x->stash.nmax;
          Scalar *ta;
          ta = (Scalar *) PetscMalloc((nmax+200)*sizeof(Scalar)+(nmax+200)*sizeof(int));CHKPTRQ(ta);
          PLogObjectMemory(xin,200*(sizeof(Scalar) + sizeof(int)));
          idx = (int *) (ta + nmax + 200);
          PetscMemcpy(ta,x->stash.array,nmax*sizeof(Scalar));
          PetscMemcpy(idx,x->stash.idx,nmax*sizeof(int));
          if (x->stash.array) PetscFree(x->stash.array);
          x->stash.array = ta;
          x->stash.idx   = idx;
          x->stash.nmax += 200;
        }
        x->stash.array[x->stash.n] = y[i];
        x->stash.idx[x->stash.n++] = row;
      }
    }
  } else {
    for ( i=0; i<ni; i++ ) {
      if ( (row = ix[i]) >= start && row < end) {
        xx[row-start] += y[i];
      }
      else if (!x->stash.donotstash) {
#if defined(USE_PETSC_BOPT_g)
        if (ix[i] < 0 || ix[i] > x->N) SETERRQ(PETSC_ERR_ARG_OUTOFRANGE,0,"Out of range");
#endif
        if (x->stash.n == x->stash.nmax) { /* cache is full */
          int    *idx, nmax = x->stash.nmax;
          Scalar *ta;
          ta = (Scalar *) PetscMalloc((nmax+200)*sizeof(Scalar)+(nmax+200)*sizeof(int));CHKPTRQ(ta);
          PLogObjectMemory(xin,200*(sizeof(Scalar) + sizeof(int)));
          idx = (int *) (ta + nmax + 200);
          PetscMemcpy(ta,x->stash.array,nmax*sizeof(Scalar));
          PetscMemcpy(idx,x->stash.idx,nmax*sizeof(int));
          if (x->stash.array) PetscFree(x->stash.array);
          x->stash.array = ta;
          x->stash.idx   = idx;
          x->stash.nmax += 200;
        }
        x->stash.array[x->stash.n] = y[i];
        x->stash.idx[x->stash.n++] = row;
      }
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "VecSetValuesBlocked_MPI"
int VecSetValuesBlocked_MPI(Vec xin, int ni, int *ix, Scalar* y,InsertMode addv)
{
  Vec_MPI  *x = (Vec_MPI *)xin->data;
  int      rank = x->rank, *owners = x->ownership, start = owners[rank];
  int      end = owners[rank+1], i, row,bs = xin->bs,j;
  Scalar   *xx = x->array;

  PetscFunctionBegin;
#if defined(USE_PETSC_BOPT_g)
  if (x->insertmode == INSERT_VALUES && addv == ADD_VALUES) { 
   SETERRQ(PETSC_ERR_ARG_WRONGSTATE,0,"You have already inserted values; you cannot now add");
  }
  else if (x->insertmode == ADD_VALUES && addv == INSERT_VALUES) { 
   SETERRQ(PETSC_ERR_ARG_WRONGSTATE,0,"You have already added values; you cannot now insert");
  }
#endif
  x->insertmode = addv;

  if (addv == INSERT_VALUES) {
    for ( i=0; i<ni; i++ ) {
      if ( (row = bs*ix[i]) >= start && row < end) {
        for ( j=0; j<bs; j++ ) {
          xx[row-start+j] = y[j];
        }
      }
      else if (!x->stash.donotstash) {
#if defined(USE_PETSC_BOPT_g)
        if (ix[i] < 0 || ix[i] >= x->N) SETERRQ(PETSC_ERR_ARG_OUTOFRANGE,0,"Out of range");
#endif
        if (x->stash.n == x->stash.nmax) { /* cache is full */
          int    *idx, nmax = x->stash.nmax;
          Scalar *ta;
          ta = (Scalar *) PetscMalloc((nmax+200)*sizeof(Scalar)+(nmax+200)*sizeof(int));CHKPTRQ(ta);
          PLogObjectMemory(xin,200*(sizeof(Scalar) + sizeof(int)));
          idx = (int *) (ta + nmax + 200);
          PetscMemcpy(ta,x->stash.array,nmax*sizeof(Scalar));
          PetscMemcpy(idx,x->stash.idx,nmax*sizeof(int));
          if (x->stash.array) PetscFree(x->stash.array);
          x->stash.array = ta;
          x->stash.idx   = idx;
          x->stash.nmax += 200;
        }
        for ( j=0; j<bs; j++ ) {
          x->stash.array[x->stash.n + j] = y[j];
          x->stash.idx[x->stash.n + j]   = row + j;
        }
        x->stash.n += bs;
      }
      y += bs;
    }
  } else {
    for ( i=0; i<ni; i++ ) {
      if ( (row = bs*ix[i]) >= start && row < end) {
        for ( j=0; j<bs; j++ ) {
          xx[row-start+j] += y[j];
        }
      }
      else if (!x->stash.donotstash) {
#if defined(USE_PETSC_BOPT_g)
        if (ix[i] < 0 || ix[i] > x->N) SETERRQ(PETSC_ERR_ARG_OUTOFRANGE,0,"Out of range");
#endif
        if (x->stash.n == x->stash.nmax) { /* cache is full */
          int    *idx, nmax = x->stash.nmax;
          Scalar *ta;
          ta = (Scalar *) PetscMalloc((nmax+200)*sizeof(Scalar)+(nmax+200)*sizeof(int));CHKPTRQ(ta);
          PLogObjectMemory(xin,200*(sizeof(Scalar) + sizeof(int)));
          idx = (int *) (ta + nmax + 200);
          PetscMemcpy(ta,x->stash.array,nmax*sizeof(Scalar));
          PetscMemcpy(idx,x->stash.idx,nmax*sizeof(int));
          if (x->stash.array) PetscFree(x->stash.array);
          x->stash.array = ta;
          x->stash.idx   = idx;
          x->stash.nmax += 200;
        }
        for ( j=0; j<bs; j++ ) {
          x->stash.array[x->stash.n + j] = y[j];
          x->stash.idx[x->stash.n + j]   = row + j;
        }
        x->stash.n += bs;
      }
      y += bs;
    }
  }
  PetscFunctionReturn(0);
}

/*
   Since nsends or nreceives may be zero we add 1 in certain mallocs
to make sure we never malloc an empty one.      
*/
#undef __FUNC__  
#define __FUNC__ "VecAssemblyBegin_MPI"
int VecAssemblyBegin_MPI(Vec xin)
{
  Vec_MPI    *x = (Vec_MPI *)xin->data;
  int         rank = x->rank, *owners = x->ownership, size = x->size;
  int         *nprocs,i,j,idx,*procs,nsends,nreceives,nmax,*work;
  int         *owner,*starts,count,tag = xin->tag,ierr;
  InsertMode  addv;
  Scalar      *rvalues,*svalues;
  MPI_Comm    comm = xin->comm;
  MPI_Request *send_waits,*recv_waits;

  PetscFunctionBegin;
  if (x->stash.donotstash) {
    PetscFunctionReturn(0);
  }

  ierr = MPI_Allreduce(&x->insertmode,&addv,1,MPI_INT,MPI_BOR,comm);CHKERRQ(ierr);
  if (addv == (ADD_VALUES|INSERT_VALUES)) { 
    SETERRQ(PETSC_ERR_ARG_NOTSAMETYPE,0,"Some processors inserted values while others added");
  }
  x->insertmode = addv; /* in case this processor had no cache */

  /*  first count number of contributors to each processor */
  nprocs = (int *) PetscMalloc( 2*size*sizeof(int) ); CHKPTRQ(nprocs);
  PetscMemzero(nprocs,2*size*sizeof(int)); procs = nprocs + size;
  owner = (int *) PetscMalloc( (x->stash.n+1)*sizeof(int) ); CHKPTRQ(owner);
  for ( i=0; i<x->stash.n; i++ ) {
    idx = x->stash.idx[i];
    for ( j=0; j<size; j++ ) {
      if (idx >= owners[j] && idx < owners[j+1]) {
        nprocs[j]++; procs[j] = 1; owner[i] = j; break;
      }
    }
  }
  nsends = 0;  for ( i=0; i<size; i++ ) { nsends += procs[i];} 

  /* inform other processors of number of messages and max length*/
  work = (int *) PetscMalloc( size*sizeof(int) ); CHKPTRQ(work);
  ierr = MPI_Allreduce(procs, work,size,MPI_INT,MPI_SUM,comm);CHKERRQ(ierr);
  nreceives = work[rank]; 
  ierr = MPI_Allreduce(nprocs,work,size,MPI_INT,MPI_MAX,comm);CHKERRQ(ierr);
  nmax = work[rank];
  PetscFree(work);

  /* post receives: 
       1) each message will consist of ordered pairs 
     (global index,value) we store the global index as a double 
     to simply the message passing. 
       2) since we don't know how long each individual message is we 
     allocate the largest needed buffer for each receive. Potentially 
     this is a lot of wasted space.
       This could be done better.
  */
  rvalues = (Scalar *) PetscMalloc(2*(nreceives+1)*(nmax+1)*sizeof(Scalar));CHKPTRQ(rvalues);
  recv_waits = (MPI_Request *) PetscMalloc((nreceives+1)*sizeof(MPI_Request));CHKPTRQ(recv_waits);
  for ( i=0; i<nreceives; i++ ) {
    ierr = MPI_Irecv(rvalues+2*nmax*i,2*nmax,MPIU_SCALAR,MPI_ANY_SOURCE,tag,
                     comm,recv_waits+i);CHKERRQ(ierr);
  }

  /* do sends:
      1) starts[i] gives the starting index in svalues for stuff going to 
         the ith processor
  */
  svalues = (Scalar *) PetscMalloc(2*(x->stash.n+1)*sizeof(Scalar));CHKPTRQ(svalues);
  send_waits = (MPI_Request *) PetscMalloc( (nsends+1)*sizeof(MPI_Request));CHKPTRQ(send_waits);
  starts = (int *) PetscMalloc( size*sizeof(int) ); CHKPTRQ(starts);
  starts[0] = 0; 
  for ( i=1; i<size; i++ ) { starts[i] = starts[i-1] + nprocs[i-1];} 
  for ( i=0; i<x->stash.n; i++ ) {
    svalues[2*starts[owner[i]]]       = (Scalar)  x->stash.idx[i];
    svalues[2*(starts[owner[i]]++)+1] =  x->stash.array[i];
  }
  PetscFree(owner);
  starts[0] = 0;
  for ( i=1; i<size; i++ ) { starts[i] = starts[i-1] + nprocs[i-1];} 
  count = 0;
  for ( i=0; i<size; i++ ) {
    if (procs[i]) {
      ierr = MPI_Isend(svalues+2*starts[i],2*nprocs[i],MPIU_SCALAR,i,tag,comm,send_waits+count++);CHKERRQ(ierr);
    }
  }
  PetscFree(starts); PetscFree(nprocs);

  /* Free cache space */
  PLogInfo(0,"VecAssemblyBegin_MPI:Number of off-processor values %d\n",x->stash.n);
  x->stash.nmax = x->stash.n = 0;
  if (x->stash.array){ PetscFree(x->stash.array); x->stash.array = 0;}

  x->svalues    = svalues;       x->rvalues = rvalues;
  x->nsends     = nsends;         x->nrecvs = nreceives;
  x->send_waits = send_waits; x->recv_waits = recv_waits;
  x->rmax       = nmax;
  
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "VecAssemblyEnd_MPI"
int VecAssemblyEnd_MPI(Vec vec)
{
  Vec_MPI     *x = (Vec_MPI *)vec->data;
  MPI_Status  *send_status,recv_status;
  int         ierr,imdex,base,nrecvs = x->nrecvs, count = nrecvs, i, n;
  Scalar      *values;

  PetscFunctionBegin;
  if (x->stash.donotstash) {
    x->insertmode = NOT_SET_VALUES;
    PetscFunctionReturn(0);
  }

  base = x->ownership[x->rank];

  /*  wait on receives */
  while (count) {
    ierr = MPI_Waitany(nrecvs,x->recv_waits,&imdex,&recv_status);CHKERRQ(ierr);
    /* unpack receives into our local space */
    values = x->rvalues + 2*imdex*x->rmax;
    ierr = MPI_Get_count(&recv_status,MPIU_SCALAR,&n);CHKERRQ(ierr);
    n = n/2;
    if (x->insertmode == ADD_VALUES) {
      for ( i=0; i<n; i++ ) {
        x->array[((int) PetscReal(values[2*i])) - base] += values[2*i+1];
      }
    } else if (x->insertmode == INSERT_VALUES) {
      for ( i=0; i<n; i++ ) {
        x->array[((int) PetscReal(values[2*i])) - base] = values[2*i+1];
      }
    } else { 
      SETERRQ(PETSC_ERR_ARG_CORRUPT,0,"Insert mode is not set correctly; corrupted vector");
    }
    count--;
  }
  PetscFree(x->recv_waits); PetscFree(x->rvalues);
 
  /* wait on sends */
  if (x->nsends) {
    send_status = (MPI_Status *) PetscMalloc(x->nsends*sizeof(MPI_Status));CHKPTRQ(send_status);
    ierr        = MPI_Waitall(x->nsends,x->send_waits,send_status);CHKERRQ(ierr);
    PetscFree(send_status);
  }
  PetscFree(x->send_waits); PetscFree(x->svalues);

  x->insertmode = NOT_SET_VALUES;
  PetscFunctionReturn(0);
}

