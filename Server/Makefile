TARGET = server 
LIBS = -lm

CC = gcc
CFLAGS = -g -Wall -Wextra -Werror -I. -D_GNU_SOURCE -D_REENTRANT -lpthread 
LDFLAGS = -g -D_REENTRANT -lpthread
TFLAGS= -lrt -D_REENTRANT -lpthread

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c)) \
	$(patsubst ../Common/%.c, ../Common/%.o, $(wildcard ../Common/*.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $(TFLAGS) $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) $(TFLAGS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)
