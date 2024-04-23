    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <string.h>
    #include <fcntl.h>
    #include <sys/stat.h>
    #include <sys/types.h>

    #define PIPE_NAME "/tmp/orchestrator_pipe"

    int main(int argc, char *argv[]) {
        // Checagem básica para comandos de status
        if (argc < 2) {
            fprintf(stderr, "Usage: %s <command> [args]\n", argv[0]);
            exit(EXIT_FAILURE);
        }

        // Abrir o pipe nomeado para escrita
        int pipe_fd = open(PIPE_NAME, O_WRONLY);
        if (pipe_fd < 0) {
            perror("Failed to open pipe");
            exit(EXIT_FAILURE);
        }

        char cmd_buffer[1024];

        // Montar o comando baseado no input
        if (strcmp(argv[1], "status") == 0) {
            // Comando de status não requer mais argumentos
            strcpy(cmd_buffer, "status");
        } else if (strcmp(argv[1], "execute") == 0 && argc == 5) {
            // Comando de execução com todos os argumentos necessários
            snprintf(cmd_buffer, sizeof(cmd_buffer), "%s %s %s \"%s\"", argv[1], argv[2], argv[3], argv[4]);
        } else {
            fprintf(stderr, "Invalid command or insufficient arguments\n");
            fprintf(stderr, "Usage for execute: %s execute time -u|-p \"command [args]...\"\n", argv[0]);
            fprintf(stderr, "Usage for status: %s status\n", argv[0]);
            exit(EXIT_FAILURE);
        }

        // Enviar o comando pelo pipe
        write(pipe_fd, cmd_buffer, strlen(cmd_buffer) + 1);
        close(pipe_fd);

        printf("Command submitted: %s\n", cmd_buffer);

        return 0;
    }