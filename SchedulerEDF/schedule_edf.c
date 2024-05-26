#include "schedule_edf.h"
#include "list.h"
#include "task.h"
#include "CPU.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define INTERVALO_TEMPO 10 // Intervalo de tempo em unidades de tempo

// Estrutura da fila de tarefas prontas como uma lista de listas de prioridades
struct node *filaTarefas[MAX_PRIORITY] = {NULL};

// Variáveis globais para simulação do tempo
int tempoGlobal = 0;
pthread_mutex_t mutexTempo = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condTempo = PTHREAD_COND_INITIALIZER;
int tempoExpirado = 0; // Flag de expiração do tempo

// Função para calcular a folga de uma tarefa
int calcularIndicePrioridade(Task *tarefa)
{
   int tempo_limite = tarefa->arrival_time + tarefa->deadline;
   int tempoEstimadoConclusao = tempoGlobal + tarefa->burst;
   int folga = tempo_limite - tempoEstimadoConclusao;
   return (folga > 0) ? tarefa->priority - 1 : 0;
}

// Função para adicionar uma tarefa à fila de prontas
void add(char *nome, int prioridade, int tempo_execucao, int deadline)
{
   Task *novaTarefa = malloc(sizeof(Task));
   novaTarefa->name = strdup(nome);
   novaTarefa->priority = prioridade;
   novaTarefa->burst = tempo_execucao;
   novaTarefa->deadline = deadline;
   novaTarefa->arrival_time = tempoGlobal;

   int indice_prioridade = calcularIndicePrioridade(novaTarefa);
   insert_end(&filaTarefas[indice_prioridade], novaTarefa);
}

// Função de simulação do temporizador
void *temporizador(void *arg)
{
   while (1)
   {
      sleep(1); // Simula 1 unidade de tempo
      pthread_mutex_lock(&mutexTempo);
      tempoGlobal += INTERVALO_TEMPO;
      tempoExpirado = 1; // Sinaliza expiração do tempo
      pthread_cond_signal(&condTempo);
      pthread_mutex_unlock(&mutexTempo);
   }
}

// Função para reorganizar a fila de tarefas prontas de acordo com os deadlines
void reorganizarFilaTarefas()
{
   struct node *novaFilaTarefas[MAX_PRIORITY] = {NULL};

   for (int i = 0; i < MAX_PRIORITY; i++)
   {
      while (filaTarefas[i] != NULL)
      {
         struct node *temp = filaTarefas[i];
         filaTarefas[i] = filaTarefas[i]->next;

         int indice_prioridade = calcularIndicePrioridade(temp->task);
         temp->task->priority = indice_prioridade + 1; // Define a nova prioridade no caso de reordenação
         insert(&novaFilaTarefas[indice_prioridade], temp->task);
         free(temp);
      }
   }

   for (int i = 0; i < MAX_PRIORITY; i++)
   {
      filaTarefas[i] = novaFilaTarefas[i];
   }
}

// Função para escolher a próxima tarefa a ser executada
Task *selecionarProximaTarefa()
{
   reorganizarFilaTarefas(); // Reorganiza a fila antes de selecionar a próxima tarefa

   for (int i = 0; i < MAX_PRIORITY; i++)
   {
      if (filaTarefas[i] != NULL)
      {
         struct node *temp = filaTarefas[i];
         filaTarefas[i] = temp->next;
         Task *tarefa = temp->task;
         free(temp);
         return tarefa;
      }
   }
   return NULL;
}

// Função principal de escalonamento
void schedule()
{
   pthread_t thread_temporizador;
   pthread_create(&thread_temporizador, NULL, temporizador, NULL);
   reorganizarFilaTarefas();
   while (1)
   {
      // Sincroniza com o temporizador
      pthread_mutex_lock(&mutexTempo);
      while (tempoExpirado == 0)
      {
         pthread_cond_wait(&condTempo, &mutexTempo);
      }
      tempoExpirado = 0;
      pthread_mutex_unlock(&mutexTempo);

      Task *tarefa = selecionarProximaTarefa();
      if (tarefa != NULL)
      {
         int tempoExecucao = (tarefa->burst < INTERVALO_TEMPO) ? tarefa->burst : INTERVALO_TEMPO;
         run(tarefa, tempoExecucao);
         tarefa->burst -= tempoExecucao;

         int tempo_limite = tarefa->arrival_time + tarefa->deadline;
         if (tempoGlobal > tempo_limite)
         {
            printf("AVISO: Deadline não cumprido para a tarefa %s | Tempo global %d > Tempo limite %d.\n\n", tarefa->name, tempoGlobal, tempo_limite);
         }

         if (tarefa->burst > 0)
         {
            int indice_prioridade = calcularIndicePrioridade(tarefa);
            insert(&filaTarefas[indice_prioridade], tarefa);
         }
         else
         {
            printf("Tarefa %s concluída.\n\n", tarefa->name);
            free(tarefa->name);
            free(tarefa);
         }
      }
      else
      {
         printf("Todas as tarefas foram concluídas.\n");
         break;
      }
   }
}
