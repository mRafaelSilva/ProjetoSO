CC = gcc
CFLAGS = -Wall -g -Iinclude
LDFLAGS =
CLIENT_SRC = client.c
ORCHESTRATOR_SRC = orchestrator.c
CLIENT_OBJ = $(patsubst %.c, obj/%.o, $(CLIENT_SRC))
ORCHESTRATOR_OBJ = $(patsubst %.c, obj/%.o, $(ORCHESTRATOR_SRC))
EXECUTABLES = bin/orchestrator bin/client

all: folders $(EXECUTABLES)

folders:
	@mkdir -p obj bin tmp

bin/orchestrator: $(ORCHESTRATOR_OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

bin/client: $(CLIENT_OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

obj/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f obj/* tmp/* bin/*
