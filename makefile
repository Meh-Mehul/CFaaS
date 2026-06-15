CC = gcc
CFLAGS = -Wall -Wextra -pthread
BUILD_DIR = build

SOURCES = main.c \
	ires/ires.c \
	funcexe/funcex.c \
	libct/libct.c \
	libres/libres.c

OBJECTS = $(addprefix $(BUILD_DIR)/, $(SOURCES:.c=.o))
TARGET = $(BUILD_DIR)/main

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)/ires $(BUILD_DIR)/funcexe $(BUILD_DIR)/libct $(BUILD_DIR)/libres $(BUILD_DIR)/clients $(BUILD_DIR)/templib $(BUILD_DIR)/libs

# client build outputs
CLIENT_BUILD_DIR = $(BUILD_DIR)/clients

CLIENTS = $(CLIENT_BUILD_DIR)/libct $(CLIENT_BUILD_DIR)/faas_sample

client: $(CLIENT_BUILD_DIR) $(CLIENTS)

$(CLIENT_BUILD_DIR):
	mkdir -p $(CLIENT_BUILD_DIR)

$(CLIENT_BUILD_DIR)/libct: clients/libct_client.c
	$(CC) $(CFLAGS) -o $@ $<

$(CLIENT_BUILD_DIR)/faas_sample: clients/faas_client_sample.c
	$(CC) $(CFLAGS) -o $@ $<

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean

