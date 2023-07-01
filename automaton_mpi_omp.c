#include <stdio.h>
#include "./headers/automaton_mpi_omp.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include <mpi.h>
#include <omp.h>
#include <windows.h>

#define DIV_ROUND_CLOSEST(n, d) ((((n) < 0) == ((d) < 0)) ? (((n) + (d)/2)/(d)) : (((n) - (d)/2)/(d)))

/* PARALELO: MEMORIA DISTRIBUIDA */
int main (int argc, char** argv) {
    int rank, size;
    omp_set_num_threads(NUM_THREADS);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    time_t time_spent_seconds;
    if (rank == 0) time(&time_spent_seconds);
    double time_spent_clock = 0.0;
    clock_t myBegin = clock();

    srand(time(NULL));

    Node** grid = (Node**) malloc(HEIGHT * sizeof(Node*));
    for (int i = 0; i < HEIGHT; i++)
        grid[i] = (Node*) malloc(WIDTH * sizeof(Node));
    
    Node** gridAux = (Node**) malloc(HEIGHT * sizeof(Node*));
    for (int i = 0; i < HEIGHT; i++)
        gridAux[i] = (Node*) malloc(WIDTH * sizeof(Node));

    int myRows = getRowsPerProcess(rank, size);
    int myRange[myRows + (MOORE_RANGE * 2)];
    getRangePerProcess(rank, size, myRows, myRange);


    MPI_Datatype mpi_node_type;
    createNodeType(&mpi_node_type);

    if (rank == 0) {
        initGrid(grid);
    }

    int iteration = 0;

    while(iteration < ITERATIONS) {
        if (rank == 0 && !PERFORMANCE_MODE && !DEV_MODE) {
            printf("Ciclo %d.\n", iteration+1);
            /*if (iteration < 3) {
                fflush(stdin);
                getchar();
            }*/
        }
        MPI_Barrier(MPI_COMM_WORLD);
        
        divideAndSendGrid(grid, rank, size, mpi_node_type, myRows, myRange, gridAux);
        if (0 < PERFORMANCE_MODE && PERFORMANCE_MODE < 4) printTimeSpent(&myBegin, rank, iteration, 0, &time_spent_clock);
        MPI_Barrier(MPI_COMM_WORLD);

        applyRulesGrid(grid, gridAux, rank, size, myRows, myRange);
        if (0 < PERFORMANCE_MODE && PERFORMANCE_MODE < 4) printTimeSpent(&myBegin, rank, iteration, 1, &time_spent_clock);
        MPI_Barrier(MPI_COMM_WORLD);
        
        updateGrid(grid, gridAux, rank, size, myRows, myRange, mpi_node_type);
        if (0 < PERFORMANCE_MODE && PERFORMANCE_MODE < 4) printTimeSpent(&myBegin, rank, iteration, 2, &time_spent_clock);
        MPI_Barrier(MPI_COMM_WORLD);
        if (!DEV_MODE && !PERFORMANCE_MODE && rank == 0) showGrid(grid);
        iteration++;
    }

    clock_t myEnd = clock();
    time_spent_clock += (double)(myEnd - myBegin) / CLOCKS_PER_SEC;
    if (rank == 0) printf("Tiempo de Ejecucion Total: %lf segundos.\n", time_spent_clock);
    if (rank == 0) {
        time_t time_end;
        time(&time_end);
        printf("Tiempo de Ejecucion Total Con Time: %ld segundos.\n", (time_end - time_spent_seconds));
    }
    for (int i = 0; i < HEIGHT; i++)
        free(grid[i]);
    free(grid);

    for (int i = 0; i < HEIGHT; i++)
        free(gridAux[i]);
    free(gridAux);
    
    MPI_Type_free(&mpi_node_type);
    MPI_Finalize();

    return 0;
}



