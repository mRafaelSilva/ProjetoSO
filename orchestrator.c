#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <sys/wait.h>

#define PIPE_NAME "/tmp/orchestrator_pipe"
#define MAX_TASKS 100

typedef struct {
    int id;
    char command[256];
    int estimated_time;
    time_t start_time;
    time_t end_time;
    int status; // 0 = waiting, 1 = running, 2 = completed
    pid_t pid;
} task_t;

task_t tasks[MAX_TASKS];
int task_count = 0;
int parallel_tasks;

void add_task(char *command, int estimated_time) {
    if (task_count >= MAX_TASKS) {
        fprintf(stderr, "Maximum task limit reached.\n");
        return;
    }
    tasks[task_count].id = task_count;
    strcpy(tasks[task_count].command, command);
    tasks[task_count].estimated_time = estimated_time;
    tasks[task_count].status = 0;
    tasks[task_count].start_time = time(NULL);
    printf("Task %d added: %s\n", tasks[task_count].id, tasks[task_count].command);
    task_count++;
}

void process_tasks() {
    int active_tasks = 0;
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].status == 0 && active_tasks < parallel_tasks) {
            pid_t pid = fork();
            if (pid == 0) {
                execlp("sh", "sh", "-c", tasks[i].command, NULL);
                exit(EXIT_FAILURE);
            } else if (pid > 0) {
                tasks[i].pid = pid;
                tasks[i].status = 1;
                active_tasks++;
            } else {
                perror("Failed to fork");
            }
        }
    }

    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int j = 0; j < task_count; j++) {
            if (tasks[j].pid == pid) {
                tasks[j].end_time = time(NULL);
                tasks[j].status = 2;
                printf("Task %d completed\n", tasks[j].id);
                active_tasks--;
            }
        }
    }
}

void handle_status_request() {
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].status == 1) {
            printf("Task %d running: %s\n", tasks[i].id, tasks[i].command);
        } else if (tasks[i].status == 0) {
            printf("Task %d waiting: %s\n", tasks[i].id, tasks[i].command);
        } else if (tasks[i].status == 2) {
            printf("Task %d completed: %s, Duration: %ld seconds\n", tasks[i].id, tasks[i].command, tasks[i].end_time - tasks[i].start_time);
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
    while (1) {
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
                    process_tasks();
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
        }
    }

    close(pipe_fd);
    return 0;
}