/*$Id: pmetis.c,v 1.30 2000/01/30 03:41:04 bsmith Exp bsmith $*/
 
#include "src/mat/impls/csr/mpi/mpicsr.h"    /*I "mat.h" I*/

/* 
   Currently using ParMetis-2.0. The following include file has
   to be changed to par_kmetis.h for ParMetis-1.0
*/
EXTERN_C_BEGIN
#include "parmetis.h"
EXTERN_C_END

/*
      The first 5 elements of this structure are the input control array to Metis
*/
typedef struct {
  int cuts;         /* number of cuts made (output) */
  int foldfactor;
  int parallel;     /* use parallel partitioner for coarse problem */
  int indexing;     /* 0 indicates C indexing, 1 Fortran */
  int printout;     /* indicates if one wishes Metis to print info */
} MatPartitioning_Parmetis;

/*
   Uses the ParMETIS parallel matrix partitioner to partition the matrix in parallel
*/
#undef __FUNC__  
#define __FUNC__ "MatPartitioningApply_Parmetis" 
static int MatPartitioningApply_Parmetis(MatPartitioning part,IS *partitioning)
{
  int                      ierr,*locals,size,rank;
  int                      *vtxdist,*xadj,*adjncy,itmp = 0;
  Mat                      mat = part->adj,newmat;
  Mat_MPICSR               *adj = (Mat_MPICSR *)mat->data;
  MatPartitioning_Parmetis *parmetis = (MatPartitioning_Parmetis*)part->data;

  PetscFunctionBegin;
  ierr = MPI_Comm_size(mat->comm,&size);CHKERRQ(ierr);
  if (part->n != size) {
    SETERRQ(PETSC_ERR_SUP,1,"Supports exactly one domain per processor");
  }

  if (mat->type != MATMPICSR) {
    ierr = MatConvert(mat,MATMPICSR,&newmat);CHKERRQ(ierr);
    adj = (Mat_MPICSR *)newmat->data;
  }

  vtxdist = adj->rowners;
  xadj    = adj->i;
  adjncy  = adj->j;
  ierr = MPI_Comm_rank(part->comm,&rank);CHKERRQ(ierr);
  if (!(vtxdist[rank+1] - vtxdist[rank])) {
    SETERRQ(1,1,"Does not support any processor with no entries");
  }
#if defined(PETSC_BOPT_g)
  /* check that matrix has no diagonal entries */
  {
    int j,rstart;
    ierr = MatGetOwnershipRange(mat,&rstart,PETSC_NULL);CHKERRQ(ierr);
    for (i=0; i<m; i++) {
      for (j=xadj[i]; j<xadj[i+1]; j++) {
        if (adjncy[j] == i+rstart) SETERRQ1(1,1,"Row %d has illigal diagonal entry",i+rstart);
      }
    }
  }
#endif

  locals = (int*)PetscMalloc((adj->m+1)*sizeof(int));CHKPTRQ(locals);

  if (PLogPrintInfo) {itmp = parmetis->printout; parmetis->printout = 127;}
  PARKMETIS(vtxdist,xadj,part->vertex_weights,adjncy,adj->values,locals,(int*)parmetis,part->comm);
  if (PLogPrintInfo) {parmetis->printout = itmp;}

  ierr = ISCreateGeneral(part->comm,adj->m,locals,partitioning);CHKERRQ(ierr);
  ierr = PetscFree(locals);CHKERRQ(ierr);

  if (mat->type != MATMPICSR) {
    ierr = MatDestroy(newmat);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}


#undef __FUNC__  
#define __FUNC__ "MatPartitioningView_Parmetis" 
int MatPartitioningView_Parmetis(MatPartitioning part,Viewer viewer)
{
  MatPartitioning_Parmetis *parmetis = (MatPartitioning_Parmetis *)part->data;
  int                      ierr,rank;
  PetscTruth               isascii;

  PetscFunctionBegin;
  ierr = MPI_Comm_rank(part->comm,&rank);CHKERRQ(ierr);
  ierr = PetscTypeCompare((PetscObject)viewer,ASCII_VIEWER,&isascii);CHKERRQ(ierr);
  if (isascii) {
    if (parmetis->parallel == 2) {
      ierr = ViewerASCIIPrintf(viewer,"  Using parallel coarse grid partitioner\n");CHKERRQ(ierr);
    } else {
      ierr = ViewerASCIIPrintf(viewer,"  Using sequential coarse grid partitioner\n");CHKERRQ(ierr);
    }
    ierr = ViewerASCIIPrintf(viewer,"  Using %d fold factor\n",parmetis->foldfactor);CHKERRQ(ierr);
    ierr = ViewerASCIISynchronizedPrintf(viewer,"  [%d]Number of cuts found %d\n",rank,parmetis->cuts);CHKERRQ(ierr);
    ierr = ViewerFlush(viewer);CHKERRQ(ierr);
  } else {
    SETERRQ1(1,1,"Viewer type %s not supported for this Parmetis partitioner",((PetscObject)viewer)->type_name);
  }

  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "MatPartitioningParmetisSetCoarseSequential"
/*@
     MatPartitioningParmetisSetCoarseSequential - Use the sequential code to 
         do the partitioning of the coarse grid.

  Collective on MatPartitioning

  Input Parameter:
.  part - the partitioning context

   Level: advanced

@*/
int MatPartitioningParmetisSetCoarseSequential(MatPartitioning part)
{
  MatPartitioning_Parmetis *parmetis = (MatPartitioning_Parmetis *)part->data;

  PetscFunctionBegin;
  parmetis->parallel = 1;
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "MatPartitioningPrintHelp_Parmetis" 
int MatPartitioningPrintHelp_Parmetis(MatPartitioning part)
{
  int ierr;

  PetscFunctionBegin;
  ierr = (*PetscHelpPrintf)(part->comm,"ParMETIS options\n");CHKERRQ(ierr);
  ierr = (*PetscHelpPrintf)(part->comm,"  -mat_partitioning_parmetis_coarse_sequential\n");CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "MatPartitioningSetFromOptions_Parmetis" 
int MatPartitioningSetFromOptions_Parmetis(MatPartitioning part)
{
  int        ierr;
  PetscTruth flag;

  PetscFunctionBegin;
  ierr = OptionsHasName(part->prefix,"-mat_partitioning_parmetis_coarse_sequential",&flag);CHKERRQ(ierr);
  if (flag) {
    ierr = MatPartitioningParmetisSetCoarseSequential(part);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}


#undef __FUNC__  
#define __FUNC__ "MatPartitioningDestroy_Parmetis" 
int MatPartitioningDestroy_Parmetis(MatPartitioning part)
{
  MatPartitioning_Parmetis *parmetis = (MatPartitioning_Parmetis *)part->data;
  int ierr;

  PetscFunctionBegin;
  ierr = PetscFree(parmetis);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}

EXTERN_C_BEGIN
#undef __FUNC__  
#define __FUNC__ "MatPartitioningCreate_Parmetis" 
int MatPartitioningCreate_Parmetis(MatPartitioning part)
{
  MatPartitioning_Parmetis *parmetis;

  PetscFunctionBegin;
  parmetis = PetscNew(MatPartitioning_Parmetis);CHKPTRQ(parmetis);

  parmetis->cuts       = 0;   /* output variable */
  parmetis->foldfactor = 150; /*folding factor */
  parmetis->parallel   = 2;   /* use parallel partitioner for coarse grid */
  parmetis->indexing   = 0;   /* index numbering starts from 0 */
  parmetis->printout   = 0;   /* print no output while running */

  part->ops->apply          = MatPartitioningApply_Parmetis;
  part->ops->view           = MatPartitioningView_Parmetis;
  part->ops->destroy        = MatPartitioningDestroy_Parmetis;
  part->ops->printhelp      = MatPartitioningPrintHelp_Parmetis;
  part->ops->setfromoptions = MatPartitioningSetFromOptions_Parmetis;
  part->data                = (void*)parmetis;
  PetscFunctionReturn(0);
}
EXTERN_C_END

