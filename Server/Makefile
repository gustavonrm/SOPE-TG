TARGET = server 
LIBS = -lm

CC = gcc
CFLAGS = -g -Wall -Wextra -Werror -I. -D_GNU_SOURCE -D_REENTRANT  -lpthread 
LDFLAGS = -g -D_REENTRANT -lpthread


.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)

#Makefile adapted from stackoverflow with owner permition 
#https://stackoverflow.com/questions/1484817/how-do-i-make-a-simple-makefile-for-gcc-on-linux	