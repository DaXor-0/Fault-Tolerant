#include "header.h"
#include <time.h>

static int root_in_failed_group(MPI_Comm comm, int root_comm_rank, int *nf_out)
{
    if (root_comm_rank == MPI_UNDEFINED) {
        if (nf_out != NULL) {
            *nf_out = 0;
        }
        return 1;
    }

    MPI_Group group_f, group_c, group_root, inter;
    int nf, inter_size;

    MPIX_Comm_failure_ack(comm);
    MPIX_Comm_failure_get_acked(comm, &group_f);
    MPI_Comm_group(comm, &group_c);

    MPI_Group_size(group_f, &nf);
    if (nf_out != NULL) {
        *nf_out = nf;
    }
    if (nf <= 0) {
        MPI_Group_free(&group_f);
        MPI_Group_free(&group_c);
        return 0;
    }

    MPI_Group_incl(group_c, 1, &root_comm_rank, &group_root);
    MPI_Group_intersection(group_f, group_root, &inter);
    MPI_Group_size(inter, &inter_size);

    MPI_Group_free(&group_f);
    MPI_Group_free(&group_c);
    MPI_Group_free(&group_root);
    MPI_Group_free(&inter);
    return inter_size > 0;
}

/*
 * Fault-aware broadcast wrapper: reruns the provided broadcast algorithm
 * on a shrunken communicator after a single failure. Aborts if the root dies.
 */
int run_fault_aware_bcast(int (*algo)(void *, size_t, MPI_Datatype, int, MPI_Comm),
                          int buf_size, int root_world)
{
    int orig_rank, orig_size;
    MPI_Comm comm;
    MPI_Group world_group;
    int root_comm_rank;
    int err = MPI_SUCCESS;

    MPI_Comm_rank(MPI_COMM_WORLD, &orig_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &orig_size);

    int *buffer = malloc(sizeof(int) * buf_size);
    if (buffer == NULL) {
        return MPI_ERR_NO_MEM;
    }

    MPI_Comm_dup(MPI_COMM_WORLD, &comm);
    MPI_Comm_group(MPI_COMM_WORLD, &world_group);

    MPI_Group group_c;
    MPI_Comm_group(comm, &group_c);
    MPI_Group_translate_ranks(world_group, 1, &root_world, group_c, &root_comm_rank);
    MPI_Group_free(&group_c);
    if (root_comm_rank == MPI_UNDEFINED) {
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    clock_t start = clock();

    while (1) {
        /* Reset buffer to the initial state before each attempt */
        int root_value = root_world + 1;
        for (int i = 0; i < buf_size; i++) {
            buffer[i] = (orig_rank == root_world) ? root_value : -1;
        }

        MPI_Comm_set_errhandler(comm, MPI_ERRORS_RETURN);

        err = algo(buffer, (size_t)buf_size, MPI_INT, root_comm_rank, comm);
        int barrier_err = MPI_Barrier(comm);
        if (err == MPI_SUCCESS && barrier_err == MPI_SUCCESS) {
            break;
        }

        /* Only one failure supported; check and recover */
        int agree = 1;
        MPIX_Comm_agree(comm, &agree);

        int nf = 0;
        if (root_in_failed_group(comm, root_comm_rank, &nf)) {
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        if (nf > 1) {
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        MPI_Comm new_comm;
        MPIX_Comm_shrink(comm, &new_comm);
        MPI_Comm_free(&comm);
        comm = new_comm;

        MPI_Comm_group(comm, &group_c);
        MPI_Group_translate_ranks(world_group, 1, &root_world, group_c, &root_comm_rank);
        MPI_Group_free(&group_c);
        if (root_comm_rank == MPI_UNDEFINED) {
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    MPI_Comm_set_errhandler(comm, MPI_ERRORS_ARE_FATAL);
    MPI_Barrier(comm);

    clock_t end = clock();
    double difftime = ((double)(end - start)) / CLOCKS_PER_SEC;

    int res = 0;
    for (int i = 0; i < buf_size; i++) {
        res += (buffer[i] % 17);
    }

    if (orig_rank == 0) {
        printf("P: %d\n", orig_size);
        printf("Size: %d\n", buf_size);
        printf("Time: %lf\n", difftime);
    }
    printf("Hello from %d of %d and the result is: %d\n", orig_rank, orig_size, res);

    MPI_Group_free(&world_group);
    MPI_Comm_free(&comm);
    free(buffer);
    return err;
}
