#ifndef SIMULATION_H
#define SIMULATION_H

#include "config.h"

typedef struct {
    double p_hit_within_K;
    double avg_steps_to_hit;
} sim_jedno_policko;

typedef struct {
sim_jedno_policko stats;
int x;
int y;
}policko_data;

typedef struct  {
policko_data *policko;        
server_config *config;
unsigned long long  maxKrokov;
}vlakno_args;

void sim_step(const server_config *cfg, int *x, int *y);

sim_jedno_policko sim_simuluj_policko(const server_config *cfg, int sx, int sy, int K, unsigned long max_steps);

int sim_dojst_do_stredu_za_K(const server_config *cfg, int sx, int sy, int K);

void* sim_policko_vlakno(void* arg);

unsigned long sim_kolko_krokov_kym_trafim(const server_config *cfg, int sx, int sy, unsigned long max_steps);

void sim_vypocitaj_maticu(
    const server_config *cfg,
                        
    policko_data *vystup_matica  
);

void sim_vypis_maticu(
    const server_config *cfg,
    const policko_data *matica,int type
);

#endif
