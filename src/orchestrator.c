#include "orchestrator.h"
#include "log.h"

task_t tasks[MAX_TASKS];
int contador_tasks = 0;
int tasks_paralelas;


/*
A função "verifica_se_terminou", percorre todas as tarefas que foram recebidas. Caso alguma delas esteja a ser executada, a função irá perceber se recebeu alguma mensagem que
indique que o processo-filho que está responsável por essa tarefa a terminou. Caso o mesmo se comprove, a função chama o processo de registar o log individual da tarefa assim como lhe
atualiza o status.
*/
void verifica_se_terminou(int *tarefas_ativas) {
    int terminou;
    for (int i = 0; i < contador_tasks; i++) {
        task_t *task = &tasks[i];
        if (task->status == 1) {
            int bytes_read = read(task->pipefd[0], &terminou, sizeof(terminou));
            if (bytes_read > 0) {
                if (terminou == 1) {
                    int status;
                    waitpid(task->pid, &status, 0);
                    gettimeofday(&task->hora_fim, NULL);
                    task->status = 2;
                    log_task_completa(*task);
                    printf("Task %d completada\n", task->id);
                    close(task->pipefd[0]);
                    (*tarefas_ativas)--;
                }
            } else if (bytes_read == 0) {
                // O processo filho terminou a escrita e fechou o pipe, trate como erro se necessário
                int status;
                waitpid(task->pid, &status, 0);
                gettimeofday(&task->hora_fim, NULL);
                task->status = 2;
                log_task_completa(*task);
                printf("Task %d completada\n", task->id);
                close(task->pipefd[0]);
                (*tarefas_ativas)--;
            }
        }
    }
}

/*
A função adiciona_task faz o que o próprio nome indica. Esta inserção tem de acordo o tempo estimado de cada pedido submetido pelo cliente de forma a otimizar o processo de resoluções
de tarefas.
*/

void adiciona_task(char *comando, char *tipo, int tempo_estado, int fd_response) {
    if (contador_tasks >= MAX_TASKS) {
        char error_msg[] = "Atingiu-se o limite máximo de tarefas.\n";
        write(STDERR_FILENO, error_msg, sizeof(error_msg));
        return;
    }

    int pos = 0;
    while (pos < contador_tasks && tasks[pos].tempo_estado <= tempo_estado) {
        pos++;
    }

    for (int i = contador_tasks; i > pos; i--) {
        tasks[i] = tasks[i - 1];
    }

    tasks[pos].id = contador_tasks;

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
    write(fd_response, &contador_tasks, sizeof(contador_tasks));

    contador_tasks++;
    }


/*
A função "executa_task" é responsável por executar os comandos que são submetidos pelo cliente. Após os ter executado, faz com que o processo-filho envie para o seu pai uma mensagem 
a indicar que terminou a execução.
*/

