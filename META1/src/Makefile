# gcc settings
CC = gcc
CFLAGS = -Wall -Wextra -g -pthread

# variables (for better organization during the creation of the project)
TARGET = main
TARGET2 = TxGen
CONTROLLER_OBJ = controller.o
MINER_OBJ = miner.o
TRANSACTIONGEN_OBJ = transactionutils.o
TRANSACTION_OBJ = txgen.o
VALIDATOR_OBJ = validator.o
STATISTICS_OBJ = statistics.o
UTILS_OBJ = utils.o

all: $(TARGET) $(TARGET2)

$(TARGET): main.o $(CONTROLLER_OBJ) $(MINER_OBJ) $(TRANSACTIONGEN_OBJ) $(STATISTICS_OBJ) $(VALIDATOR_OBJ) $(UTILS_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(CONTROLLER_OBJ): controller.c controller.h miner.h validator.h
	$(CC) $(CFLAGS) -c $<

$(UTILS_OBJ): utils.c deichain.h 
	$(CC) $(CFLAGS) -c $<

$(TRANSACTIONGEN_OBJ): transactionutils.c transaction.h controller.h
	$(CC) $(CFLAGS) -c $<

$(MINER_OBJ): miner.c miner.h controller.h
	$(CC) $(CFLAGS) -c $<

$(STATISTICS_OBJ): statistics.c statistics.h controller.h 
	$(CC) $(CFLAGS) -c $<

$(VALIDATOR_OBJ): validator.c validator.h controller.h
	$(CC) $(CFLAGS) -c $<

main.o: main.c controller.h
	$(CC) $(CFLAGS) -c $<
$(TARGET2): $(TRANSACTIONGEN_OBJ) $(TRANSACTION_OBJ) $(UTILS_OBJ)
	$(CC) $(CFLAGS) -o $@ $^


$(TRANSACTION_OBJ): txgen.c transaction.h controller.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o
