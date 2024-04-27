    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <string.h>
    #include <fcntl.h>
    #include <sys/stat.h>
    #include <sys/types.h>

    #define PIPE_NAME "/tmp/orchestrator_pipe"

    int main(int argc, char *argv[]) {
        if (argc < 2) {
            fprintf(stderr, "Usage: %s <command> [args]\n", argv[0]);
            exit(EXIT_FAILURE);
        }

        int pipe_fd = open(PIPE_NAME, O_WRONLY);
        if (pipe_fd < 0) {
            perror("Não deu para abrir o pipe");
            exit(EXIT_FAILURE);
        }

        char cmd_buffer[1024];

        if (strcmp(argv[1], "status") == 0) {
            strcpy(cmd_buffer, "status");
        } else if (strcmp(argv[1], "execute") == 0 && argc == 5) {
            snprintf(cmd_buffer, sizeof(cmd_buffer), "%s %s %s \"%s\"", argv[1], argv[2], argv[3], argv[4]);
        } else {
            fprintf(stderr, "Comando inválido\n");
            exit(EXIT_FAILURE);
        }

        write(pipe_fd, cmd_buffer, strlen(cmd_buffer) + 1); 
        close(pipe_fd);

        printf("Comando entregue: %s\n", cmd_buffer); 

        return 0;
    }