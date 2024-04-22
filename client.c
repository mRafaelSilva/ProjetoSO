#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define PIPE_NAME "/tmp/orchestrator_pipe"

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s execute time -u|-p \"command [args]...\"\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Configurar os argumentos da tarefa
    char cmd_buffer[1024];
    sprintf(cmd_buffer, "%s %s %s", argv[1], argv[2], argv[3]);

    // Enviar o comando para o servidor
    int pipe_fd = open(PIPE_NAME, O_WRONLY);
    if (pipe_fd < 0) {
        perror("Failed to open pipe");
        exit(EXIT_FAILURE);
    }

    write(pipe_fd, cmd_buffer, strlen(cmd_buffer)+1);
    close(pipe_fd);

    printf("Task submitted: %s\n", cmd_buffer);

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

#define PIPE_NAME "/tmp/orchestrator_pipe"

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s execute time -u|-p \"command [args]...\"\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Configurar os argumentos da tarefa
    char cmd_buffer[1024];
    sprintf(cmd_buffer, "execute %s %s", argv[2], argv[3]);

    // Enviar o comando para o servidor
    int pipe_fd = open(PIPE_NAME, O_WRONLY);
    if (pipe_fd < 0) {
        perror("Failed to open pipe");
        exit(EXIT_FAILURE);
    }

    write(pipe_fd, cmd_buffer, strlen(cmd_buffer)+1);
    close(pipe_fd);

    printf("Task submitted: %s\n", cmd_buffer);

    return 0;
}
*/