#define _POSIX_C_SOURCE 200809L

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <string.h>

int config_parse(server_config *cfg, int argc, char **argv) {
    
    if (argc != 10) {
        fprintf(stderr, "Pouzitie: %s <suborVystup> <W> <H> <K> <pU> <pD> <pL> <pR>\n", argv[0]);
        return 1;
    }

    cfg->suborVystup = argv[1];
    cfg->sirka = atoi(argv[2]);
    cfg->vyska = atoi(argv[3]);
    cfg->K = atoi(argv[4]);
    cfg->R = atoi(argv[5]);
    cfg->pU = strtod(argv[6], NULL);
    cfg->pD = strtod(argv[7], NULL);
    cfg->pL = strtod(argv[8], NULL);
    cfg->pR = strtod(argv[9], NULL);

    if (cfg->sirka <= 0 || cfg->vyska <= 0 || cfg->K <= 0) {
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
    printf("CONFIG suborVystup=%s W=%d H=%d K=%d pU=%.6f pD=%.6f pL=%.6f pR=%.6f\n",
           cfg->suborVystup, cfg->sirka, cfg->vyska, cfg->K, cfg->pU, cfg->pD, cfg->pL, cfg->pR);
    fflush(stdout);
}


void NacitajConfig(char *out, size_t out_size) {
    char subor[128];
    char W[16], H[16], K[16], R[16];
    char pU[16], pD[16], pL[16], pR[16];

    printf("Zadaj vstupny subor sveta: ");
    fgets(subor, sizeof(subor), stdin);

    printf("Zadaj sirku (W): ");
    fgets(W, sizeof(W), stdin);

    printf("Zadaj vysku (H): ");
    fgets(H, sizeof(H), stdin);

    printf("Zadaj K (max kroky): ");
    fgets(K, sizeof(K), stdin);

    printf("Zadaj pocet replikacii: ");
    fgets(R, sizeof(R), stdin);

    printf("Zadaj pU: ");
    fgets(pU, sizeof(pU), stdin);

    printf("Zadaj pD: ");
    fgets(pD, sizeof(pD), stdin);

    printf("Zadaj pL: ");
    fgets(pL, sizeof(pL), stdin);

    printf("Zadaj pR: ");
    fgets(pR, sizeof(pR), stdin);

    if((pU + pD + pL + pR) != 1) {
        return 1;
        printf("Pravdepodobnosti musia byt spolu 1");
    }
    /* odstránenie '\n' */
    subor[strcspn(subor, "\n")] = 0;
    W[strcspn(W, "\n")] = 0;
    H[strcspn(H, "\n")] = 0;
    K[strcspn(K, "\n")] = 0;
    R[strcspn(R, "\n")] = 0;
    pU[strcspn(pU, "\n")] = 0;
    pD[strcspn(pD, "\n")] = 0;
    pL[strcspn(pL, "\n")] = 0;
    pR[strcspn(pR, "\n")] = 0;

    /* výsledný string – rovnaký formát, aký čaká config_parse */
    snprintf(out, out_size,
             "%s %s %s %s %s %s %s %s %s",
             subor, W, H, K, R, pU, pD, pL, pR);
}
