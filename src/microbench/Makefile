# Directories
SRC_DIR = .
BIN_DIR = bin

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Executable names
BINS = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%, $(SRCS))

# Compilation flags
CFLAGS = -lm

# Targets
all: $(BINS)

$(BIN_DIR)/%: $(SRC_DIR)/%.c $(SRC_DIR)/common.h
	$(CC) -o $@ $< $(CFLAGS)

clean:
	-rm -f $(BINS)