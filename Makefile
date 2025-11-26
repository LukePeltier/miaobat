# Makefile for read_battery utility

CC = gcc
CFLAGS = -O2 -Wall -Wextra
TARGET = read_battery
SOURCE = src/read_battery.c
PREFIX ?= /usr/local

.PHONY: all clean install uninstall test

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)
	@echo "Build complete: $(TARGET)"
	@ls -lh $(TARGET)

clean:
	rm -f $(TARGET)
	@echo "Cleaned"

install: $(TARGET)
	install -m 755 $(TARGET) $(PREFIX)/bin/$(TARGET)
	@echo "Installed to $(PREFIX)/bin/$(TARGET)"

uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)
	@echo "Uninstalled $(PREFIX)/bin/$(TARGET)"

test: $(TARGET)
	@echo "Testing battery read..."
	./$(TARGET) -v
