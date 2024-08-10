CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = puppyfetch
SRC = puppyfetch.c
PREFIX = /usr/bin

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

.PHONY: install
install: $(TARGET)
	install -m 755 $(TARGET) $(PREFIX)/$(TARGET)

.PHONY: clean
clean:
	rm -f $(TARGET)

