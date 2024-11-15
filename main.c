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
    int estado;         // Estado do processo (0: new/ready, 1: ready, etc.)
    int currentCycle;   // Ciclo atual do processo
} PCB;

int nProc, nDisp;        // Número de processos e dispositivos
int tDisp[MAX_DEVICES];  // Tempos de atendimento de dispositivos
PCB processes[MAX_PROCESSES]; // Lista de processos

void readInput();

int main() {
    const char *filename = "../input.txt";// Caminho fixo para o arquivo de entrada na raiz do projeto MUDAR CONFORME A IDE 

    readInput(filename);

    // Exibir os dados lidos para teste
    printf("Numero de processos: %d\n", nProc);
    printf("Numero de dispositivos: %d\n", nDisp);
    for (int i = 0; i < nDisp; i++) {
        printf("Tempo de atendimento do dispositivo %d: %d\n", i + 1, tDisp[i]);
    }
    for (int i = 0; i < nProc; i++) {
        printf("Processo %d:\n", processes[i].id);
        printf("  tInicio: %d\n", processes[i].tInicio);
        printf("  Ciclos (CPU/I/O):\n");
        for (int j = 0; j < processes[i].numCycles; j++) {
            printf("    CPU: %d, Dispositivo: %d\n",
                   processes[i].tCpu[j],
                   processes[i].disp[j]);
        }
    }

    return 0;
}

void readInput(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Erro ao abrir o arquivo");
        exit(EXIT_FAILURE);
    }

    // Ler o número de processos e dispositivos
    fscanf(file, "%d %d", &nProc, &nDisp);

    // Ler tempos de dispositivos
    for (int i = 0; i < nDisp; i++) {
        fscanf(file, "%d", &tDisp[i]);
    }

    // Ler processos
    for (int i = 0; i < nProc; i++) {
        processes[i].id = i + 1;
        fscanf(file, "%d", &processes[i].tInicio); // Tempo de chegada

        // Alocar memória para os ciclos
        processes[i].tCpu = (int *)malloc(MAX_DEVICES * sizeof(int));
        processes[i].disp = (int *)malloc(MAX_DEVICES * sizeof(int));

        // Ler ciclos CPU/I/O
        processes[i].numCycles = 0;
        while (1) {
            int cpuTime, device;
            if (fscanf(file, "%d", &cpuTime) != 1) break;
            processes[i].tCpu[processes[i].numCycles] = cpuTime;

            if (fscanf(file, "%d", &device) == 1) {
                processes[i].disp[processes[i].numCycles] = device;
            } else {
                processes[i].disp[processes[i].numCycles] = -1; // Finaliza sem I/O
                break;
            }
            processes[i].numCycles++;
        }

        processes[i].estado = 0; // Estado inicial: new/ready
        processes[i].currentCycle = 0;
    }

    fclose(file);
}
