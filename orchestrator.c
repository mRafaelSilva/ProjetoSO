#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define PIPE_NAME "/tmp/orchestrator_pipe"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s output_folder parallel_tasks sched_policy\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *output_folder = argv[1];
    int parallel_tasks = atoi(argv[2]);
    const char *sched_policy = argv[3];

    // Criar o pipe nomeado se não existir
    mkfifo(PIPE_NAME, 0666);

    // Abrir o pipe para leitura e escrita
    int pipe_fd = open(PIPE_NAME, O_RDWR);
    if (pipe_fd < 0) {
        perror("Failed to open pipe");
        exit(EXIT_FAILURE);
    }

    printf("Server started with output_folder: %s, parallel_tasks: %d, sched_policy: %s\n", output_folder, parallel_tasks, sched_policy);

    char buffer[1024];
    while (1) {
        read(pipe_fd, buffer, 1024);
        printf("Received: %s\n", buffer);
        // Aqui iríamos processar o pedido
    }

    close(pipe_fd);
    return 0;
}
/*
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define PIPE_NAME "/tmp/orchestrator_pipe"
#define MAX_TASKS 100

typedef struct {
    int id;
    char command[256];
    int estimated_time;
    time_t start_time;
    time_t end_time;
    int status; // 0 = waiting, 1 = running, 2 = completed
} task_t;

task_t tasks[MAX_TASKS];
int task_count = 0;

void add_task(char *command, int estimated_time) {
    if (task_count >= MAX_TASKS) {
        fprintf(stderr, "Maximum task limit reached.\n");
        return;
    }
    tasks[task_count].id = task_count + 1;
    strcpy(tasks[task_count].command, command);
    tasks[task_count].estimated_time = estimated_time;
    tasks[task_count].status = 0; // Waiting
    task_count++;
}

void process_tasks() {
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].status == 0) { // Se a tarefa está esperando
            printf("Executing task %d: %s\n", tasks[i].id, tasks[i].command);
            // Simular execução da tarefa
            tasks[i].status = 1; // Definir como em execução
            tasks[i].start_time = time(NULL);
            system(tasks[i].command); // Executar o comando
            tasks[i].end_time = time(NULL);
            tasks[i].status = 2; // Definir como completada
            printf("Completed task %d\n", tasks[i].id);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s output_folder parallel_tasks sched_policy\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    mkfifo(PIPE_NAME, 0666);
    int pipe_fd = open(PIPE_NAME, O_RDWR);
    if (pipe_fd < 0) {
        perror("Failed to open pipe");
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    while (1) {
        read(pipe_fd, buffer, sizeof(buffer));
        int estimated_time;
        char command[256];
        sscanf(buffer, "execute %d %[^\n]", &estimated_time, command); // Corrigir o formato aqui
        add_task(command, estimated_time);
        process_tasks();
    }

    close(pipe_fd);
    return 0;
}
*/  