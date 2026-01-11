#define _POSIX_C_SOURCE 200809L
#include "simulation.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#define MAX_POCET_KROKOV 100000

void sim_step(const simulation_context *ctx, int *x, int *y) { //tu som bral dost velku inspiraciu od AI 
    const server_config *cfg = ctx->cfg;

    int old_x = *x;
    int old_y = *y;

    double r = rand() / (double)RAND_MAX;

    if (r < cfg->pU) (*y)--;
    else if (r < cfg->pU + cfg->pD) (*y)++;
    else if (r < cfg->pU + cfg->pD + cfg->pL) (*x)--;
    else (*x)++;

    *x = (*x % cfg->sirka + cfg->sirka) % cfg->sirka;
    *y = (*y % cfg->vyska + cfg->vyska) % cfg->vyska;

    if (cfg->prekazky && ctx->prekazky) {
        int idx = (*y) * cfg->sirka + (*x);
        if (ctx->prekazky[idx]) {
            *x = old_x;
            *y = old_y;
        }
    }
}

int sim_dojst_do_stredu_za_K(
    const simulation_context *ctx,
    int sx, int sy, int K
) {
    int x = sx, y = sy;

    if (x == 0 && y == 0)
        return 1;

    for (int i = 0; i < K; ++i) {
        sim_step(ctx, &x, &y);
        if (x == 0 && y == 0)
            return 1;
    }
    return 0;
}

unsigned long sim_kolko_krokov_kym_trafim(
    const simulation_context *ctx,
    int sx, int sy,
    unsigned long max_steps
) {
    int x = sx, y = sy;

    if (x == 0 && y == 0)
        return 0;

    for (unsigned long i = 1; i < max_steps; ++i) {
        sim_step(ctx, &x, &y);
        if (x == 0 && y == 0)
            return i;
    }
    return max_steps;
}

sim_jedno_policko sim_simuluj_policko(
    const simulation_context *ctx,
    int sx, int sy,
    int K,
    unsigned long max_steps
) {
    sim_jedno_policko res;
    unsigned long long suma_krokov = 0;
    int trafenych = 0;

    for (int r = 0; r < ctx->cfg->R; ++r) {
        trafenych += sim_dojst_do_stredu_za_K(ctx, sx, sy, K);
        suma_krokov += sim_kolko_krokov_kym_trafim(ctx, sx, sy, max_steps);
    }

    res.p_hit_within_K = (double)trafenych / ctx->cfg->R;
    res.avg_steps_to_hit = (double)suma_krokov / ctx->cfg->R;
    return res;
}

void sim_vypocitaj_maticu( 
    const simulation_context *ctx,
    policko_data *vystup_matica
) {
    const server_config *cfg = ctx->cfg;
    unsigned long long pocet = cfg->sirka * cfg->vyska;

    for (unsigned long long i = 0; i < pocet; ++i) {
        int x = i % cfg->sirka;
        int y = i / cfg->sirka;

        vystup_matica[i].x = x;
        vystup_matica[i].y = y;

        vystup_matica[i].stats =
            sim_simuluj_policko(ctx, x, y, cfg->K, MAX_POCET_KROKOV);
    }
}

char *sim_matica_string( // tu mi to tiez AI trocha debugovalo a upravilo do krajsej formy
    const server_config *cfg,
    const policko_data *matica,
    int type
) {
    size_t odhad = (cfg->sirka * 8 + 2) * cfg->vyska + 1;
    char *out = malloc(odhad);
    if (!out) return NULL;

    out[0] = '\0';

    for (int y = 0; y < cfg->vyska; ++y) {
        for (int x = 0; x < cfg->sirka; ++x) {
            int idx = y * cfg->sirka + x;
            char tmp[32];

            if (type == 0)
                snprintf(tmp, sizeof(tmp), "%5.3f ",
                         matica[idx].stats.p_hit_within_K);
            else
                snprintf(tmp, sizeof(tmp), "%5.2f ",
                         matica[idx].stats.avg_steps_to_hit);

            strcat(out, tmp);
        }
        strcat(out, "\n");
    }
    return out;
}

void sim_interactive(const simulation_context *ctx,void (*writer)(const char *, void *),simulation_should_stop_cb should_stop,void *userdata) {// tato metoda bola dost slusne miero robena s ai hlavne call backy a podobne 
    const server_config *cfg = ctx->cfg;
    const unsigned tick_ms = 200;

    for (int sy = 0; sy < cfg->vyska; ++sy) {
        for (int sx = 0; sx < cfg->sirka; ++sx) {
            for (int r = 0; r < cfg->R; ++r) {

                if (should_stop && should_stop(userdata))
                    return;

                int x = sx, y = sy;
                char info[128];
                snprintf(info, sizeof(info),
                         "REPLIKACIA %d/%d Z POLICKA [%d,%d]\n",
                         r + 1, cfg->R, sx, sy);
                writer(info, userdata);

                for (int k = 0; k < cfg->K; ++k) {
                    if (should_stop && should_stop(userdata))
                        return;

                    sim_step(ctx, &x, &y);

                    char upd[128];
                    snprintf(upd, sizeof(upd),
                             "UPDATE start_x=%d start_y=%d rep=%d krok=%d x=%d y=%d\n",
                             sx, sy, r + 1, k + 1, x, y);
                    writer(upd, userdata);

                    if (x == 0 && y == 0) {
                        writer("DOSIAHNUTY STRED\n", userdata);
                        break;
                    }

                    struct timespec ts;
                    ts.tv_sec = tick_ms / 1000;
                    ts.tv_nsec = (tick_ms % 1000) * 1000000;
                    nanosleep(&ts, NULL);
                }
            }
        }
    }
    writer("INTERACTIVE MOD KONCI\n", userdata);
}
