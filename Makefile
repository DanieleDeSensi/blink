# Directories
SRC_DIR = .
BIN_DIR = bin

# Compilers (Make defaults to cc and g++, but it's good practice to define them)
CC = gcc
CXX = g++

# Source files
C_SRCS = $(wildcard $(SRC_DIR)/*.c)
CPP_SRCS = $(wildcard $(SRC_DIR)/*.cpp)

# Executable names
C_BINS = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%, $(C_SRCS))
CPP_BINS = $(patsubst $(SRC_DIR)/%.cpp, $(BIN_DIR)/%, $(CPP_SRCS))

# Compilation flags
CFLAGS = -lm -D_GNU_SOURCE -O3
CXXFLAGS = -lm -D_GNU_SOURCE -O3  # Matches CFLAGS, but you can add C++ specific flags like -std=c++17 here

# Targets
all: $(C_BINS) $(CPP_BINS)

# Rule for C files
$(BIN_DIR)/%: $(SRC_DIR)/%.c $(SRC_DIR)/common.h
	@mkdir -p $(BIN_DIR)
	$(CC) -o $@ $< $(CFLAGS)

# Rule for C++ files
$(BIN_DIR)/%: $(SRC_DIR)/%.cpp $(SRC_DIR)/common.h
	@mkdir -p $(BIN_DIR)
	$(CXX) -o $@ $< $(CXXFLAGS)

clean:
	-rm -f $(C_BINS) $(CPP_BINS)
