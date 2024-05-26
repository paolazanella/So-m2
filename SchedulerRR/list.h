/**
 * list data structure containing the tasks in the system
 */

#include "task.h"

typedef struct node
{
    Task *task;
    struct node *next;
} Node; // estrutura Node é usada para criar uma lista encadeada de tarefas.


// insert and delete operations.
void insert(struct node **head, Task *task);
void delete(struct node **head, Task *task);
void traverse(struct node *head);
void insert_end(struct node **head, Task *task);