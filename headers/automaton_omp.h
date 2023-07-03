
typedef struct {
    double temperature;     // Variable que toma valores desde -272 hasta 272
    double conductivity;    // Variable que toma valores entre [0 ; 0.5]
    short sign;             // Variable que puede ser 1 o -1
} Node;

// 1: No mostrar grilla, 0: Mostrar grilla.
#define PERFORMANCE_MODE 1
// 1: Activar comentarios, 0: No mostrar ayudas.
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
// Numero de threads
#define NUM_THREADS 4

void initGrid(Node**);
void applyRulesGrid(Node**, Node**);
void applyRules(Node**, Node**, int, int);
double ruleA(Node **grid, int i, int j);
short ruleB();
void updateGrid(Node**, Node**);
int getDistance(int, int);
int inGrid(int, int);
void copyGrid(Node**, Node**);
void showGrid(Node**);
void showTemperatures(Node**);
//void showGridAux(Node[HEIGHT][WIDTH]);
