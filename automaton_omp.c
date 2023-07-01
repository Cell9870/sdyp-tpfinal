#include <stdio.h>
#include "./headers/automaton_omp.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include <omp.h>

#define DIV_ROUND_CLOSEST(n, d) ((((n) < 0) == ((d) < 0)) ? (((n) + (d)/2)/(d)) : (((n) - (d)/2)/(d)))

/* OpenMP */
int main (int argc, char** argv) {
    omp_set_num_threads(NUM_THREADS);

    time_t time_spent_seconds;
    time(&time_spent_seconds);
    double time_spent_clock = 0.0;
    clock_t begin = clock();
    
    Node** grid = (Node**) malloc(HEIGHT * sizeof(Node*));
    for (int i = 0; i < HEIGHT; i++)
        grid[i] = (Node*) malloc(WIDTH * sizeof(Node));

    Node** gridAux = (Node**) malloc(HEIGHT * sizeof(Node*));
    for (int i = 0; i < HEIGHT; i++)
        gridAux[i] = (Node*) malloc(WIDTH * sizeof(Node));
    
    srand(time(NULL));
    initGrid(grid);
    copyGrid(grid, gridAux);

    short finish = 0;

    while (finish < ITERATIONS) {
        if (!PERFORMANCE_MODE) {
            printf("Ciclo %d.\n", finish+1);
            /*if (finish < 2) {
                fflush(stdin);
                getchar();
            }*/
        }

        applyRulesGrid(grid, gridAux);
        updateGrid(grid, gridAux);
        if (!PERFORMANCE_MODE) showGrid(grid);
        if (!PERFORMANCE_MODE && DEV_MODE) showTemperatures(grid);
        finish++;
    }


    clock_t end = clock();
    time_spent_clock += (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Tiempo de Ejecucion: %lf segundos.\n", time_spent_clock);
    time_t time_end;
    time(&time_end);
    printf("Tiempo de Ejecucion Total Con Time: %ld segundos.\n", (time_end - time_spent_seconds));

    for (int i = 0; i < HEIGHT; i++)
        free(grid[i]);
    free(grid);

    for (int i = 0; i < HEIGHT; i++)
        free(gridAux[i]);
    free(gridAux);

    return 0;
}

void initGrid(Node **grid) {
    int blopleft1, blopright1, blopup, blopdown, blopleft2, blopright2;
    if (INIT_TYPE == 4) {
        blopup = DIV_ROUND_CLOSEST(HEIGHT*1, 4);
        blopleft1 = DIV_ROUND_CLOSEST(WIDTH*1, 8);
        blopleft2 = DIV_ROUND_CLOSEST(WIDTH*2, 8);
        blopdown = DIV_ROUND_CLOSEST(HEIGHT*3, 4);
        blopright1 = DIV_ROUND_CLOSEST(WIDTH*6, 8);
        blopright2 = DIV_ROUND_CLOSEST(WIDTH*7, 8);
    }

    #pragma omp parallel for schedule(dynamic) collapse(2)
    for (int i = 0; i < HEIGHT; i++) 
        for (int j = 0; j < WIDTH; j++) {
            switch (INIT_TYPE) {
                case 1: { // RANDOM
                        grid[i][j].temperature = (((double) rand() / RAND_MAX) * 272 * 2) - 271;
                    }; break;
                case 2: { // HOT CENTER
                    if (i == (HEIGHT / 2) && j == (WIDTH / 2)) grid[i][j].temperature = (double) 272;
                    else grid[i][j].temperature = (double) 0;
                }; break;
                case 3: { // COLD CENTER
                    if (i == (HEIGHT / 2) && j == (WIDTH / 2)) grid[i][j].temperature = (double) -272;
                    else grid[i][j].temperature = (double) 0;
                }; break;
                case 4: { // COLD VS HOT BLOPS
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
                default: { //RANDOM
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

void applyRulesGrid(Node **grid, Node **gridAux) {
    #pragma omp parallel for schedule(dynamic) collapse(2)
    for (int i = 0; i < HEIGHT; i++) 
        for (int j = 0; j < WIDTH; j++) 
            applyRules(grid, gridAux, i, j);
}

void applyRules(Node **grid, Node **gridAux, int i, int j) {
    //printf("temperature %11lf\n", grid[i][j].temperature);
    gridAux[i][j].temperature = ruleA(grid, i, j);
    if (RANDOMNESS) gridAux[i][j].sign *= ruleB();

    
    if (gridAux[i][j].temperature < -272) 
        gridAux[i][j].temperature = -272;
    if (gridAux[i][j].temperature > 272) 
        gridAux[i][j].temperature = 272;

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


void updateGrid(Node **grid, Node **gridAux) {
    copyGrid(gridAux, grid);
}

int getDistance(int x, int y) {
    for (int i = MOORE_RANGE; i >= 0; i--)
        if (x == i || x == -i || y == i || y == -i)
            return i;
}

int inGrid(int row, int column) {
    if (row < 0 || row >= HEIGHT) return 0;
    if (column < 0 || column >= WIDTH) return 0;
    return 1;   
}

void copyGrid(Node **originalGrid, Node **auxGrid) {
    #pragma omp parallel for schedule(dynamic) collapse(2)
    for (int i = 0; i < HEIGHT; i++) 
        for (int j = 0; j < WIDTH; j++) 
            auxGrid[i][j] = originalGrid[i][j];
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

void showTemperatures(Node **grid) {
    for (int i = 0; i < HEIGHT; i++) {
        if (i % 100 == 0) {
            for (int j = 0; j < WIDTH; j++) {
                printf("grid[%3d][%3d]: %11lf\n", i, j, grid[i][j].temperature);
            }
        }
    }
}