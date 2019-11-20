CC=gcc
CFLAGS= -g3
SRC_DIR=src
OBJ_DIR=obj
INCLUDE_DRI=include

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

.PHONY: all clean

all: jobCommander jobExecutorServer

jobCommander: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $<

jobExecutorServer: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
		$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) $(OBJ) jobCommander jobExecutorServer