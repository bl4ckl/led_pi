CC = gcc
CFLAGS = -Wall -g -O0
OBJ = main.o tcpclient.o udpclient.o utility.o entity.o spi.o
BIN = prog

prog: $(OBJ)
	$(CC) $(CFLAGS) -o prog $(OBJ) -lwiringPi -lrt

%.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -rf $(BIN) $(OBJ)
	make
