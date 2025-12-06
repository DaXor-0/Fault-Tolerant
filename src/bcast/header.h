#ifndef BCAST_HEADER_H
#define BCAST_HEADER_H

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

typedef struct {
    MPI_Request *reqs; /* Pointer to the array of requests */
    int num_reqs;      /* Current size of the array */
} request_manager_t;

/* Ensures the array of MPI_Request is properly initialized and large enough. */
static inline MPI_Request *alloc_reqs(request_manager_t *manager, int nreqs)
{
    if (nreqs == 0) {
        return NULL;
    }

    if (manager->num_reqs < nreqs) {
        MPI_Request *new_reqs = realloc(manager->reqs, sizeof(MPI_Request) * nreqs);
        if (new_reqs == NULL) {
            manager->reqs = NULL;
            manager->num_reqs = 0;
            return NULL;
        }

        manager->reqs = new_reqs;

        for (int i = manager->num_reqs; i < nreqs; i++) {
            manager->reqs[i] = MPI_REQUEST_NULL;
        }

        manager->num_reqs = nreqs;
    }

    return manager->reqs;
}

/* Frees the request manager resources. */
static inline void cleanup_reqs(request_manager_t *manager)
{
    if (manager->reqs != NULL) {
        free(manager->reqs);
        manager->reqs = NULL;
    }
    manager->num_reqs = 0;
}

/*
 * rounddown: Rounds a number down to nearest multiple.
 *     rounddown(10,4) = 8, rounddown(6,3) = 6, rounddown(14,3) = 12
 */
static inline int rounddown(int num, int factor)
{
    num /= factor;
    return num * factor; /* floor(num / factor) * factor */
}

#ifdef DEBUG
#define BINE_DEBUG_PRINT(fmt, ...)           \
    do {                                     \
        fprintf(stderr, fmt, ##__VA_ARGS__); \
    } while (0)
#else
#define BINE_DEBUG_PRINT(fmt, ...) \
    do {                           \
    } while (0)
#endif

int bcast_linear(void *buff, size_t count, MPI_Datatype datatype, int root, MPI_Comm comm);
int bcast_binomial(void *buffer, size_t count, MPI_Datatype datatype, int root, MPI_Comm comm_ptr);
int bcast_scatter_allgather(void *buf, size_t count, MPI_Datatype dtype, int root, MPI_Comm comm);

#endif /* BCAST_HEADER_H */
