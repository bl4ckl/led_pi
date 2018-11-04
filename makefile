#CC = gcc
CFLAGS = -Wall -g -O0
OBJ = main.o networking/tcpclient.o networking/udpclient.o utility.o entity/entity.o spi.o threads.o networking/tcp_handler.o entity/entity_handler.o timers.o
BIN = prog

prog: $(OBJ)
	mv tcpclient.o networking/tcpclient.o
	mv udpclient.o networking/udpclient.o
	mv entity.o entity/entity.o
	mv entity_handler.o entity/entity_handler.o
	mv tcp_handler.o networking/tcp_handler.o
	$(CC) $(CFLAGS) -o prog $(OBJ) -lwiringPi -lrt -lpthread

%.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -rf $(BIN) $(OBJ)
	make
