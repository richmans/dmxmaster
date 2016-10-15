CC=gcc
INSTALL_DIR=/usr/local/bin
SOURCES=dmxmaster.c
CFLAGS=-c -Wall -I /usr/local/include
LFLAGS=-L /usr/local/lib -lftdi1
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=build/dmxmaster

all: $(SOURCES) $(EXECUTABLE)

build:
	mkdir build

install: $(EXECUTABLE)
	install build/dmxmaster $(INSTALL_DIR)

$(EXECUTABLE): build $(OBJECTS)
	$(CC) $(LFLAGS) $(OBJECTS) -o $@

build/%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf build
