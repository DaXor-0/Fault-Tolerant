CC = mpicc
CFLAGS = -Wall

TARGETS = linear binomial scatter_allgather

all: $(TARGETS)

linear: linear.c
	$(CC) $(CFLAGS) -o $@ linear.c

binomial: binomial.c
	$(CC) $(CFLAGS) -o $@ binomial.c

scatter_allgather: scatter_allgather.c
	$(CC) $(CFLAGS) -o $@ scatter_allgather.c

clean:
	rm -f $(TARGETS)
