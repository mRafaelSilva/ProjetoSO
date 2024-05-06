#include "log.h"


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