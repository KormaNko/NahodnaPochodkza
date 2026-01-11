#ifndef SIMULATION_H
#define SIMULATION_H

#include "config.h"
// v tejto triede bol pekny borel nech som to AI poupratovat
typedef struct {
    double p_hit_within_K;
    double avg_steps_to_hit;
} sim_jedno_policko;

typedef struct {
    sim_jedno_policko stats;
    int x;
    int y;
} policko_data;

typedef struct {
    const server_config *cfg;
    const int *prekazky;  
} simulation_context;

void sim_step(
    const simulation_context *ctx,
    int *x,
    int *y
);

int sim_dojst_do_stredu_za_K(
    const simulation_context *ctx,
    int sx,
    int sy,
    int K
);

unsigned long sim_kolko_krokov_kym_trafim(
    const simulation_context *ctx,
    int sx,
    int sy,
    unsigned long max_steps
);

sim_jedno_policko sim_simuluj_policko(
    const simulation_context *ctx,
    int sx,
    int sy,
    int K,
    unsigned long max_steps
);

void sim_vypocitaj_maticu(
    const simulation_context *ctx,
    policko_data *vystup_matica
);

char *sim_matica_string(
    const server_config *cfg,
    const policko_data *matica,
    int type
);

typedef int (*simulation_should_stop_cb)(void *userdata);

void sim_interactive(
    const simulation_context *ctx,
    void (*writer)(const char *, void *),
    simulation_should_stop_cb should_stop,
    void *userdata
);

#endif 
