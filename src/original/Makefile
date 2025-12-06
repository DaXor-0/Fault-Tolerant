CC     = mpicc
CFLAGS = -Wall

# Targets
all: rd raben linear binomial scatter_allgather

rd: rd.c
	$(CC) $(CFLAGS) -o rd.exe rd.c

raben: raben.c
	$(CC) $(CFLAGS) -o raben.exe raben.c

linear: linear.c
	$(CC) $(CFLAGS) -o linear.exe linear.c

binomial: binomial.c
	$(CC) $(CFLAGS) -o binomial.exe binomial.c

scatter_allgather: scatter_allgather.c
	$(CC) $(CFLAGS) -o scatter_allgather.exe scatter_allgather.c

clean:
	rm -f rd.exe raben.exe linear.exe binomial.exe scatter_allgather.exe
