#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/time.h>

#define PIPE_NAME "/tmp/orchestrator_pipe"
#define PIPE_2 "/tmp/responde_pipe"

#define MAX_TASKS 100


typedef struct{
    int id;
    char comando[256];
    char tipo[2];
    int tempo_estado;
    struct timeval hora_inicio;
    struct timeval hora_fim;    
    int status; // 0 = waiting, 1 = running, 2 = completed
    pid_t pid;
    int pipefd[2];
}task_t;


void verifica_se_terminou(int *active_tasks);
void adiciona_task(char *comando, char *tipo, int tempo_estado, int fd_response);
void executa_task(int task_index);
void processa_tasks();
void handle_status();

#endif /* ORCHESTRATOR_H */
