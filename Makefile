CC := gcc
CFLAGS := -Wall -Wextra -Wpedantic -std=c17 -Iinclude $(shell pkg-config --cflags xft)
LDLIBS := -lX11 -lXinerama $(shell pkg-config --libs xft) -lpam

TARGET := lockd
SOURCES := src/main.c src/input.c src/ui.c src/auth.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $@ $(LDLIBS)

clean:
	rm -f $(TARGET)