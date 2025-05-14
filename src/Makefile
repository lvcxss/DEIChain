# gcc settings
CC        = gcc
CFLAGS    = -Wall -Wextra -g -pthread
LDFLAGS   = -lcrypto

# executáveis
TARGET    = main
TARGET2   = TxGen

# objetos do controlador principal
CONTROLLER_OBJ      = controller.o
MINER_OBJ           = miner.o
STATISTICS_OBJ      = statistics.o
VALIDATOR_OBJ       = validator.o
VC_OBJ              = validatorcontroller.o
POW_OBJ             = pow.o
UTILS_OBJ           = utils.o
TRANSACTIONUTILS_OBJ= transactionutils.o
TXGEN_OBJ           = txgen.o

# regra padrão
all: $(TARGET) $(TARGET2)

# linka o binário principal
$(TARGET): main.o \
           $(CONTROLLER_OBJ) \
           $(MINER_OBJ) \
           $(STATISTICS_OBJ) \
           $(VALIDATOR_OBJ) \
           $(VC_OBJ) \
           $(UTILS_OBJ) \
           $(POW_OBJ) \
           $(TRANSACTIONUTILS_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# TXGen
$(TARGET2): $(TRANSACTIONUTILS_OBJ) $(TXGEN_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# regras de compilação
main.o: main.c controller.h
	$(CC) $(CFLAGS) -c $<


$(CONTROLLER_OBJ): controller.c controller.h miner.h validator.h statistics.h transaction.h
	$(CC) $(CFLAGS) -c $<

$(MINER_OBJ): miner.c miner.h controller.h pow.h deichain.h
	$(CC) $(CFLAGS) -c $<

$(STATISTICS_OBJ): statistics.c statistics.h controller.h deichain.h
	$(CC) $(CFLAGS) -c $<

$(VALIDATOR_OBJ): validator.c validator.h deichain.h
	$(CC) $(CFLAGS) -c $<

$(VC_OBJ): validatorcontroller.c validator.h deichain.h
	$(CC) $(CFLAGS) -c $<

$(UTILS_OBJ): utils.c deichain.h
	$(CC) $(CFLAGS) -c $<

$(POW_OBJ): pow.c pow.h deichain.h
	$(CC) $(CFLAGS) -c $<

$(TRANSACTIONUTILS_OBJ): transactionutils.c transaction.h controller.h deichain.h
	$(CC) $(CFLAGS) -c $<

$(TXGEN_OBJ): txgen.c transaction.h controller.h deichain.h
	$(CC) $(CFLAGS) -c $<

# limpeza
clean:
	rm -f *.o $(TARGET) $(TARGET2)


