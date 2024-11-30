#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROCESSES 100
#define MAX_DEVICES 10
#define QUANTUM 4

typedef struct {
    int id;
    int tInicio;
    int *tCpu;
    int *disp;
    int numCycles;
    int estado;          // 0: new/ready, 1: waiting, 2: running, 3: finished
    int currentCycle;
    int remainingTime;
    int waitingTime;     // Tempo na fila ready
    int deviceTime;      // Tempo nos dispositivos
    int throughput;      // Tempo total desde a submissão até a conclusão
} PCB;

typedef struct Node {
    PCB *process;
    struct Node *next;
} Node;

typedef struct {
    Node *front;
    Node *rear;
} Queue;

int nProc, nDisp;
int tDisp[MAX_DEVICES];
PCB processes[MAX_PROCESSES];

// Funções auxiliares

void readInput(const char *filename);
void recordTrace(char **trace, int *traceIndex, int currentTime);
void initQueue(Queue *q);
void enqueue(Queue *q, PCB *process);
PCB *dequeue(Queue *q);
int isQueueEmpty(Queue *q);
void processIO(Queue *waitingQueue, Queue *readyQueue, int currentTime);
void saveOutput(const char *filename, int totalIdleTime, int currentTime, char **trace, int traceLength);

int main() {
    const char *filename = "../input.txt";
    const char *outputFilename = "../output.txt";

    Queue readyQueue, waitingQueue;
    initQueue(&readyQueue);
    initQueue(&waitingQueue);

    readInput(filename);

    char **trace = (char **)malloc(10000 * sizeof(char *));
    int traceIndex = 0;

    int totalIdleTime = 0;
    int currentTime = 0;

    for (int i = 0; i < nProc; i++) {
        processes[i].estado = 0;
        processes[i].waitingTime = 0;
        processes[i].deviceTime = 0;
        enqueue(&readyQueue, &processes[i]);
    }

    while (!isQueueEmpty(&readyQueue) || !isQueueEmpty(&waitingQueue)) {
        recordTrace(trace, &traceIndex, currentTime); // Registrar estado

        processIO(&waitingQueue, &readyQueue, currentTime);

        PCB *process = dequeue(&readyQueue);
        if (process) {
            process->estado = 2; // Executando
            int remainingTime = process->remainingTime;

            if (remainingTime > QUANTUM) {
                process->remainingTime -= QUANTUM;
                currentTime += QUANTUM;
                enqueue(&readyQueue, process);
            } else {
                currentTime += remainingTime;
                process->currentCycle++;
                if (process->currentCycle < process->numCycles) {
                    process->estado = 1;
                    enqueue(&waitingQueue, process);
                } else {
                    process->estado = 3; // Finalizado
                    process->throughput = currentTime - process->tInicio;
                }
            }
        } else {
            totalIdleTime++;
            currentTime++;
        }

        Node *node = readyQueue.front;
        while (node) {
            node->process->waitingTime++;
            node = node->next;
        }
    }

    saveOutput(outputFilename, totalIdleTime, currentTime, trace, traceIndex);

    for (int i = 0; i < traceIndex; i++) {
        free(trace[i]);
    }
    free(trace);

    return 0;
}
// Função para processar I/O
void processIO(Queue *waitingQueue, Queue *readyQueue, int currentTime) {
    Node *current = waitingQueue->front;
    Node *prev = NULL;

    while (current) {
        PCB *process = current->process;
        int deviceIndex = process->disp[process->currentCycle - 1];
        if (deviceIndex >= 0 && deviceIndex < nDisp) {
            process->deviceTime += tDisp[deviceIndex];
        }

        if (deviceIndex > 0) {
            deviceIndex--;
        }

        if (deviceIndex == 0) {
            process->estado = 0; // Pronto
            enqueue(readyQueue, process);

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

    if (!waitingQueue->front) {
        waitingQueue->rear = NULL;
    }
}

// Função para salvar a saída em arquivo
void saveOutput(const char *filename, int totalIdleTime, int currentTime, char **trace, int traceLength) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Erro ao criar o arquivo de saída");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "Trace:\n");
    for (int t = 0; t < traceLength; t++) {
        fprintf(file, "%s\n", trace[t]);
    }

    fprintf(file, "\nMetrics:\n");
    for (int i = 0; i < nProc; i++) {
        fprintf(file, "P%02d device time", processes[i].id);
        for (int d = 0; d < nDisp; d++) {
            int deviceTime = (d < processes[i].numCycles) ? processes[i].disp[d] : 0;
            fprintf(file, " d%d: %d,", d + 1, deviceTime);
        }
        fprintf(file, " waiting time: %d,", processes[i].waitingTime);
        fprintf(file, " throughput: %d\n", processes[i].throughput);
    }
    fprintf(file, "CPU idle time: %d\n", totalIdleTime);

    fclose(file);
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

        // Inicializar o vetor disp com -1
        for (int j = 0; j < MAX_DEVICES; j++) {
            processes[i].disp[j] = -1;
        }

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
void recordTrace(char **trace, int *traceIndex, int currentTime) {
    char buffer[1024];
    sprintf(buffer, "<%02d> | ", currentTime);

    for (int i = 0; i < nProc; i++) {
        char state[256];
        switch (processes[i].estado) {
            case 0:
                sprintf(state, "P%02d state: new/ready", processes[i].id);
                break;
            case 1:
                if (processes[i].currentCycle < processes[i].numCycles && processes[i].disp[processes[i].currentCycle] >= 0) {
                    sprintf(state, "P%02d state: blocked d%d", processes[i].id, processes[i].disp[processes[i].currentCycle]);
                } else {
                    sprintf(state, "P%02d state: blocked", processes[i].id);
                }
                break;
            case 2:
                sprintf(state, "P%02d state: running", processes[i].id);
                break;
            case 3:
                sprintf(state, "P%02d state: terminated", processes[i].id);
                break;
            default:
                sprintf(state, "P%02d state: --", processes[i].id);
                break;
        }
        strcat(buffer, state);
        strcat(buffer, " | ");
    }

    trace[*traceIndex] = (char *)malloc(strlen(buffer) + 1);
    strcpy(trace[*traceIndex], buffer);
    (*traceIndex)++;
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
