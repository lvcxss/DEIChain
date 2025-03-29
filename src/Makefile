# gcc settings
CC = gcc
CFLAGS = -Wall -Wextra -g -pthread

# variables (for better organization during the creation of the project)
TARGET = main
TARGET2 = TxGen
CONTROLLER_OBJ = controller.o
MINER_OBJ = miner.o
TRANSACTIONGEN_OBJ = transactiongenerator.o
TRANSACTION_OBJ = transaction.o
STATISTICS_OBJ = statistics.o

all: $(TARGET) $(TARGET2)

$(TARGET): main.o $(CONTROLLER_OBJ) $(MINER_OBJ) $(TRANSACTIONGEN_OBJ) $(STATISTICS_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(CONTROLLER_OBJ): controller.c controller.h miner.h
	$(CC) $(CFLAGS) -c $<

$(TRANSACTIONGEN_OBJ): transactiongenerator.c transaction.h 
	$(CC) $(CFLAGS) -c $<

$(MINER_OBJ): miner.c miner.h
	$(CC) $(CFLAGS) -c $<

$(STATISTICS_OBJ): statistics.c statistics.h
	$(CC) $(CFLAGS) -c $<

main.o: main.c controller.h
	$(CC) $(CFLAGS) -c $<

$(TARGET2): $(TRANSACTIONGEN_OBJ) $(TRANSACTION_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(TRANSACTION_OBJ): transaction.c transaction.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGET) $(TARGET2) *.o
