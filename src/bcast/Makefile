CC = mpicc
CFLAGS = -Wall

TARGETS = linear binomial scatter_allgather

all: $(TARGETS)

linear: linear.c header.h
	$(CC) $(CFLAGS) -o $@ linear.c

binomial: binomial.c header.h
	$(CC) $(CFLAGS) -o $@ binomial.c

scatter_allgather: scatter_allgather.c header.h
	$(CC) $(CFLAGS) -o $@ scatter_allgather.c

clean:
	rm -f $(TARGETS)
