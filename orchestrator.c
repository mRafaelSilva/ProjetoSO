// quando faço algum comando que deie erro, o erro não 
// imprime no terminal, apenas vai para o ficheiro log.txt ... 
// é suposto imprimir no terminal

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
#define MAX_TASKS 100

typedef struct {
    int id;
    char command[256];
    int estimated_time;
    struct timeval start_time;
    struct timeval end_time;
    int status; // 0 = waiting, 1 = running, 2 = completed
    pid_t pid;
} task_t;

task_t tasks[MAX_TASKS];
int task_count = 0;
int parallel_tasks;

void log_task_completion(task_t task) {
    FILE *log_file = fopen("task_log_geral.txt", "a");
    if (log_file == NULL) {
        perror("Failed to open log file");
        return;
    }
    long seconds = task.end_time.tv_sec - task.start_time.tv_sec;
    long useconds = task.end_time.tv_usec - task.start_time.tv_usec;
    double duration = seconds + useconds / 1E6;
    fprintf(log_file, "Task ID: %d, Command: %s, Estimated Time: %d ms, Real Time: %.6f s\n",
            task.id, task.command, task.estimated_time, duration);
    fclose(log_file);
}


/*
void add_task(char *command, int estimated_time) {
    if (task_count >= MAX_TASKS) {
        fprintf(stderr, "Maximum task limit reached.\n");
        return;
    }
    struct timeval now;
    gettimeofday(&now, NULL); // Pega o tempo atual em segundos e microssegundos

    tasks[task_count].id = task_count;
    strcpy(tasks[task_count].command, command);
    tasks[task_count].estimated_time = estimated_time;
    tasks[task_count].status = 0;
    tasks[task_count].start_time = now;

//    snprintf(tasks[task_count].log_filename, sizeof(tasks[task_count].log_filename), "task_log_%d.txt", tasks[task_count].id);
    
    printf("Task %d added: %s\n", tasks[task_count].id, tasks[task_count].command);
    task_count++;
}
*/
void check_task_completion(int *active_tasks) {
    int status;
    pid_t pid;
    struct timeval now;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int j = 0; j < task_count; j++) {
            if (tasks[j].pid == pid) {
                gettimeofday(&now, NULL);
                tasks[j].end_time = now;
                tasks[j].status = 2; // Marca como completada
                printf("Task %d completed\n", tasks[j].id);
                log_task_completion(tasks[j]);
                (*active_tasks)--;
            }
        }
    }
}
void add_task(char *command, int estimated_time) {
    if (task_count >= MAX_TASKS) {
        fprintf(stderr, "Maximum task limit reached.\n");
        return;
    }

    // Encontrar a posição correta para inserir a nova tarefa
    int pos = 0;
    while (pos < task_count && tasks[pos].estimated_time <= estimated_time) {
        pos++;
    }

    // Mover as tarefas existentes para abrir espaço para a nova tarefa
    for (int i = task_count; i > pos; i--) {
        tasks[i] = tasks[i - 1];
    }

    // Inserir a nova tarefa
    tasks[pos].id = task_count;
    strcpy(tasks[pos].command, command);
    tasks[pos].estimated_time = estimated_time;
    tasks[pos].status = 0;
    struct timeval now;
    gettimeofday(&now, NULL);
    tasks[pos].start_time = now;

    printf("Task %d added: %s\n", tasks[pos].id, tasks[pos].command);
    task_count++;
}


void process_tasks() {
    int active_tasks = 0;

    // Contar quantas tarefas já estão em execução
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].status == 1) {
            active_tasks++;
        }
    }

    for (int i = 0; i < task_count; i++) {
        // Somente inicia novas tarefas se menos do que o limite de tarefas paralelas estão ativas
        if (tasks[i].status == 0 && active_tasks < parallel_tasks) {
            pid_t pid = fork();
            if (pid == 0) { // Processo filho
                char log_filename[256];
                sprintf(log_filename, "logs/task_log_id%d.txt", tasks[i].id); // Nome do arquivo de log

                int log_fd = open(log_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (log_fd < 0) {
                    perror("Failed to open log file");
                    exit(EXIT_FAILURE);
                }

                dup2(log_fd, STDOUT_FILENO);
                dup2(log_fd, STDERR_FILENO);
                close(log_fd);

                execlp("sh", "sh", "-c", tasks[i].command, NULL);
                exit(EXIT_FAILURE);
            } else if (pid > 0) { // Processo pai
                tasks[i].pid = pid;
                tasks[i].status = 1; // Marca a tarefa como em execução
                gettimeofday(&tasks[i].start_time, NULL);
                active_tasks++; // Incrementa o contador de tarefas ativas
            } else {
                perror("Failed to fork");
            }
        }
    }

    // A função check_task_completion pode ser chamada aqui para verificar tarefas concluídas e decrementar active_tasks
    check_task_completion(&active_tasks);
}

