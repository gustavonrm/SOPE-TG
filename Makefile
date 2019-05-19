LIBS = -lm

CC = gcc
CFLAGS = -g -Wall -Wextra -Werror -I. -D_GNU_SOURCE -D_REENTRANT -lpthread -lrt
LDFLAGS = -g -D_REENTRANT -lpthread
TFLAGS= -lrt -D_REENTRANT -lpthread

.PHONY: default user server all clean

all: user server

OBJECTS = log.c operations.c reply.c srv_utils.c usr_utils.c
HEADERS = constants.h error.h operations.h reply.h sope.h srv_utils.h types.h usr_utils.h

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $(TFLAGS) $< -o $@

server: $(OBJECTS)
	$(CC) server.c $(OBJECTS) -Wall $(LIBS) $(TFLAGS) -o $@

user: $(OBJECTS)
	$(CC) user.c $(OBJECTS) -Wall $(LIBS) $(TFLAGS) -o $@

clean:
	-rm -f *.o