void initGrid(Node **grid) {
    #pragma omp parallel for schedule(dynamic) collapse(2)
    for (int i = 0; i < HEIGHT; i++) 
        for (int j = 0; j < WIDTH; j++) {
            switch (INIT_TYPE) {
                case 1: {
                    grid[i][j].temperature = (((double) rand() / RAND_MAX) * 272 * 2) - 271;
                }; break;
                case 2: {
                    if (i == (HEIGHT / 2) && j == (WIDTH / 2)) grid[i][j].temperature = (double) 272;
                    else grid[i][j].temperature = (double) 0;
                }; break;
                case 3: {
                    if (i == (HEIGHT / 2) && j == (WIDTH / 2)) grid[i][j].temperature = (double) -272;
                    else grid[i][j].temperature = (double) 0;
                }; break;
                case 4: {
                    int blopup = DIV_ROUND_CLOSEST(HEIGHT*1, 4);
                    int blopleft1 = DIV_ROUND_CLOSEST(WIDTH*1, 8);
                    int blopleft2 = DIV_ROUND_CLOSEST(WIDTH*2, 8);
                    int blopdown = DIV_ROUND_CLOSEST(HEIGHT*3, 4);
                    int blopright1 = DIV_ROUND_CLOSEST(WIDTH*6, 8);
                    int blopright2 = DIV_ROUND_CLOSEST(WIDTH*7, 8);
                    if (blopup <= i && i <= blopdown) {
                        if (blopleft1 <= j && j <= blopleft2) {
                            grid[i][j].temperature = (double) 272;
                        }
                        else if (blopright1 <= j && j <= blopright2) {
                            grid[i][j].temperature = (double) -272;
                        }
                        else grid[i][j].temperature = (double) 0;
                    }
                    else grid[i][j].temperature = (double) 0;
                }; break;
                default: {
                    grid[i][j].temperature = (((double) rand() / RAND_MAX) * 272 * 2) - 271;
                }; break;
            }
            grid[i][j].conductivity = (((double) rand() / RAND_MAX) * 0.5);
            if (RANDOMNESS == 0 || RANDOMNESS == 2) grid[i][j].sign = 1;
            else {
                if (((double) rand() / RAND_MAX) <= 0.5) grid[i][j].sign = -1;
                else grid[i][j].sign = 1;
            }
        }
}

void copyGrid(Node **originalGrid, Node **auxGrid) {
    for (int i = 0; i < HEIGHT; i++) 
        for (int j = 0; j < WIDTH; j++) 
            auxGrid[i][j] = originalGrid[i][j];
}

void divideAndSendGrid(Node **grid, int rank, int size, MPI_Datatype MPI_NODE_TYPE, int myRows, int myRange[], Node **gridAux) {
    MPI_Status status;

    if (rank == 0) {
        int rowsAux;

        for (int turn = 0; turn < size; turn++) {
            if (turn == 0) rowsAux = myRows;
            else MPI_Recv(&rowsAux, 1, MPI_INT, turn, 90, MPI_COMM_WORLD, &status);

            int rangeAux[(MOORE_RANGE * 2) + rowsAux];

            for (int i = 0; i < ((MOORE_RANGE * 2) + rowsAux); i++) {
                if (turn == 0) rangeAux[i] = myRange[i];
                else MPI_Recv(&rangeAux[i], 1, MPI_INT, turn, 85, MPI_COMM_WORLD, &status);
            }
            //ENVIO LAS FILAS DE LA GRID ORIGINAL A CADA PROCESO
            
                for (int i = 0; i < ((MOORE_RANGE * 2) + rowsAux); i++) {
                    if (inGrid(rangeAux[i], 0)) {
                        for (int j = 0; j < WIDTH; j++) {
                            gridAux[rangeAux[i]][j] = grid[rangeAux[i]][j];  
                        }
                        if (turn != 0) MPI_Send(gridAux[rangeAux[i]], WIDTH, MPI_NODE_TYPE, turn, 95, MPI_COMM_WORLD);
                    }
                }
        }
    } else {
        MPI_Send(&myRows, 1, MPI_INT, 0, 90, MPI_COMM_WORLD);

        for (int i = 0; i < ((MOORE_RANGE * 2) + myRows); i++)
            MPI_Send(&myRange[i], 1, MPI_INT, 0, 85, MPI_COMM_WORLD);
        
        //CADA PROCESO RECIBE LAS FILAS Y LAS AGREGA A SU GRID
        for (int i = 0; i < ((MOORE_RANGE * 2) + myRows); i++) {
            if (inGrid(myRange[i], 0)) {
                MPI_Recv(gridAux[myRange[i]], WIDTH, MPI_NODE_TYPE, 0, 95, MPI_COMM_WORLD, &status);
                
                for (int j = 0; j < WIDTH; j++) {
                    grid[myRange[i]][j] = gridAux[myRange[i]][j];  
                }
            } 
        }
    }

    if (DEV_MODE) {
        printf("\n\nTRANSFERENCIA DE FILAS DE MASTER A SLAVES:\nEN AZUL: LA GRID DESTINO ");
        if (rank == 0) printf("(el mismo master)\n\n");
        else printf("(la de los slaves)\n\n");
        printMyRows(grid, rank, myRows, myRange, 2);
    }
}

