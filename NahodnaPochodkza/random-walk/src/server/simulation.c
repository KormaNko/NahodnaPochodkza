#include "simulation.h"

#include <stdlib.h> 

void sim_step(const server_config *cfg, int *x, int *y) {
    double r = rand() / (double)RAND_MAX;

    if (r < cfg->pU) {
        (*y) -= 1;
    } else if (r < cfg->pU + cfg->pD) {
        (*y) += 1;
    } else if (r < cfg->pU + cfg->pD + cfg->pL) {
        (*x) -= 1;
    } else {
        (*x) += 1;
    }

   
    *x = (*x % cfg->sirka + cfg->sirka) % cfg->sirka;
    *y = (*y % cfg->vyska + cfg->vyska) % cfg->vyska;
}

static void sim_get_center(const server_config *cfg, int *cx, int *cy) {
*cx = cfg->sirka /2;
*cy = cfg->vyska /2;
}

