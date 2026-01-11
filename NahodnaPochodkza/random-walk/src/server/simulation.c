#define _POSIX_C_SOURCE 200809L

#include "simulation.h"
#include <time.h>

#include <stdlib.h> 
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define MAX_POCET_KROKOV 100000 



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

    for (unsigned long long i = 0; i < pocetPolicok; i++) {
        int x = i % cfg->sirka;
        int y = i / cfg->sirka;

        vystup_matica[i].x = x;
        vystup_matica[i].y = y;

        vystup_matica[i].stats =
            sim_simuluj_policko(cfg, x, y, cfg->K, MAX_POCET_KROKOV);
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


char *sim_matica_string(const server_config *cfg, const policko_data *matica, int type) {
    
    int sirka = cfg->sirka, vyska = cfg->vyska;
    size_t estimate = (sirka * 8 + 3) * vyska + 1;
    char *vystup = malloc(estimate);
    if (!vystup) return NULL;

    vystup[0] = '\0';
    char riadok[256];
    for (int y = 0; y < vyska; ++y) {
        riadok[0] = '\0';
        for (int x = 0; x < sirka; ++x) {
            int idx = y * sirka + x;
            char tmp[32];
            if (type == 0)
                snprintf(tmp, sizeof(tmp), "%5.3lf ", matica[idx].stats.p_hit_within_K);
            else
                snprintf(tmp, sizeof(tmp), "%5.2lf ", matica[idx].stats.avg_steps_to_hit);
            strcat(riadok, tmp);
        }
        strcat(riadok, "\n");
        strcat(vystup, riadok);
    }
    return vystup;
}

void sim_interactive(
    const server_config *cfg,
    void (*writer)(const char *, void *),
    simulation_should_stop_cb should_stop,
    void *userdata
)
{
    const unsigned long tick_ms = 200;
    unsigned R = cfg->R;
    unsigned K = cfg->K;
    for (int start_y = 0; start_y < cfg->vyska; ++start_y) {
        for (int start_x = 0; start_x < cfg->sirka; ++start_x) {
            for (unsigned rep = 0; rep < R; ++rep) {
                if (should_stop && should_stop(userdata))
        return;
                int x = start_x, y = start_y;
                unsigned kroky = 0;
                char info[128];
                snprintf(info, sizeof(info), "REPLIKACIA %u/%u Z POLICKA [%d,%d]\n",
                         rep+1, R, start_x, start_y);
                writer(info, userdata);

                for (unsigned k = 0; k < K; ++k) {
                    if (should_stop && should_stop(userdata))
        return;
                    kroky++;
                    sim_step(cfg, &x, &y);
                    char buf[128];
                    snprintf(buf, sizeof(buf), "UPDATE start_x=%d start_y=%d rep=%u krok=%u x=%d y=%d\n",
                             start_x, start_y, rep+1, kroky, x, y);
                    writer(buf, userdata);

                    if (x == 0 && y == 0) {
                        writer("DOSIAHNUTY STRED\n", userdata);
                        break;
                    }
                   struct timespec ts;
ts.tv_sec = tick_ms / 1000;
ts.tv_nsec = (tick_ms % 1000) * 1000 * 1000;
nanosleep(&ts, NULL);

                }
            }
        }
    }
    writer("INTERACTIVE MOD KONCI (vsetky replikacie hotove)\n", userdata);
}




