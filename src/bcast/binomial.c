#include "header.h"
#include <limits.h>
#include <time.h>

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

static int run_binomial(int buf_size, int root, MPI_Comm comm)
{
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    int *buffer = malloc(sizeof(int) * buf_size);
    if (buffer == NULL) {
        return MPI_ERR_NO_MEM;
    }

    int root_value = root + 1;
    for (int i = 0; i < buf_size; i++) {
        buffer[i] = (rank == root) ? root_value : -1;
    }

    clock_t start = clock();
    int err = bcast_binomial(buffer, (size_t)buf_size, MPI_INT, root, comm);
    MPI_Barrier(comm);
    clock_t end = clock();
    double difftime = ((double)(end - start)) / CLOCKS_PER_SEC;

    int res = 0;
    for (int i = 0; i < buf_size; i++) {
        res += (buffer[i] % 17);
    }

    if (rank == 0) {
        printf("P: %d\n", size);
        printf("Size: %d\n", buf_size);
        printf("Time: %lf\n", difftime);
    }
    printf("Hello from %d of %d and the result is: %d\n", rank, size, res);

    free(buffer);
    return err;
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

    run_binomial(buf_size, root, MPI_COMM_WORLD);

    MPI_Finalize();
    return 0;
}
