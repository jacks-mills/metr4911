CC = gcc
CFLAGS = -std=c99 -Wall -pedantic -g

SRC_DIR = ./src
BLD_DIR = ./bld
BIN_DIR = ./bin
INS_DIR = $(HOME)/.local/bin

SRCS = $(filter-out $(patsubst %,$(SRC_DIR)/%.c,$(BINS)),$(wildcard $(SRC_DIR)/*.c))
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BLD_DIR)/%.o, $(SRCS))
BINS = plister

LDLIBS =

.PHONY: all clean
.SECONDARY:


all: $(addprefix $(BIN_DIR)/,$(BINS))

clean:
	rm -rf $(BLD_DIR)
	rm -rf $(BIN_DIR)


$(BIN_DIR)/%: $(BLD_DIR)/%.o $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS)   -o $@ $^ $(LDLIBS)

$(BLD_DIR)/%.o: $(SRC_DIR)/%.c | $(BLD_DIR)
	$(CC) $(CFLAGS)   -o $@ -c $^

$(BLD_DIR) $(BIN_DIR):
	mkdir -p $@