void executa_task(int task_index) {
    task_t* task = &tasks[task_index];
    if (pipe(task->pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == 0) { 
        close(task->pipefd[0]); 

        char log_filename[256];
        sprintf(log_filename, "logs/task_log_id%d.txt", task->id);
        int log_fd = open(log_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (log_fd == -1) {
            perror("abrir o log file");
            exit(EXIT_FAILURE);
        }

        dup2(log_fd, STDOUT_FILENO);
        dup2(log_fd, STDERR_FILENO);
        close(log_fd);

        if (strcmp(task->tipo, "-p") == 0) {
            execlp("sh", "sh", "-c", task->comando, NULL);
        } else {
            execlp("sh", "sh", "-c", task->comando, NULL);
        }
        
        int done = 1;
        write(task->pipefd[1], &done, sizeof(done));
        close(task->pipefd[1]);  
        exit(0);
    } else if (pid > 0) {  
        close(task->pipefd[1]); 
        task->pid = pid;
        task->status = 1;
    } else {
        perror("fork");
    }
}

/*
A função "processa_tasks" é responsável por gerir o início de tarefs que estão na fila de espera. É ela que, de acordo com as restrições implementadas, indica se um pedido pode ou não ser
iniciado.
*/

void processa_tasks() {
    static int tarefas_ativas = 0;

    verifica_se_terminou(&tarefas_ativas);

    for (int i = 0; i < contador_tasks; i++) {
        if (tasks[i].status == 0 && tarefas_ativas < tasks_paralelas) {
            if (pipe(tasks[i].pipefd) == -1) {
                perror("pipe");
                continue;
            }

            pid_t pid = fork();
            if (pid == 0) {  // Processo filho
                close(tasks[i].pipefd[0]);  // Fecha a extremidade de leitura
                executa_task(i);
                exit(0);
            } else if (pid > 0) {  // Processo pai
                close(tasks[i].pipefd[1]);  // Fecha a extremidade de escrita
                tasks[i].pid = pid;
                tasks[i].status = 1;
                gettimeofday(&tasks[i].hora_inicio, NULL);
                tarefas_ativas++;
            } else {
                perror("Falha no fork");
            }
        }
    }
}


/*
A função "handle_status" está relacionada com o comando ""./client status". Ao percorrer todas as tarefas que foram rebecidas até ao momento, irá organiza-las e indicar as que se encontram
a ser executadas, as que estão em espera e as que foram completadas.
*/

void handle_status() {
        write(STDOUT_FILENO, "A serem executadas\n", 20);
        for (int i = 0; i < contador_tasks; i++) {
            if (tasks[i].status == 1) {
                char msg[1024];
                snprintf(msg, sizeof(msg), "Task %d a correr: %s\n", tasks[i].id, tasks[i].comando);
                write(STDOUT_FILENO, msg, strlen(msg));
            } 
        }

        write(STDOUT_FILENO, "Em espera\n", 10);

        for (int i = 0; i < contador_tasks; i++) {
            if (tasks[i].status == 0) {

                char msg[1024];
                snprintf(msg, sizeof(msg), "Task %d à espera: %s\n", tasks[i].id, tasks[i].comando);
                write(STDOUT_FILENO, msg, strlen(msg));
            } 

        }
        write(STDOUT_FILENO, "Completadas\n", 12);

        for (int i = 0; i<contador_tasks; i++) {

            if (tasks[i].status == 2) {
                long seconds = tasks[i].hora_fim.tv_sec - tasks[i].hora_inicio.tv_sec;
                long useconds = tasks[i].hora_fim.tv_usec - tasks[i].hora_inicio.tv_usec;
                double duration = seconds + useconds / 1E6;
                char msg[1024];
                snprintf(msg, sizeof(msg), "Task %d completada: %s, Duração: %.6f segundos\n", tasks[i].id, tasks[i].comando, duration);
                write(STDOUT_FILENO, msg, strlen(msg));                
            }

        }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        char usage_msg[256];
        snprintf(usage_msg, sizeof(usage_msg), "Usage: %s output_folder tasks_paralelas sched_policy\n", argv[0]);
        write(STDERR_FILENO, usage_msg, strlen(usage_msg));
        exit(EXIT_FAILURE);
    }

    tasks_paralelas = atoi(argv[2]);
    mkfifo(PIPE_NAME, 0666);
    int pipe_fd = open(PIPE_NAME, O_RDONLY); 
    if (pipe_fd < 0) {
        perror("Falha a abrir o pipe");
        exit(EXIT_FAILURE);
    }

    mkfifo(PIPE_2, 0666);
    int response_fd = open(PIPE_2, O_WRONLY);
    if (response_fd < 0) {
        perror("Falha a abrir o pipe de resposta");
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
                    adiciona_task(comando, tipo, tempo_estado, response_fd);
                }
                    ptr += strlen(ptr) + 1; 
                } else if (strncmp(ptr, "status", 6) == 0) {
                    handle_status();
                    ptr += strlen(ptr) + 1;
                } else {
                    ptr += strlen(ptr) + 1; // Ignora comandos desconhecidos
                }
            }
        }
        processa_tasks(); // Chama fora da condição de leitura para evitar bloqueio
        usleep(50000); // Valor aleatório que demos só para correr os testes e não consumir muito CPU
    }

    close(pipe_fd);
    return 0;
}