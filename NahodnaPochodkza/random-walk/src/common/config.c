#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int config_parse(server_config *cfg, int argc, char **argv) {
    
    if (argc != 9) {
        fprintf(stderr, "Pouzitie: %s <port> <W> <H> <K> <pU> <pD> <pL> <pR>\n", argv[0]);
        return 1;
    }

    cfg->port = argv[1];
    cfg->sirka = atoi(argv[2]);
    cfg->vyska = atoi(argv[3]);
    cfg->maxKrokov = atoi(argv[4]);

    cfg->pU = strtod(argv[5], NULL);
    cfg->pD = strtod(argv[6], NULL);
    cfg->pL = strtod(argv[7], NULL);
    cfg->pR = strtod(argv[8], NULL);

    if (cfg->sirka <= 0 || cfg->vyska <= 0 || cfg->maxKrokov <= 0) {
        fprintf(stderr, "Chyba: W, H, K musia byt kladne cele cisla.\n");
        return 2;
    }

    if (cfg->pU < 0 || cfg->pD < 0 || cfg->pL < 0 || cfg->pR < 0) {
        fprintf(stderr, "Chyba: pravdepodobnosti musia byt >= 0.\n");
        return 3;
    }

    double sum = cfg->pU + cfg->pD + cfg->pL + cfg->pR;
    if (fabs(sum - 1.0) > 1e-9) {
        fprintf(stderr, "Chyba: pU+pD+pL+pR musi byt 1.0 (teraz je %.17g).\n", sum);
        return 4;
    }

    return 0;
}

void config_print(const server_config *cfg) {
    printf("CONFIG port=%s W=%d H=%d K=%d pU=%.6f pD=%.6f pL=%.6f pR=%.6f\n",
           cfg->port, cfg->sirka, cfg->vyska, cfg->maxKrokov, cfg->pU, cfg->pD, cfg->pL, cfg->pR);
    fflush(stdout);
}
