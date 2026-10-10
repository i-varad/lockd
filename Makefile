CC := gcc
CFLAGS := -Wall -Wextra -Wpedantic -std=c17
LDLIBS := -lX11

TARGET := lockd
SOURCES := src/main.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $@ $(LDLIBS)

clean:
	rm -f $(TARGET)