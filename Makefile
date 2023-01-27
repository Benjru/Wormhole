CC=gcc
SOURCES=event_handler.c keybinds.c action_processor.c
CFLAGS=$(shell pkg-config --cflags --libs x11 xcb xcb-keysyms xkbcommon xkbcommon-x11)
EXECUTABLE=wormhole

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(SOURCES)
	$(CC) $(SOURCES) $(CFLAGS) -o $(EXECUTABLE)
