
#if !defined(VecNode_impl_h)
#define VecNode_impl_h

#include <petsc/private/vecimpl.h>

#if defined(PETSC_HAVE_MPI_WIN_CREATE_FEATURE)
typedef struct {
  VECHEADER
  MPI_Win     win;
  MPI_Comm    shmcomm;
  PetscScalar **winarray; /* holds array pointer of shared value array */
} Vec_Node;
#endif

#endif
