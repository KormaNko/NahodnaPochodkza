#include "simulation.h"

#include <stdlib.h> 
#include <pthread.h>
#include <stdio.h>
#define MAX_POCET_KROKOV 100000 // aby som tam nahodou neostal stucknuty vecne



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

void* sim_policko_vlakno(void* arg) {
    vlakno_args * data = (vlakno_args*) arg;
    sim_jedno_policko vysledok = sim_simuluj_policko(data->config,data->policko->x,data->policko->y,data->config->K,data->maxKrokov);
    data->policko->stats = vysledok;
    return NULL;
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

    sim_jedno_policko sim_simuluj_policko(const server_config *cfg, int sx, int sy, int K, unsigned long max_steps) {
        sim_jedno_policko policko;
    
        int trafil = 0;
        unsigned long long suma_krokov = 0;


        for (int i = 0; i < cfg->R; i++)
        {
            
            trafil += sim_dojst_do_stredu_za_K(cfg,sx,sy,K);
            suma_krokov += sim_kolko_krokov_kym_trafim(cfg,sx,sy,max_steps);

        } 
        policko.avg_steps_to_hit = (double)suma_krokov /cfg->R;
        policko.p_hit_within_K =    (double)trafil /cfg->R;
       
        return policko;
    }

    void sim_vypocitaj_maticu(const server_config *cfg, policko_data *vystup_matica) {
    unsigned long long pocetPolicok = cfg->vyska * cfg->sirka;
    pthread_t vlakna[pocetPolicok];
    vlakno_args vlaknaData[pocetPolicok];

    for (unsigned long long i = 0; i < pocetPolicok; i++) {
        int x = i % cfg->sirka;
        int y = i / cfg->sirka;

        vystup_matica[i].x = x;
        vystup_matica[i].y = y;

        vlaknaData[i].config   = cfg;
        vlaknaData[i].maxKrokov = MAX_POCET_KROKOV;
        vlaknaData[i].policko  = &vystup_matica[i];

        pthread_create(&vlakna[i], NULL, sim_policko_vlakno, &vlaknaData[i]);
    }

    for (unsigned long long i = 0; i < pocetPolicok; i++) {
        pthread_join(vlakna[i], NULL);
    }
}

void sim_vypis_maticu(const server_config *cfg, const policko_data *matica, int type) {
    for (int y = 0; y < cfg->vyska; ++y) {
        for (int x = 0; x < cfg->sirka; ++x) {
            int idx = y * cfg->sirka + x;
            if (type == 0) {
                printf("%5.3lf ", matica[idx].stats.p_hit_within_K);
            } else {
                printf("%5.2lf ", matica[idx].stats.avg_steps_to_hit);
            }
        }
        printf("\n");
    }
}



