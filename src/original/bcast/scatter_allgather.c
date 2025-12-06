#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[])
{
    setenv("OMPI_MCA_coll_tuned_use_dynamic_rules", "1", 1);   /* Use dynamic rules */
    setenv("OMPI_MCA_coll_tuned_bcast_algorithm", "8", 1);     /* Scatter-allgather */

    int size, rank, res;
    clock_t start, end;
    double difftime;
    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc < 2) {
        printf("Error: buffer size expected\n");
        MPI_Finalize();
        return EXIT_FAILURE;
    }
    int buf_size = atoi(argv[1]);
    int *buffer = (int *)malloc(buf_size * sizeof(int));
    if (buffer == NULL) {
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    int root = 0;
    int root_value = root + 1;
    for (int i = 0; i < buf_size; i++) {
        buffer[i] = (rank == root) ? root_value : -1;
    }

    start = clock();
    MPI_Bcast(buffer, buf_size, MPI_INT, root, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    end = clock();
    difftime = ((double)(end - start)) / CLOCKS_PER_SEC;

    res = 0;
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
    MPI_Finalize();
    return 0;
}
