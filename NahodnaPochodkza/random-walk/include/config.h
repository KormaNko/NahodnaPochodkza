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
    int prekazky;
} server_config;

int config_parse(server_config *cfg, int argc, char **argv);
int config_parse_string(server_config *cfg, const char *str);
void NacitajConfig(char *out, size_t out_size);
int config_save_to_file(const server_config *cfg);
void config_print(const server_config *cfg);

#endif
