#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROCESSES 100
#define MAX_DEVICES 10
#define QUANTUM 4

typedef struct {
    int id;             // Identificador do processo
    int tInicio;        // Tempo de chegada
    int *tCpu;          // Vetor de tempos de CPU
    int *disp;          // Vetor de dispositivos (intercalado com tCpu)
    int numCycles;      // Número de ciclos CPU/I/O
    int estado;         // Estado do processo (0: new/ready, 1: waiting, 2: running, 3: finished)
    int currentCycle;   // Ciclo atual do processo
    int remainingTime;  // Tempo restante para finalizar o ciclo atual
} PCB;

typedef struct Node {
    PCB *process;
    struct Node *next;
} Node;

typedef struct {
    Node *front;
    Node *rear;
} Queue;

int nProc, nDisp;        // Número de processos e dispositivos
int tDisp[MAX_DEVICES];  // Tempos de atendimento de dispositivos
PCB processes[MAX_PROCESSES]; // Lista de processos

void readInput(const char *filename);

// Funções de fila
void initQueue(Queue *q);
void enqueue(Queue *q, PCB *process);
PCB *dequeue(Queue *q);
int isQueueEmpty(Queue *q);

void processIO(Queue *waitingQueue, Queue *readyQueue, int currentTime);

int main() {
    const char *filename = "../input.txt"; // Caminho fixo para o arquivo de entrada

    // Inicializar filas
    Queue readyQueue, waitingQueue;
    initQueue(&readyQueue);
    initQueue(&waitingQueue);

    readInput(filename);

    // Adicionar todos os processos à fila de prontos
    for (int i = 0; i < nProc; i++) {
        processes[i].estado = 0; // Estado inicial: new/ready
        enqueue(&readyQueue, &processes[i]);
    }

    // Simulação simples do escalonador
    printf("\nSimulacao do escalonador:\n");
    int currentTime = 0;

    while (!isQueueEmpty(&readyQueue) || !isQueueEmpty(&waitingQueue)) {
        // Processar I/O antes de executar ciclos de CPU
        processIO(&waitingQueue, &readyQueue, currentTime);

        // Selecionar o próximo processo para execução
        PCB *process = dequeue(&readyQueue);
        if (process) {
            printf("Tempo %d: Executando Processo %d (Ciclo Atual: %d):\n", currentTime, process->id, process->currentCycle);
            process->estado = 2; // Executando
            int remainingTime = process->remainingTime;

            if (remainingTime > QUANTUM) {
                printf("  Quantum utilizado: %d\n", QUANTUM);
                process->remainingTime -= QUANTUM;
                currentTime += QUANTUM;
                enqueue(&readyQueue, process);
            } else {
                printf("  Ciclo concluido em %d unidades de tempo\n", remainingTime);
                currentTime += remainingTime;
                process->currentCycle++;
                if (process->currentCycle < process->numCycles) {
                    // Enviar para a fila de espera (I/O)
                    printf("  Processo %d movido para a fila de espera.\n", process->id);
                    process->estado = 1;
                    enqueue(&waitingQueue, process);
                } else {
                    printf("  Processo %d finalizado.\n", process->id);
                    process->estado = 3; // Finalizado
                }
            }
        } else {
            // Avançar o tempo enquanto aguarda processos de I/O
            currentTime++;
        }
    }

    return 0;
}

// Função para processar I/O
void processIO(Queue *waitingQueue, Queue *readyQueue, int currentTime) {
    Node *current = waitingQueue->front;
    Node *prev = NULL;

    while (current) {
        PCB *process = current->process;
        int deviceTime = process->disp[process->currentCycle - 1]; // Tempo de dispositivo do ciclo anterior
        if (deviceTime <= currentTime) {
            printf("  Processo %d completou I/O e foi movido para a fila de prontos.\n", process->id);
            process->estado = 0; // Pronto
            enqueue(readyQueue, process);

            // Remover da fila de espera
            if (prev) {
                prev->next = current->next;
            } else {
                waitingQueue->front = current->next;
            }
            Node *temp = current;
            current = current->next;
            free(temp);
        } else {
            prev = current;
            current = current->next;
        }
    }

    // Atualizar o final da fila se necessário
    if (!waitingQueue->front) {
        waitingQueue->rear = NULL;
    }
}

// Função para ler o arquivo de entrada
void readInput(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Erro ao abrir o arquivo");
        exit(EXIT_FAILURE);
    }

    // Ler o número de processos e dispositivos
    if (fscanf(file, "%d %d", &nProc, &nDisp) != 2) {
        fprintf(stderr, "Erro ao ler o numero de processos e dispositivos.\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Ler tempos de dispositivos
    for (int i = 0; i < nDisp; i++) {
        if (fscanf(file, "%d", &tDisp[i]) != 1) {
            fprintf(stderr, "Erro ao ler os tempos dos dispositivos.\n");
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }

    // Ler os processos
    for (int i = 0; i < nProc; i++) {
        processes[i].id = i + 1;

        // Inicializar memória para os ciclos
        processes[i].tCpu = (int *)malloc(MAX_DEVICES * sizeof(int));
        processes[i].disp = (int *)malloc(MAX_DEVICES * sizeof(int));
        processes[i].numCycles = 0;
        processes[i].currentCycle = 0;

        // Ler tempo de chegada do processo
        if (fscanf(file, "%d", &processes[i].tInicio) != 1) {
            fprintf(stderr, "Erro ao ler o tempo de chegada do processo %d.\n", i + 1);
            fclose(file);
            exit(EXIT_FAILURE);
        }

        // Ler tempos de CPU e dispositivos intercalados
        char line[256];
        if (fgets(line, sizeof(line), file) == NULL) {
            fprintf(stderr, "Erro ao ler os ciclos do processo %d.\n", i + 1);
            fclose(file);
            exit(EXIT_FAILURE);
        }

        // Processar a linha de tempos
        char *token = strtok(line, " ");
        while (token != NULL) {
            int cpuTime = atoi(token); // Ler tempo de CPU
            processes[i].tCpu[processes[i].numCycles] = cpuTime;
            processes[i].remainingTime = cpuTime;

            token = strtok(NULL, " "); // Ler próximo token
            if (token != NULL) {
                int deviceTime = atoi(token); // Ler tempo do dispositivo
                processes[i].disp[processes[i].numCycles] = deviceTime;
                token = strtok(NULL, " "); // Avançar para o próximo ciclo
            } else {
                processes[i].disp[processes[i].numCycles] = -1; // Sem dispositivo associado
            }

            processes[i].numCycles++;

            // Verificar limite máximo de ciclos
            if (processes[i].numCycles >= MAX_DEVICES) {
                fprintf(stderr, "Número maximo de ciclos atingido para o processo %d.\n", i + 1);
                break;
            }
        }
    }

    fclose(file);
}

// Funções de fila
void initQueue(Queue *q) {
    q->front = q->rear = NULL;
}

void enqueue(Queue *q, PCB *process) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->process = process;
    newNode->next = NULL;
    if (q->rear == NULL) {
        q->front = q->rear = newNode;
    } else {
        q->rear->next = newNode;
        q->rear = newNode;
    }
}

PCB *dequeue(Queue *q) {
    if (q->front == NULL) return NULL;
    Node *temp = q->front;
    PCB *process = temp->process;
    q->front = q->front->next;
    if (q->front == NULL) q->rear = NULL;
    free(temp);
    return process;
}

int isQueueEmpty(Queue *q) {
    return q->front == NULL;
}
