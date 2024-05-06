CC = gcc
CFLAGS = -Wall -g -I$(INCDIR)
LDFLAGS =
SRCDIR = src
INCDIR = include
BINDIR = bin
CLIENT_SRC = $(SRCDIR)/client.c
ORCHESTRATOR_SRC = $(SRCDIR)/orchestrator.c
LOG_SRC = $(SRCDIR)/log.c
EXECUTABLES = orchestrator client
GERAL = task_log_geral.txt
LOGS = logs/*.txt

all: $(EXECUTABLES)

orchestrator: orchestrator.o log.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

client: client.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(EXECUTABLES) *.o
	rm -f $(LOGS)
	rm -f $(GERAL)
