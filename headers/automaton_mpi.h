#include <mpi.h>
#include <time.h>

typedef struct {
    double temperature;     // Variable que toma valores desde -272 hasta 272
    double conductivity;    // Variable que toma valores entre [0 ; 0.5]
    short sign;             // Variable que puede ser 1 o -1
} Node;

// 0: Mostrar grilla; 1: No mostrar grilla, mostrar timings; 2: Solo timings de MASTER; 3: Solo timings iteracion final de MASTER; 4: No mostrar timings
#define PERFORMANCE_MODE 4
// 0: No mostrar ayudas, 1: Activar comentarios.
#define DEV_MODE 0
// Número de iteraciones 
#define ITERATIONS 25
// Cantidad de columnas en la grilla (columns)
#define WIDTH 5000
// Cantidad de filas en la grilla (rows)
#define HEIGHT 5000

// Rango de Moore
#define MOORE_RANGE 2
// Cantidad de vecinos, sin contarse a sí mismo
#define NEIGHBORS ((2*MOORE_RANGE+1)*(2*MOORE_RANGE+1))-1 
// Factor de crecimiento
#define GROWTH_FACTOR 1
// Factor de aleatoridad: 0 = No ruleB, and no random sign. 1: RuleB and random sign. 2: RuleB and no random sign
#define RANDOMNESS 1
// Tipo de inicializacion de grilla: 1 = RANDOM, 2 = HOT CENTER, 3 = COLD CENTER, 4 = COLD VS HOT BLOPS
#define INIT_TYPE 1
// Ejecucion en cluster
#define CLUSTER 0

void initGrid(Node**);
void applyRulesGrid(Node**, Node**, int, int, int, int[]);
void applyRules(Node**, Node**, int, int);
double ruleA(Node**, int, int);
short ruleB();
void updateGrid(Node**, Node**, int, int, int, int[], MPI_Datatype);
int getDistance(int, int);
int inGrid(int, int);
void copyGrid(Node**, Node**);
void showGrid(Node**);
void divideAndSendGrid(Node**, int, int, MPI_Datatype, int, int[], Node**);
int getRowsPerProcess(int, int);
void getRangePerProcess(int, int, int, int[]);
void createNodeType(MPI_Datatype*);
int isMyRow(int, int);
void printMyRows(Node**, int, int, int[], int);
void printTimeSpent(clock_t*, int, int, int, double*);
