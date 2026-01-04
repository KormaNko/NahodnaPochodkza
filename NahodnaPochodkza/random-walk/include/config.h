#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    const char *port;   
    int sirka;
    int vyska;
    int maxKrokov;
    double pU, pD, pL, pR;
} server_config;


int config_parse(server_config *cfg, int argc, char **argv);


void config_print(const server_config *cfg);

#endif
