CC = gcc
CFLAGS = -Wall -g -Iinclude
LDFLAGS =
CLIENT_SRC = client.c
ORCHESTRATOR_SRC = orchestrator.c
EXECUTABLES = orchestrator client
LOG_FILE = task_log.txt  # Adiciona o nome do arquivo de log aqui

all: $(EXECUTABLES)

orchestrator: $(ORCHESTRATOR_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

client: $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

clean:
	rm -f $(EXECUTABLES) $(LOG_FILE)  # Inclui o arquivo de log na limpeza

