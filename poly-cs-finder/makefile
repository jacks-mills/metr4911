CC = gcc
CFLAGS = -std=c99 -Wall -pedantic -g

SRC_DIR = ./src
BLD_DIR = ./bld
BIN_DIR = ./bin
INS_DIR = $(HOME)/.local/bin

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BLD_DIR)/%.o, $(SRCS))
PROG = $(BIN_DIR)/pcritical

LDLIBS =

.PHONY: all clean install uninstall
.SECONDARY:


all: $(PROG)

clean:
	rm -rf $(BLD_DIR)
	rm -rf $(BIN_DIR)

install: $(INS_DIR)/$(notdir $(PROG))

uninstall:
	rm -f $(INS_DIR)/$(notdir $(PROG))

$(BIN_DIR)/%: $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS)   -o $@ $^ $(LDLIBS)

$(BLD_DIR)/%.o: $(SRC_DIR)/%.c | $(BLD_DIR)
	$(CC) $(CFLAGS)   -o $@ -c $^

$(INS_DIR)/% : $(BIN_DIR)/%
	cp $< $@

$(BLD_DIR) $(BIN_DIR):
	mkdir -p $@
