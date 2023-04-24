BIN = dbus-example
CFLAGS = -Wall
CFLAGS += $(shell pkg-config --cflags --libs dbus-1)
SOURCES = dbus-example.c
#SOURCES = send.c

default: $(SOURCES)
	gcc  $(SOURCES) $(CFLAGS) -o $(BIN) 

clean:
	rm -f $(BIN)

.PHONY: default clean
