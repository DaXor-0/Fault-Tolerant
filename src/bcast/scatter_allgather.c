#include "header.h"
#include <time.h>

/*
 * Open MPI scatter + recursive doubling allgather broadcast baseline (no fault awareness).
 */
int bcast_scatter_allgather(void *buf, size_t count, MPI_Datatype dtype, int root, MPI_Comm comm)
{
    int rank, comm_size, err = MPI_SUCCESS, dtype_size_int;
    ptrdiff_t lb, extent;
    MPI_Status status;
    MPI_Type_get_extent(dtype, &lb, &extent);
    MPI_Type_size(dtype, &dtype_size_int);
    size_t dtype_size = (size_t)dtype_size_int;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &comm_size);

    if (comm_size < 2 || dtype_size == 0) {
        return MPI_SUCCESS;
    }

    if (count < (size_t)comm_size) {
        if (rank == 0) {
            BINE_DEBUG_PRINT("Error: count < comm_size");
        }
        return MPI_ERR_COUNT;
    }

    int vrank = (rank - root + comm_size) % comm_size;
    size_t recv_count = 0, send_count = 0;
    size_t scatter_count = (count + comm_size - 1) / comm_size; /* ceil(count / comm_size) */
    size_t curr_count = (rank == root) ? count : 0;
    int tmp_count;

    int mask = 0x1;
    while (mask < comm_size) {
        if (vrank & mask) {
            int parent = (rank - mask + comm_size) % comm_size;
            recv_count = count - vrank * scatter_count;
            if (recv_count <= 0) {
                curr_count = 0;
            } else {
                err = MPI_Recv((char *)buf + (ptrdiff_t)vrank * scatter_count * extent,
                               recv_count, dtype, parent, 0, comm, &status);
                if (MPI_SUCCESS != err) {
                    goto cleanup_and_return;
                }
                MPI_Get_count(&status, dtype, &tmp_count);
                curr_count = (size_t)tmp_count;
            }
            break;
        }
        mask <<= 1;
    }

    mask >>= 1;
    while (mask > 0) {
        if (vrank + mask < comm_size) {
            send_count = curr_count - scatter_count * mask;
            if (send_count > 0) {
                int child = (rank + mask) % comm_size;
                err = MPI_Send((char *)buf + (ptrdiff_t)scatter_count * (vrank + mask) * extent,
                               send_count, dtype, child, 0, comm);
                if (MPI_SUCCESS != err) {
                    goto cleanup_and_return;
                }
                curr_count -= send_count;
            }
        }
        mask >>= 1;
    }

    size_t rem_count = count - vrank * scatter_count;
    curr_count = (scatter_count < rem_count) ? scatter_count : rem_count;
    if (curr_count < 0) {
        curr_count = 0;
    }

    mask = 0x1;
    while (mask < comm_size) {
        int vremote = vrank ^ mask;
        int remote = (vremote + root) % comm_size;

        int vrank_tree_root = rounddown(vrank, mask);
        int vremote_tree_root = rounddown(vremote, mask);

        if (vremote < comm_size) {
            ptrdiff_t send_offset = vrank_tree_root * scatter_count * extent;
            ptrdiff_t recv_offset = vremote_tree_root * scatter_count * extent;
            recv_count = count - vremote_tree_root * scatter_count;
            if (recv_count < 0) {
                recv_count = 0;
            }
            err = MPI_Sendrecv((char *)buf + send_offset, curr_count, dtype, remote, 0,
                               (char *)buf + recv_offset, recv_count, dtype, remote, 0,
                               comm, &status);
            if (MPI_SUCCESS != err) {
                goto cleanup_and_return;
            }
            MPI_Get_count(&status, dtype, &tmp_count);
            recv_count = (size_t)tmp_count;
            curr_count += recv_count;
        }

        if (vremote_tree_root + mask > comm_size) {
            int nprocs_alldata = comm_size - vrank_tree_root - mask;
            ptrdiff_t offset = scatter_count * (vrank_tree_root + mask);
            for (int rhalving_mask = mask >> 1; rhalving_mask > 0; rhalving_mask >>= 1) {
                vremote = vrank ^ rhalving_mask;
                remote = (vremote + root) % comm_size;
                int tree_root = rounddown(vrank, rhalving_mask << 1);

                if ((vremote > vrank) && (vrank < tree_root + nprocs_alldata) &&
                    (vremote >= tree_root + nprocs_alldata)) {
                    err = MPI_Send((char *)buf + (ptrdiff_t)offset * extent,
                                   recv_count, dtype, remote, 0, comm);
                    if (MPI_SUCCESS != err) {
                        goto cleanup_and_return;
                    }

                } else if ((vremote < vrank) && (vremote < tree_root + nprocs_alldata) &&
                           (vrank >= tree_root + nprocs_alldata)) {
                    err = MPI_Recv((char *)buf + (ptrdiff_t)offset * extent,
                                   count, dtype, remote, 0, comm, &status);
                    if (MPI_SUCCESS != err) {
                        goto cleanup_and_return;
                    }
                    MPI_Get_count(&status, dtype, &tmp_count);
                    recv_count = (size_t)tmp_count;
                    curr_count += recv_count;
                }
            }
        }
        mask <<= 1;
    }

cleanup_and_return:
    return err;
}

static int run_scatter_allgather(int buf_size, int root, MPI_Comm comm)
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
    int err = bcast_scatter_allgather(buffer, (size_t)buf_size, MPI_INT, root, comm);
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

    run_scatter_allgather(buf_size, root, MPI_COMM_WORLD);

    MPI_Finalize();
    return 0;
}
