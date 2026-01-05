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



int sim_dojst_do_stredu_za_K(const server_config *cfg, int sx, int sy, int K) {
   
    int x = sx, y = sy;
    
    if(x == 0 && y == 0) {
        return 1;
    }
    int i = 1;
    while ( i <= K) {
        sim_step(cfg,&x,&y);
        if(x == 0 && y == 0) {
        return 1;
    }
    i++;
    }
    return 0;
}

unsigned long sim_kolko_krokov_kym_trafim(const server_config *cfg, int sx, int sy, unsigned long max_steps) {
    
    int x = sx, y = sy;
    if(x == 0 && y == 0) {
        return 0;
    }
    unsigned long i = 1;
    while ( i < max_steps) {

        sim_step(cfg,&x,&y);
        if(x == 0 && 0 == y) {
        return i;
    }
    i++;
    }
    return max_steps;
}

    sim_jedno_policko sim_simuluj_policko(const server_config *cfg, int sx, int sy, int K, int R, unsigned long max_steps) {
        sim_jedno_policko policko;
    
        int trafil = 0;
        unsigned long long suma_krokov = 0;


        for (size_t i = 0; i < R; i++)
        {
            
            trafil += sim_dojst_do_stredu_za_K(cfg,sx,sy,K);
            suma_krokov += sim_kolko_krokov_kym_trafim(cfg,sx,sy,max_steps);

        } 
        policko.avg_steps_to_hit = (double)suma_krokov /R;
        policko.p_hit_within_K =    (double)trafil /R;
       
        return policko;
    }



