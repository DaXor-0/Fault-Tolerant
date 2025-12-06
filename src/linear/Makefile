CC = mpicc
CFLAGS = -Wall -I../bcast_common
TARGET = main

SRCS = linear.c ../bcast_common/fault_common.c

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)
