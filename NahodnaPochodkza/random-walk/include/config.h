#ifndef CONFIG_H
#include <stddef.h>
#define CONFIG_H

typedef struct {
    const char *suborVystup;   
    int sirka;
    int vyska;
    int K;
    int R;
    double pU, pD, pL, pR;
} server_config;


int config_parse(server_config *cfg, int argc, char **argv);

void NacitajConfig(char *out, size_t out_size);

void config_print(const server_config *cfg);

#endif
