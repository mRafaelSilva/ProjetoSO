#include "client.h"

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

    int responde_fd = open(PIPE_2, O_RDONLY);
    if (responde_fd < 0) {
        perror("Não deu para abrir o pipe de resposta");
        exit(EXIT_FAILURE);
    }

    int id;

    char cmd_buffer[1024];

    if (strcmp(argv[1], "status") == 0) {
        strcpy(cmd_buffer, "status");
        close (responde_fd);

        write(pipe_fd, cmd_buffer, strlen(cmd_buffer) + 1); 
        close(pipe_fd);

    } else if (strcmp(argv[1], "execute") == 0 && argc == 5) {
        snprintf(cmd_buffer, sizeof(cmd_buffer), "%s %s %s \"%s\"", argv[1], argv[2], argv[3], argv[4]);

        write(pipe_fd, cmd_buffer, strlen(cmd_buffer) + 1); 
        close(pipe_fd);  

        char id_task[1024];

    if (read(responde_fd, &id, sizeof(id)) < 0) {
        perror("Falha ao ler do FIFO");
        close(responde_fd);
        return EXIT_FAILURE;
    }

    int length = snprintf(id_task, sizeof(id_task), "Comando recebido. Foi atribuído à tarefa '%s' o id; %d\n", argv[4], id);
    if (length > 0 && length <  sizeof(id_task)) {
        write (STDOUT_FILENO, id_task, length);
    } else {
        write(STDOUT_FILENO, "ID da tarefa recebido, mas a mensagem é muito grande.\n", 56);

    }

    close (responde_fd);

    } else {
        const char* msg = "Comando inválido\n";
        write(STDERR_FILENO, msg, strlen(msg)); // Escreve na saída de erro
        close (pipe_fd);
        close (responde_fd);
        exit(EXIT_FAILURE);
    }


    
    return 0;
}