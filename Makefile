CC = gcc
CFLAGS = -Wall -Wextra -O3 -Wno-unused-result -Wno-unused-parameter
TARGET = puppyfetch
SRC = puppyfetch.c
PREFIX = /usr/bin

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)
	strip $(TARGET)

.PHONY: install
install: $(TARGET)
	install -m 755 $(TARGET) $(PREFIX)/$(TARGET)

.PHONY: uninstall 
uninstall:
	rm -f $(PREFIX)/$(TARGET)

.PHONY: clean
clean:
	rm -f $(TARGET)

