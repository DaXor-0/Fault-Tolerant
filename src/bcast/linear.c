#include "header.h"
#include <time.h>

/*
 * Open MPI linear broadcast baseline (no fault awareness).
 */
int bcast_linear(void *buff, size_t count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
    int i, size, rank, err;
    request_manager_t req_manager = {NULL, 0};
    MPI_Request *reqs = NULL;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    if (1 == size) {
        return MPI_SUCCESS;
    }

    if (rank != root) {
        return MPI_Recv(buff, count, datatype, root, 0, comm, MPI_STATUS_IGNORE);
    }

    reqs = alloc_reqs(&req_manager, size - 1);
    if (NULL == reqs) {
        return MPI_ERR_NO_MEM;
    }

    for (i = 0; i < size; ++i) {
        if (i == rank) {
            continue;
        }

        err = MPI_Isend(buff, count, datatype, i, 0, comm, &reqs[i < rank ? i : i - 1]);
        if (MPI_SUCCESS != err) {
            goto err_hndl;
        }
    }
    --i;

    err = MPI_Waitall(i, reqs, MPI_STATUSES_IGNORE);
err_hndl:
    if (NULL != reqs) {
        cleanup_reqs(&req_manager);
    }
    return err;
}

static int run_linear(int buf_size, int root, MPI_Comm comm)
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
    int err = bcast_linear(buffer, (size_t)buf_size, MPI_INT, root, comm);
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

    run_linear(buf_size, root, MPI_COMM_WORLD);

    MPI_Finalize();
    return 0;
}
