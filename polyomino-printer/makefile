CC = gcc
CFLAGS = -Wall -pedantic -std=c99

SRC_DIR = ./src
BLD_DIR = ./bld
BIN_DIR = ./bin
INS_DIR = $(HOME)/.local/bin


SRCS = $(filter-out $(patsubst %,$(SRC_DIR)/%.c,$(BINS)),$(wildcard $(SRC_DIR)/*.c))
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BLD_DIR)/%.o, $(SRCS))
BINS = polyomino-printer

LDLIBS =

.PHONY: all clean install uninstall


all: $(addprefix $(BIN_DIR)/,$(BINS))

clean:
	rm -rf $(BLD_DIR)
	rm -rf $(BIN_DIR)

install: $(INS_DIR)/$(notdir $(BINS))

uninstall:
	rm -f $(INS_DIR)/$(notdir $(BINS))

$(BIN_DIR)/%: $(BLD_DIR)/%.o $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS)   -o $@ $^ $(LDLIBS)

$(BLD_DIR)/%.o: $(SRC_DIR)/%.c | $(BLD_DIR)
	$(CC) $(CFLAGS)   -o $@ -c $^

$(INS_DIR)/% : $(BIN_DIR)/%
	cp $< $@

$(BLD_DIR) $(BIN_DIR):
	mkdir -p $@
