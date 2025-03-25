CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lreadline -lhistory
SRC = lambda_calc.c
BUILD_DIR = build
TARGET = $(BUILD_DIR)/lambda_calc

$(TARGET): $(SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SRC) -o $@ $(LDFLAGS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