int getRowsPerProcess(int rank, int size) {
    int turno;
    int rows = 0;
    for (int i = 1; i <= HEIGHT; i++) {
        turno = i % size;
        if (turno == rank) rows++;
    }


    if (DEV_MODE) {
        if (rank == 0) printf("\n\nLOS PROCESOS TIENEN ASIGNADAS ESTA CANTIDAD DE FILAS:\n\n");
        int result = 0;
        MPI_Reduce(&rows, &result, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
        if (rank == 0) printf("Filas Asignadas: %d \t\tFilas Totales: %d\n", result, HEIGHT);
    } 
    return rows;
}

void getRangePerProcess(int rank, int size, int myRows, int myRange[]) {
    MPI_Status status;

    int rowsAcumulated = 0;
    int rowsDone = 0;
    int i = 0;
    for (int turn = 0; turn < size; turn++) {    
        if (rank == turn) {
            for (int row = -(MOORE_RANGE); row < ((MOORE_RANGE*2)+myRows)-(MOORE_RANGE); row++) {
                myRange[i] = (row+rowsAcumulated);
                i++;
            }
            rowsAcumulated += myRows;
            if (turn < size-1) MPI_Send(&rowsAcumulated, 1, MPI_INT, rank+1, 99,MPI_COMM_WORLD);
        }
        else if (rank == turn+1) {
            if (turn < size-1) MPI_Recv(&rowsDone, 1, MPI_INT, rank-1, 99, MPI_COMM_WORLD, &status);
            rowsAcumulated += rowsDone;
        }
    }
    //MOSTRAR LOS RANGOS DE CADA PROCESO (En rojo los out of bounds)
    
    if (DEV_MODE) {
        printf("\n\nMOSTRAR RANGOS DE CADA PROCESO CONTANDO EL RANGO DE MOORE:\nEN ROJO, LOS OUT OF BOUNDS\n\n");
        printf("rank: %2d, myRows: %2d, myRange: ", rank, myRows);
        for (int i = 0; i < (myRows + (MOORE_RANGE * 2)); i++) {
            if (inGrid(myRange[i], 0)) printf("\033[96m%2d \033[0m", myRange[i]);
            else printf("\033[31m%2d \033[0m",myRange[i]);
        }
        printf("\n");
    }
}

void createNodeType(MPI_Datatype *mpi_node_type) {
    const int parameters = 3;
    int blocklenghts[3] = {1, 1, 1};
    MPI_Datatype types[3] = {MPI_DOUBLE, MPI_DOUBLE, MPI_SHORT};
    MPI_Aint offsets[3];

    offsets[0] = offsetof(Node, temperature);
    offsets[1] = offsetof(Node, conductivity);
    offsets[2] = offsetof(Node, sign);

    MPI_Type_create_struct(parameters, blocklenghts, offsets, types, mpi_node_type);
    MPI_Type_commit(mpi_node_type);
}

int inGrid(int row, int column) {
    if (row < 0 || row >= HEIGHT) return 0;
    if (column < 0 || column >= WIDTH) return 0;
    return 1;   
}


void applyRulesGrid(Node **grid, Node **gridAux, int rank, int size, int myRows, int myRange[]) {
    if (DEV_MODE) {
        printf("\n\nAPLICAR REGLAS A CELDAS:\nEN AZUL OSCURO: TEMPERATURA ANTES DEL ESTADO ACTUAL, EN BLANCO: TEMPERATURA DESPUES DEL SIGUIENTE ESTADO\n\n");
        printMyRows(grid, rank, myRows, myRange, 3);
    }
    for (int i = 0; i < ((MOORE_RANGE * 2) + myRows); i++)
        if (inGrid(myRange[i], 0) && isMyRow(i, myRows)) {
            #pragma omp parallel for schedule(dynamic)
            for (int j = 0; j < WIDTH; j++) {
                applyRules(grid, gridAux, myRange[i], j);
            }
        }
    if (DEV_MODE) printMyRows(gridAux, rank, myRows, myRange, 4);
}

void applyRules(Node **grid, Node **gridAux, int i, int j) {
    gridAux[i][j].temperature = ruleA(grid, i, j);
    if (RANDOMNESS) gridAux[i][j].sign *= ruleB();

    if (gridAux[i][j].temperature < -272)
        gridAux[i][j].temperature = -272;
    if (gridAux[i][j].temperature > 272)
        gridAux[i][j].temperature = 272;
}

int isMyRow(int i, int myRows) {
    return (MOORE_RANGE <= i && i < MOORE_RANGE + myRows);
}

double ruleA(Node **grid, int i, int j) {
    double aux = 0;
    int neighbors = 0;

    for (int y = -MOORE_RANGE; y <= MOORE_RANGE; y++) 
        for (int x = -MOORE_RANGE; x <= MOORE_RANGE; x++) 
            if (!(x == 0 && y == 0) && inGrid(i + y, j + x)) {
                    neighbors++;
                    aux += grid[i+y][j+x].temperature *
                        ((grid[i+y][j+x].conductivity + grid[i][j].conductivity) /
                            pow(getDistance(x,y) + 2, 2));
                }
    
    double temperature = (grid[i][j].sign) * (1 / pow(neighbors, 2)) * (aux);
    if (GROWTH_FACTOR > 0) temperature *= GROWTH_FACTOR;
    temperature += grid[i][j].temperature;
    
    return temperature;
}

short ruleB() {
    if (((double) rand() / RAND_MAX) <= 0.1) return -1;
    return 1;
}

int getDistance(int x, int y) {
    for (int i = MOORE_RANGE; i >= 0; i--)
        if (x == i || x == -i || y == i || y == -i)
            return i;
}

void updateGrid(Node **grid, Node **gridAux, int rank, int size, int myRows, int myRange[], MPI_Datatype MPI_NODE_TYPE) {
    MPI_Status status;
    
    if (rank == 0) {
        int rowsAux;

        for (int turn = 0; turn < size; turn++) {
            if (turn == 0) rowsAux = myRows;
            else MPI_Recv(&rowsAux, 1, MPI_INT, turn, 70, MPI_COMM_WORLD, &status);

            int rangeAux[(MOORE_RANGE * 2) + rowsAux];

            for (int i = 0; i < ((MOORE_RANGE * 2) + rowsAux); i++) {
                if (turn == 0) rangeAux[i] = myRange[i];
                else MPI_Recv(&rangeAux[i], 1, MPI_INT, turn, 75, MPI_COMM_WORLD, &status);
            }

            for (int i = 0; i < ((MOORE_RANGE * 2) + rowsAux); i++) {
                if (inGrid(rangeAux[i], 0) && isMyRow(i, rowsAux)) {
                    if (turn != 0) MPI_Recv(gridAux[rangeAux[i]], WIDTH, MPI_NODE_TYPE, turn, 65, MPI_COMM_WORLD, &status);
                    
                    for (int j = 0; j < WIDTH; j++) {
                        grid[rangeAux[i]][j] = gridAux[rangeAux[i]][j];
                    }
                }
            }
        }
    }
    else {
        MPI_Send(&myRows, 1, MPI_INT, 0, 70, MPI_COMM_WORLD);

        for (int i = 0; i < ((MOORE_RANGE * 2) + myRows); i++)
            MPI_Send(&myRange[i], 1, MPI_INT, 0, 75, MPI_COMM_WORLD);
        
        for (int i = 0; i < ((MOORE_RANGE * 2) + myRows); i++) {
            if (inGrid(myRange[i], 0) && isMyRow(i, myRows)) { 
                MPI_Send(gridAux[myRange[i]], WIDTH, MPI_NODE_TYPE, 0, 65, MPI_COMM_WORLD);
            }
        }
    }
    if (DEV_MODE) {
        if (rank == 0) {
            printf("\n\nTRANSFERENCIA DE FILAS DE VUELTA A MASTER:\nEN ROJO: LA GRID ORIGEN (el mismo master)\n\n");
            printMyRows(grid, rank, myRows, myRange, 1);
        }
        else {
            printf("\n\nTRANSFERENCIA DE FILAS DE VUELTA A MASTER:\nEN ROJO: LAS GRID ORIGEN (de los slaves)\n\n");
            printMyRows(gridAux, rank, myRows, myRange, 1);
        }
    }
}

/*COLOR: 0=default, 1=RED, 2=BLUE, 3=DARK BLUE, 4=ORANGE*/
void printMyRows(Node **grid, int rank, int myRows, int myRange[], int color) {
    for (int i = 0; i < ((MOORE_RANGE * 2) + myRows); i++) {
        if (inGrid(myRange[i], 0) && isMyRow(i, myRows)) {
            switch (color) {
            case 1: printf("\033[31m%2d: grid[%2d][0]: %11lf\n\033[0m", rank, myRange[i], grid[myRange[i]][0].temperature);
                break;
            case 2: printf("\033[96m%2d: grid[%2d][0]: %11lf\n\033[0m", rank, myRange[i], grid[myRange[i]][0].temperature);
                break;
            case 3: printf("\033[34m%2d: grid[%2d][0]: %11lf\n\033[0m", rank, myRange[i], grid[myRange[i]][0].temperature);
                break;
            default: printf("%2d: grid[%2d][0]: %11lf\n", rank, myRange[i], grid[myRange[i]][0].temperature);
                break;
            }
            
        }
    }
}

void showGrid(Node **grid) {
    for (int i = 0; i < (HEIGHT > 35 ? 35 : HEIGHT) ; i++) {
        for (int j = 0; j < (WIDTH > 35 ? 35 : WIDTH); j++) {
            if (-272 <= grid[i][j].temperature && grid[i][j].temperature < -163) // LIGHT CYAN  [-272,-163)
                printf("\033[96m%c \033[0m", (char)254u);
            else if (-163 <= grid[i][j].temperature && grid[i][j].temperature < -54) // BLUE [-163,-54)
                printf("\033[34m%c \033[0m", (char)254u);
            else if (-54 <= grid[i][j].temperature && grid[i][j].temperature < 55) // WHITE [-54, 55)
                printf("\033[97m%c \033[0m", (char)254u);
            else if (55 <= grid[i][j].temperature && grid[i][j].temperature < 164) //LIGH RED / ORANGE [55, 164)
                printf("\033[91m%c \033[0m", (char)254u);
            else if (164 <= grid[i][j].temperature && grid[i][j].temperature <= 272) //RED [164, 272]
                printf("\033[31m%c \033[0m", (char)254u);
            else {
                printf("!!!"); 
                exit(0);
            }
        }
        printf("\n");
    }
}
/* TASK: 0 = Divide grill, 1 = Apply Rules, 2 = Update Grid */
void printTimeSpent(clock_t *myBegin, int rank, int iteration, int task, double *time_spent) {
    double time_iteration;
    clock_t myEnd = clock();
    *time_spent += (double)(myEnd - *myBegin) / CLOCKS_PER_SEC;
    time_iteration = (double)(myEnd - *myBegin) / CLOCKS_PER_SEC;

    if ((PERFORMANCE_MODE == 2 && rank == 0) || (PERFORMANCE_MODE == 3 && iteration == (ITERATIONS-1) && rank == 0) || PERFORMANCE_MODE == 1) {
        switch (task) {
                case 0:
                    if (rank == 0) printf("Iteracion %5d: \033[31mMASTER\033[0m   (%2d), tardo %11lf segundos en dividir la grilla.\n", iteration+1, rank, time_iteration);
                    ; break;
                case 1:
                    printf("Iteracion %5d: El proceso %2d, tardo %11lf segundos en aplicar reglas a sus filas.\n", iteration+1, rank, time_iteration);
                    ; break;
                case 2:
                    if (rank == 0) printf("Iteracion %5d: \033[31mMASTER\033[0m   (%2d), tardo %11lf segundos en actualizar la grilla.\n", iteration+1, rank, time_iteration);
                    ; break;
                default: break;
            }
        //if (iteration == (ITERATIONS - 1) && task == 2 && rank == 0) printf("\033[32mTiempo de Ejecucion Total: %lf segundos.\n\033[0m", *time_spent);
    }
    *myBegin = myEnd;
}
