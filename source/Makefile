# OFS Project Makefile
# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
PTHREAD = -pthread

# Directories
SRC_DIR = .
CORE_DIR = core
SERVER_DIR = server
INCLUDE_DIR = include
BUILD_DIR = build
BIN_DIR = bin

# Include path
INCLUDES = -I$(INCLUDE_DIR)

# Source files (NO HEADERS HERE!)
CORE_SRCS = $(CORE_DIR)/core_system.cpp \
            $(CORE_DIR)/user_manager.cpp \
            $(CORE_DIR)/file_manager.cpp \
            $(CORE_DIR)/dir_manager.cpp \
            $(CORE_DIR)/info_manager.cpp \
            $(CORE_DIR)/helper.cpp


SERVER_SRC = $(SERVER_DIR)/server.cpp
CLIENT_SRC = $(SERVER_DIR)/client.cpp

# Object files
CORE_OBJS = $(CORE_SRCS:$(CORE_DIR)/%.cpp=$(BUILD_DIR)/%.o)
SERVER_OBJ = $(BUILD_DIR)/server.o
CLIENT_OBJ = $(BUILD_DIR)/client.o

# Executables
SERVER_BIN = $(BIN_DIR)/ofs_server
CLIENT_BIN = $(BIN_DIR)/ofs_client

# Default target
all: directories $(SERVER_BIN) $(CLIENT_BIN)
	@echo ""
	@echo "========================================="
	@echo "  BUILD COMPLETE!"
	@echo "========================================="
	@echo "Server: $(SERVER_BIN)"
	@echo "Client: $(CLIENT_BIN)"
	@echo ""
	@echo "To run:"
	@echo "  Terminal 1: make run-server"
	@echo "  Terminal 2: make run-client"
	@echo "========================================="

# Create directories
directories:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

# Compile core object files
$(BUILD_DIR)/%.o: $(CORE_DIR)/%.cpp
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile server object file
$(BUILD_DIR)/server.o: $(SERVER_SRC)
	@echo "Compiling server..."
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(PTHREAD) -c $< -o $@

# Compile client object file
$(BUILD_DIR)/client.o: $(CLIENT_SRC)
	@echo "Compiling client..."
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Link server executable
$(SERVER_BIN): $(CORE_OBJS) $(SERVER_OBJ)
	@echo "Linking server..."
	@$(CXX) $(PTHREAD) $(CORE_OBJS) $(SERVER_OBJ) -o $(SERVER_BIN)

# Link client executable
$(CLIENT_BIN): $(CLIENT_OBJ)
	@echo "Linking client..."
	@$(CXX) $(CLIENT_OBJ) -o $(CLIENT_BIN)

# Run server
run-server: $(SERVER_BIN)
	@echo "Starting OFS Server..."
	@$(SERVER_BIN)

# Run client
run-client: $(CLIENT_BIN)
	@echo "Starting OFS Client..."
	@$(CLIENT_BIN)

# Clean
clean:
	@echo "Cleaning build files..."
	@rm -rf $(BUILD_DIR)
	@rm -rf $(BIN_DIR)
	@echo "Clean complete!"

# Rebuild
rebuild: clean all

# Help
help:
	@echo "OFS Project Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  make          - Build everything"
	@echo "  make run-server"
	@echo "  make run-client"
	@echo "  make clean"
	@echo "  make rebuild"

.PHONY: all directories clean rebuild run-server run-client help
