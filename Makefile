# Makefile for Awale Game

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Isrc -DDEBUG

# Source files
CORE_SRC = src/core/awale.c
UTILS_SRC = src/utils/utils.c
PROTOCOL_SRC = src/protocol/protocol.c
CLIENT_SRC = src/client/client.c
SERVER_SRC = src/server/server.c

# Output directory
BIN_DIR = bin
TARGETS = $(BIN_DIR)/server $(BIN_DIR)/client $(BIN_DIR)/test $(BIN_DIR)/offline

all: $(BIN_DIR) $(TARGETS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Server binary: server_main.c + server/server.c + protocol + core + utils
$(BIN_DIR)/server: $(BIN_DIR) src/server_main.c $(SERVER_SRC) $(PROTOCOL_SRC) $(CORE_SRC) $(UTILS_SRC)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/server src/server_main.c $(SERVER_SRC) $(PROTOCOL_SRC) $(CORE_SRC) $(UTILS_SRC)

# Client binary: client_main.c + client/client.c + protocol + utils
$(BIN_DIR)/client: $(BIN_DIR) src/client_main.c $(CLIENT_SRC) $(PROTOCOL_SRC) $(UTILS_SRC)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/client src/client_main.c $(CLIENT_SRC) $(PROTOCOL_SRC) $(UTILS_SRC)

# Test binary: test.c + core + utils
$(BIN_DIR)/test: $(BIN_DIR) src/test.c $(CORE_SRC) $(UTILS_SRC)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/test src/test.c $(CORE_SRC) $(UTILS_SRC)

# Offline binary: offline.c + core + utils
$(BIN_DIR)/offline: $(BIN_DIR) src/offline.c $(CORE_SRC) $(UTILS_SRC)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/offline src/offline.c $(CORE_SRC) $(UTILS_SRC)

clean:
	rm -rf $(BIN_DIR)

run-server: $(BIN_DIR)/server
	./$(BIN_DIR)/server

run-client: $(BIN_DIR)/client
	./$(BIN_DIR)/client $(USERNAME)

run-test: $(BIN_DIR)/test
	./$(BIN_DIR)/test

run-offline: $(BIN_DIR)/offline
	./$(BIN_DIR)/offline

.PHONY: all clean run-server run-client run-test run-offline

