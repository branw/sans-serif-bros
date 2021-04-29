CC = gcc
CFLAGS = -Wall -std=c99
INC = include ext/baro

.PHONY: default all test clean

default: ssb | test
all: default

SOURCES = src/canvas.c src/db.c src/game.c src/screen.c src/server.c src/session.c src/state.c src/terminal.c src/util.c

OBJECTS = $(patsubst %.c, %.o, src/main.c $(SOURCES))
TEST_OBJECTS = $(patsubst %.c, %.test.o, ext/baro/baro.c $(SOURCES))
HEADERS = ext/baro/baro.h $(wildcard include/*.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(INC:%=-I%) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

ssb: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LIBS) -o $@

%.test.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(INC:%=-I%) -DBARO_ENABLE -c $< -o $@

test-ssb: $(TEST_OBJECTS)
	$(CC) $(CFLAGS) $(TEST_OBJECTS) $(LIBS) -o $@

test: test-ssb
	./$<

clean:
	-rm -f $(OBJECTS) $(TEST_OBJECTS)
	-rm -f ssb
	-rm -f test-ssb
