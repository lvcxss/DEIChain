# gcc settings
CC = gcc
CFLAGS = -Wall -Wextra -g -pthread

# variables (for better organization during the creation of the project)
TARGET = main
CONTROLLER_OBJ = controller.o
MINER_OBJ = miner.o
TRANSACTION_OBJ = transaction.o

all: $(TARGET)

$(TARGET): main.o $(CONTROLLER_OBJ) $(MINER_OBJ) $(TRANSACTION_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(CONTROLLER_OBJ): controller.c controller.h miner.h
	$(CC) $(CFLAGS) -c $<

$(TRANSACTION_OBJ): transaction.c transaction.h controller.c controller.h
	$(CC) $(CFLAGS) -c $<

$(MINER_OBJ): miner.c miner.h
	$(CC) $(CFLAGS) -c $<

main.o: main.c controller.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGET) *.o