/* FUNÇÃO PROCESS  CERTA
void process_tasks() {

    int active_tasks = 0;

    for (int i = 0; i < task_count; i++) {
        if (tasks[i].status == 0 && active_tasks < parallel_tasks) {
            pid_t pid = fork();
            if (pid == 0) { // Processo filho
                char log_filename[256]; // Buffer para o nome do arquivo de log
                sprintf(log_filename, "logs/task_log_id%d.txt", tasks[i].id); // Gerando o nome do arquivo

                // Abrindo arquivo de log para stdout e stderr
                int log_fd = open(log_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (log_fd < 0) {
                    perror("Failed to open log file");
                    exit(EXIT_FAILURE);
                }

                // Redirecionando stdout e stderr para o arquivo de log
                dup2(log_fd, STDOUT_FILENO);
                dup2(log_fd, STDERR_FILENO);
                close(log_fd);

                // Executando o comando
                execlp("sh", "sh", "-c", tasks[i].command, NULL);
                exit(EXIT_FAILURE); // Saída em caso de falha na execução
            } else if (pid > 0) { // Processo pai
                tasks[i].pid = pid;
                tasks[i].status = 1;
                gettimeofday(&tasks[i].start_time, NULL); // Substitua isso pela chamada correta
                active_tasks++;
            } else {
                perror("Failed to fork");
            }
        }
    }

    check_task_completion(&active_tasks);
}   


*/

void handle_status_request() {
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].status == 1) {
            printf("Task %d running: %s\n", tasks[i].id, tasks[i].command);
        } else if (tasks[i].status == 0) {
            printf("Task %d waiting: %s\n", tasks[i].id, tasks[i].command);
        } else if (tasks[i].status == 2) {
            long seconds = tasks[i].end_time.tv_sec - tasks[i].start_time.tv_sec;
            long useconds = tasks[i].end_time.tv_usec - tasks[i].start_time.tv_usec;
            double duration = seconds + useconds / 1E6; // Convertendo microssegundos em segundos e somando
            printf("Task %d completed: %s, Duration: %.6f seconds\n", tasks[i].id, tasks[i].command, duration);
        }
    }
}
int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s output_folder parallel_tasks sched_policy\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    parallel_tasks = atoi(argv[2]);
    mkfifo(PIPE_NAME, 0666);
    int pipe_fd = open(PIPE_NAME, O_RDONLY | O_NONBLOCK); // Open pipe with non-blocking mode
    if (pipe_fd < 0) {
        perror("Failed to open pipe");
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    int need_to_check = 1;

    while (1) {
        if (need_to_check) {
            process_tasks();
            need_to_check = 0;
        }

        ssize_t num_read = read(pipe_fd, buffer, sizeof(buffer) - 1);
        if (num_read > 0) {
            buffer[num_read] = '\0'; // Ensure null-terminated string
            char *ptr = buffer;

            while (ptr < buffer + num_read) {
                if (strncmp(ptr, "execute", 7) == 0) {
                    char command[256];
                    int estimated_time;
                    sscanf(ptr, "execute %d %*s \"%[^\"]\"", &estimated_time, command);
                    add_task(command, estimated_time);
                    need_to_check = 1;
                    ptr += strlen(ptr) + 1; // Move to next command in the buffer
                } else if (strncmp(ptr, "status", 6) == 0) {
                    int client_fd = open(PIPE_NAME, O_WRONLY); // Reopen pipe for writing status response
                    if (client_fd < 0) {
                        perror("Failed to open pipe for status response");
                        continue;
                    }
                    handle_status_request(client_fd);
                    close(client_fd);
                    ptr += strlen(ptr) + 1; // Move to next command
                } else {
                    printf("Unknown command received: %s\n", ptr);
                    ptr += strlen(ptr) + 1; // Skip unknown command
                }
            }
        } else {
            // No new data, but let's check for task completions anyway
            process_tasks();
            usleep(100000); // sleep for 0.1 seconds to reduce CPU usage
        }
    }

    close(pipe_fd);
    return 0;
}
