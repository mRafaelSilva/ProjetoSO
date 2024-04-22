CC = gcc
CFLAGS = -Wall -g -Iinclude
LDFLAGS =
CLIENT_SRC = client.c
ORCHESTRATOR_SRC = orchestrator.c
EXECUTABLES = orchestrator client

all: $(EXECUTABLES)

orchestrator: $(ORCHESTRATOR_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

client: $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

clean:
	rm -f $(EXECUTABLES)