#include <stdio.h>  //servidor a dizer o id da tarefa ao cliente;  meter a tarefa a ser enviada através de uma struct para o servidor
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
        const char* msg = "Comando inválido\n";
        write(STDERR_FILENO, msg, strlen(msg)); // Escreve na saída de erro
        exit(EXIT_FAILURE);
    }

    write(pipe_fd, cmd_buffer, strlen(cmd_buffer) + 1); 
    close(pipe_fd);

    char confirmation[1024];
    //    snprintf(confirmation, sizeof(confirmation), "Comando entregue: %s\n", cmd_buffer);
    //    write(STDOUT_FILENO, confirmation, strlen(confirmation)); // Escreve na saída padrão
    int message_len = snprintf(confirmation, sizeof(confirmation), "Comando entregue: %s\n", cmd_buffer);
    if (message_len > 0 && message_len < sizeof(confirmation)) {
        write(STDOUT_FILENO, confirmation, message_len);
    } else {
        // Caso a mensagem tenha sido maior que o buffer, lidar com isso de forma adequada
        write(STDOUT_FILENO, "Comando entregue, mas a mensagem é muito longa para exibição completa.\n", 68);
    }
    
    return 0;
}