#include "header.h"
#include <limits.h>
#include <stdlib.h>

/*
 * MPICH binomial broadcast baseline (no fault awareness).
 */
int bcast_binomial(void *buffer, size_t count, MPI_Datatype datatype, int root, MPI_Comm comm_ptr)
{
    int rank, comm_size, src, dst;
    int relative_rank, mask;
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint nbytes = 0, lb;
    MPI_Status *status_p;
    status_p = MPI_STATUS_IGNORE;
    MPI_Aint type_size;

    MPI_Comm_size(comm_ptr, &comm_size);
    MPI_Comm_rank(comm_ptr, &rank);

    MPI_Type_get_extent(datatype, &lb, &type_size);

    nbytes = type_size * count;
    if (nbytes == 0) {
        goto fn_exit;
    }

    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    mask = 0x1;
    while (mask < comm_size) {
        if (relative_rank & mask) {
            src = rank - mask;
            if (src < 0) {
                src += comm_size;
            }
            mpi_errno = MPI_Recv(buffer, count, datatype, src, 0, comm_ptr, status_p);
            if (mpi_errno != MPI_SUCCESS) {
                goto fn_fail;
            }
            break;
        }
        mask <<= 1;
    }

    mask >>= 1;
    while (mask > 0) {
        if (relative_rank + mask < comm_size) {
            dst = rank + mask;
            if (dst >= comm_size) {
                dst -= comm_size;
            }
            mpi_errno = MPI_Send(buffer, count, datatype, dst, 0, comm_ptr);
            if (mpi_errno != MPI_SUCCESS) {
                goto fn_fail;
            }
        }
        mask >>= 1;
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int buf_size, root = 0;
    if (argc < 2) {
        printf("Error: buffer size expected\n");
        MPI_Finalize();
        return EXIT_FAILURE;
    }
    buf_size = atoi(argv[1]);

    run_fault_aware_bcast(bcast_binomial, buf_size, root);

    MPI_Finalize();
    return 0;
}
