CC = gcc
CFLAGS = -Wall -Wextra -pthread
BUILD_DIR = build

SOURCES = main.c \
	ires/ires.c \
	ires/conv_try.c \
	funcexe/funcex.c \
	libct/libct.c \
	libres/libres.c

OBJECTS = $(addprefix $(BUILD_DIR)/, $(SOURCES:.c=.o))
TARGET = $(BUILD_DIR)/main

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)/ires $(BUILD_DIR)/funcexe $(BUILD_DIR)/libct $(BUILD_DIR)/libres

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean

