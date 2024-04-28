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
        char comando[256];
        char tipo[2];
        int tempo_estado;
        struct timeval hora_inicio;
        struct timeval hora_fim;    
        int status; // 0 = à espera, 1 = a correr, 2 = completo
        pid_t pid;
    } task_t;

    task_t tasks[MAX_TASKS];
    int task_count = 0;
    int parallel_tasks;


    // faz o registro da tarefa no arquivo
    void log_task_completion(task_t task) {
        FILE *log_file = fopen("task_log_geral.txt", "a");
        if (log_file == NULL) {
            perror("Failed to open log file");
            return;
        }
        long seconds = task.hora_fim.tv_sec - task.hora_inicio.tv_sec;
        long useconds = task.hora_fim.tv_usec - task.hora_inicio.tv_usec;
        double duration = seconds + useconds / 1E6; 
        fprintf(log_file, "Task ID: %d, Comando: %s, Tempo Estimado: %d ms, Tempo Real: %.6f s\n",
                task.id, task.comando, task.tempo_estado, duration);
        fclose(log_file);
    }

    void check_task_completion(int *active_tasks) {
        int status;
        pid_t pid;
        struct timeval now;

        while (*active_tasks > 0 && (pid = wait(&status)) > 0) {
            for (int j = 0; j < task_count; j++) {
                if (tasks[j].pid == pid) {
                    gettimeofday(&now, NULL);
                    tasks[j].hora_fim = now;
                    tasks[j].status = 2;  // Marca como completa
                    log_task_completion(tasks[j]);

                    char output[128];
                    int length = snprintf(output, sizeof(output), "Task %d completada\n", tasks[j].id);
                    write(STDOUT_FILENO, output, length); // Imprime no terminal usando write

                    (*active_tasks)--;
                    break;  // Sai do loop após encontrar e tratar a tarefa
                }
            }
        }
    }



    void add_task(char *comando, char *tipo, int tempo_estado) {
        if (task_count >= MAX_TASKS) {
            char error_msg[] = "Atingiu-se o limite máximo de tarefas.\n";
            write(STDERR_FILENO, error_msg, sizeof(error_msg));
            return;
        }

        int pos = 0;
        while (pos < task_count && tasks[pos].tempo_estado <= tempo_estado) {
            pos++;
        }

        for (int i = task_count; i > pos; i--) {
            tasks[i] = tasks[i - 1];
        }

        tasks[pos].id = task_count;
        strcpy(tasks[pos].comando, comando);
        strcpy(tasks[pos].tipo, tipo);
        tasks[pos].tempo_estado = tempo_estado;
        tasks[pos].status = 0;
        struct timeval now;
        gettimeofday(&now, NULL);
        tasks[pos].hora_inicio = now;

        char output[256];
        int length = snprintf(output, sizeof(output), "Task %d adicionada: %s\n", tasks[pos].id, tasks[pos].comando);
        write(STDOUT_FILENO, output, length);
        
        task_count++;
    }

    // Função para executar uma tarefa específica
    void execute_task(int task_index) {
        task_t* task = &tasks[task_index];

        char log_filename[256];
        sprintf(log_filename, "logs/task_log_id%d.txt", task->id);
        int log_fd = open(log_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (log_fd < 0) {
            perror("Falha ao abrir o arquivo log");
            exit(EXIT_FAILURE);
        }

        dup2(log_fd, STDOUT_FILENO);
        dup2(log_fd, STDERR_FILENO);
        close(log_fd);

        if (strcmp(task->tipo, "-p") == 0) {
            // Executa como pipelined
            execlp("sh", "sh", "-c", task->comando, NULL);
        } else {
            // Executa como comando único
            execlp("sh", "sh", "-c", task->comando, NULL);
        }
        exit(EXIT_FAILURE); // Caso execlp falhe
    }

    void process_tasks() {
        static int active_tasks = 0;

        // Verifica e conclui tarefas que terminaram
        check_task_completion(&active_tasks);

        // Iniciar tarefas apenas se houver slots disponíveis
        for (int i = 0; i < task_count; i++) {
            if (tasks[i].status == 0 && active_tasks < parallel_tasks) {
                pid_t pid = fork();
                if (pid == 0) {
                    // Processo filho: executa a tarefa
                    execute_task(i);
                } else if (pid > 0) {
                    tasks[i].pid = pid;
                    tasks[i].status = 1;
                    gettimeofday(&tasks[i].hora_inicio, NULL);
                    active_tasks++;
                } else {
                    perror("Falha no fork");
                }
            }
        }
    }


    void handle_status_request() {
        for (int i = 0; i < task_count; i++) {
            if (tasks[i].status == 1) {
                printf("Task %d a correr: %s\n", tasks[i].id, tasks[i].comando);
            } else if (tasks[i].status == 0) {
                printf("Task %d à espera: %s\n", tasks[i].id, tasks[i].comando);
            } else if (tasks[i].status == 2) {
                long seconds = tasks[i].hora_fim.tv_sec - tasks[i].hora_inicio.tv_sec;
                long useconds = tasks[i].hora_fim.tv_usec - tasks[i].hora_inicio.tv_usec;
                double duration = seconds + useconds / 1E6;
                printf("Task %d completada: %s, Duração: %.6f segundos\n", tasks[i].id, tasks[i].comando, duration);
            }
        }
    }

    int main(int argc, char *argv[]) {
        if (argc != 4) {
            char usage_msg[256];
            snprintf(usage_msg, sizeof(usage_msg), "Usage: %s output_folder parallel_tasks sched_policy\n", argv[0]);
            write(STDERR_FILENO, usage_msg, strlen(usage_msg));
            exit(EXIT_FAILURE);
        }

        parallel_tasks = atoi(argv[2]);
        mkfifo(PIPE_NAME, 0666);
        int pipe_fd = open(PIPE_NAME, O_RDONLY | O_NONBLOCK); 
        if (pipe_fd < 0) {
            perror("Falha a abrir o pipe");
            exit(EXIT_FAILURE);
        }

        char buffer[1024];
        while (1) {
            int num_read = read(pipe_fd, buffer, sizeof(buffer) - 1);
            if (num_read > 0) {
                buffer[num_read] = '\0'; 
                char *ptr = buffer;
                while (ptr < buffer + num_read) {
                    if (strncmp(ptr, "execute", 7) == 0) {
                        char comando[256], tipo[2];
                        int tempo_estado;
                    if (sscanf(ptr, "execute %d %2s \"%[^\"]\"", &tempo_estado, tipo, comando) == 3) {
                        add_task(comando, tipo, tempo_estado);
                    }
                        ptr += strlen(ptr) + 1; 
                    } else if (strncmp(ptr, "status", 6) == 0) {
                        handle_status_request();
                        ptr += strlen(ptr) + 1;
                    } else {
                        ptr += strlen(ptr) + 1; // Ignora comandos desconhecidos
                    }
                }
            }
            process_tasks(); // Chama fora da condição de leitura para evitar bloqueio
            usleep(50000); // Ajuste conforme necessário para equilibrar carga e responsividade
        }

        close(pipe_fd);
        return 0;
    }