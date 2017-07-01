CC = gcc
CFLAGS = -Wextra -Wall -g -O0 -DDEBUG
LDLIBS = -lnl-3 -lnl-genl-3

INCLUDE = /usr/include/libnl3
SOURCES = main.c
BIN_NAME = testgenetlink
TARGET = ../$(BIN_NAME)

all: $(TARGET)

$(TARGET): $(BIN_NAME)
	cp $< $@

$(BIN_NAME): $(SOURCES)
	$(CC) $(CFLAGS) $^ -o $@ -I$(INCLUDE) $(LDLIBS)

clean:
	rm -f $(BIN_NAME) $(TARGET)
