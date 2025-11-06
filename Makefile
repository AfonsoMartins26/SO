CC = gcc
CFLAGS = -Wall -g -Iinclude -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include
LDFLAGS = -lglib-2.0

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

SRC_FILES_DSERVER = $(wildcard $(SRC_DIR)/dserver/*.c)
SRC_FILES_DCLIENT = $(wildcard $(SRC_DIR)/dclient/*.c)

OBJ_FILES_DSERVER = $(patsubst $(SRC_DIR)/dserver/%.c, $(OBJ_DIR)/dserver/%.o, $(SRC_FILES_DSERVER))
OBJ_FILES_DCLIENT = $(patsubst $(SRC_DIR)/dclient/%.c, $(OBJ_DIR)/dclient/%.o, $(SRC_FILES_DCLIENT))

all: folders dserver dclient

dserver: $(BIN_DIR)/dserver

dclient: $(BIN_DIR)/dclient

folders:
	@mkdir -p $(OBJ_DIR)/dserver $(OBJ_DIR)/dclient $(BIN_DIR) tmp

$(BIN_DIR)/dserver: $(OBJ_FILES_DSERVER)
	$(CC) $(OBJ_FILES_DSERVER) -o $@ $(LDFLAGS)

$(BIN_DIR)/dclient: $(OBJ_FILES_DCLIENT)
	$(CC) $(OBJ_FILES_DCLIENT) -o $@ $(LDFLAGS)

$(OBJ_DIR)/dserver/%.o: $(SRC_DIR)/dserver/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/dclient/%.o: $(SRC_DIR)/dclient/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) tmp

format:
	clang-format -i src/dclient/*.c src/dserver/*.c include/*.h
