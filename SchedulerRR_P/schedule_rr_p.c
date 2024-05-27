#include "schedule_rr_p.h"
#include "list.h"
#include "task.h"
#include "CPU.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define TIME_SLICE 10 // Slice de tempo máximo

Node *queues[MAX_PRIORITY] = {NULL}; // Filas de tarefas por prioridade
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t timer_expired = PTHREAD_COND_INITIALIZER;
int global_time = 0;

void add(char *name, int priority, int burst)
{
    Task *new_task = malloc(sizeof(Task));
    new_task->name = strdup(name);
    new_task->priority = priority;
    new_task->burst = burst;

    Node *new_node = malloc(sizeof(Node));
    new_node->task = new_task;
    new_node->next = NULL;

    if (queues[priority - 1] == NULL)
    {
        queues[priority - 1] = new_node;
    }
    else
    {
        Node *temp = queues[priority - 1];
        while (temp->next != NULL)
        {
            temp = temp->next;
        }
        temp->next = new_node;
    }
}

Task *remove_next_task(int priority)
{
    if (queues[priority - 1] != NULL)
    {
        Node *temp = queues[priority - 1];
        queues[priority - 1] = queues[priority - 1]->next;
        Task *task = temp->task;
        free(temp); // Liberar o nó da lista
        return task;
    }
    return NULL;
}

void list_tasks_priority(int priority)
{
    printf("\n\n-- Inicializando prioridade %d --\n", priority);
    Node *temp = queues[priority - 1];

    if (temp != NULL)
    {
        printf("Tarefas com esta prioridade:\n");
    }
    else
    {
        printf("Nenhuma Tarefa com esta prioridade.\n");
    }

    while (temp != NULL)
    {
        printf("Tarefa [%s] [Prioridade: %d] [Burst: %d]\n", temp->task->name, temp->task->priority, temp->task->burst);
        temp = temp->next;
    }
    printf("\n\n");
}

void *timer(void *arg)
{
    while (1)
    {
        sleep(1);
        global_time++;
        pthread_cond_signal(&timer_expired);
    }
}

void schedule()
{
    pthread_t timer_thread;
    if (pthread_create(&timer_thread, NULL, timer, NULL) != 0)
    {
        fprintf(stderr, "Erro ao criar a thread do temporizador.\n");
        exit(EXIT_FAILURE);
    }

    for (int priority = 1; priority <= MAX_PRIORITY; priority++)
    {
        list_tasks_priority(priority);

        while (queues[priority - 1] != NULL)
        {
            Task *task = remove_next_task(priority);
            if (task != NULL)
            {
                int execution_time = (task->burst < TIME_SLICE) ? task->burst : TIME_SLICE;

                pthread_mutex_lock(&mutex);
                while (!global_time)
                {
                    pthread_cond_wait(&timer_expired, &mutex);
                }
                global_time = 0;
                pthread_mutex_unlock(&mutex);

                run(task, execution_time);
                task->burst -= execution_time;

                if (task->burst <= 0)
                {
                    printf("Tarefa %s concluída.\n", task->name);
                    free(task->name);
                    free(task);
                }
                else
                {
                    add(task->name, task->priority, task->burst);
                }
            }
        }
    }
    printf("Todas as tarefas foram concluídas.\n");
}