#define _POSIX_C_SOURCE 200809L
#include <time.h>
#define PORT 12345
#include "net.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include "config.h"
#include "protokol.h"
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include "simulation.h"

typedef struct {
    int fd;
    int running;
    int mode;
    int x, y;
    unsigned long kroky;
    pthread_mutex_t lock;
    pthread_mutex_t send_lock;
    const server_config *cfg;
    policko_data *matica;

    nacitavanie prekazky;
} zdielaneData;


static void ukoncenie(int sig) { (void)sig; }

static void send_line(zdielaneData *ctx, const char *msg) {
    pthread_mutex_lock(&ctx->send_lock);
    siet_posli_vsetko(ctx->fd, msg, strlen(msg));
    pthread_mutex_unlock(&ctx->send_lock);
}

static void simulation_update_writer(const char *line, void *userdata) {
    zdielaneData *data = (zdielaneData*)userdata;

    
    pthread_mutex_lock(&data->lock);
    int really_interactive = (data->mode == 1 && data->running);
    pthread_mutex_unlock(&data->lock);

    if (really_interactive) {
        send_line(data, line);
    }
}


void *cmd_thread(void *arg) {
    zdielaneData *data = (zdielaneData*)arg;
    char riadok[256];
    const char *spravaPreClienta;

    while (1) {
        pthread_mutex_lock(&data->lock);
        int run = data->running;
        pthread_mutex_unlock(&data->lock);
        if (!run) break;

        int info = siet_precitaj_riadok(data->fd, riadok, sizeof(riadok));
        if (info > 0) {
            switch (protocol_parse_line(riadok)) {
            case PROTO_CMD_HELLO:
                spravaPreClienta = "CAU\n";
                break;
            case PROTO_CMD_GET_STATE: {
                int cislo;
                pthread_mutex_lock(&data->lock);
                cislo = data->mode;
                pthread_mutex_unlock(&data->lock);
                spravaPreClienta = (cislo == 1)
                    ? "JE NASTAVENY MODE INTERACTIVE\n"
                    : "JE NASTAVENY MODE SUMMARY\n";
                break;
            }
            case PROTO_CMD_MODE_INTERACTIVE:
                spravaPreClienta = "OK MODE INTERACTIVE\n";
                pthread_mutex_lock(&data->lock);
                data->mode = 1;
                pthread_mutex_unlock(&data->lock);
                break;
            case PROTO_CMD_MODE_SUMMARY:
                spravaPreClienta = "OK MODE SUMMARY\n";
                pthread_mutex_lock(&data->lock);
                data->mode = 0;
                pthread_mutex_unlock(&data->lock);
                send_line(data, "Chces zobrazit (0) PRAVDEPODOBNOST alebo (1) PRIEMERNY POCET KROKOV?\n");
                char odpoved[16];
                int pocet = siet_precitaj_riadok(data->fd, odpoved, sizeof(odpoved));
                if (pocet > 0) {
                    int volba = atoi(odpoved);
                    char *vystup = sim_matica_string(data->cfg, data->matica, volba);
                    send_line(data, vystup);
                    free(vystup);
                } else {

                    send_line(data, "Chyba pri citani volby summary.\n");
                }
                break;
                        case PROTO_CMD_MODE_OBSTACLES: {
                            uvolni_prekazky(&data->prekazky);
                char subor[128];

                if (sscanf(riadok, "MODE OBSTACLES %127s", subor) != 1) {
                    send_line(data, "ERROR ZLY FORMAT\n");
                    break;
                }

                if (nacitaj_prekazky(subor, &data->prekazky) != 0) {
                    send_line(data, "ERROR PREKAZKY NENACITANE\n");
                    break;
                }
                /* uvolni staru maticu */
                /* ===== PREPOCET SVETA S PREKAZKAMI ===== */
pthread_mutex_lock(&data->lock);

/* uvolni staru maticu */
                    if (data->matica) {
                        free(data->matica);
                        data->matica = NULL;
                    }

                    /* alokuj novu */
                    int pocet = data->cfg->sirka * data->cfg->vyska;
                    data->matica = malloc(sizeof(policko_data) * pocet);
                    if (!data->matica) {
                        pthread_mutex_unlock(&data->lock);
                        send_line(data, "ERROR ALOKACIA MATICE\n");
                        break;
                    }

                    pthread_mutex_unlock(&data->lock);

                    /* vypocitaj maticu MIMO mutexu (dlha operacia) */
                    sim_vypocitaj_maticu(data->cfg, &data->prekazky, data->matica);

                    send_line(data, "OK MATICA PREPOCITANA\n");

                if (!data->matica) {
                    send_line(data, "ERROR ALOKACIA MATICE\n");
                    break;
                }

                /* PREPOCITAJ MATICU – TU JE KLUC */
                sim_vypocitaj_maticu(
                    data->cfg,
                    &data->prekazky,   // 👈 TERAZ JU MAS
                    data->matica
                );

                send_line(data, "OK MATICA PREPOCITANA\n");

                data->prekazky.mod_prekazok = 1;
                send_line(data, "OK MOD S PREKAZKAMI\n");
                break;
            }
            case PROTO_CMD_QUIT:
                pthread_mutex_lock(&data->lock);
                data->running = 0;
                pthread_mutex_unlock(&data->lock);
                spravaPreClienta = "DOBRE MAJ SA\n";
                break;
            default:
                spravaPreClienta = "TAKUTO SPRAVU NEPOZNAM\n";
                break;
            }
            send_line(data, spravaPreClienta);
        } else if (info <= 0) {
            pthread_mutex_lock(&data->lock);
            data->running = 0;
            pthread_mutex_unlock(&data->lock);
            return NULL;
        }
    }
    return NULL;
}

static void *sim_thread(void *arg) {
    zdielaneData *data = (zdielaneData*)arg;

    while (1) {
        pthread_mutex_lock(&data->lock);
        int run = data->running;
        int mode = data->mode;
        pthread_mutex_unlock(&data->lock);

        if (!run) break;

        if (mode == 1) {
            
            sim_interactive(data->cfg, simulation_update_writer, data,&data->prekazky);

            
            pthread_mutex_lock(&data->lock);
            data->mode = 0;
            pthread_mutex_unlock(&data->lock);
        }

        struct timespec ts;
ts.tv_sec = 0;
ts.tv_nsec = 200 * 1000 * 1000; 
nanosleep(&ts, NULL);

    }
    return NULL;
}
void uvolni_prekazky(nacitavanie *n)
{
    if (!n || !n->prekazky)
        return;

    for (int y = 0; y < n->vyska; y++)
        free(n->prekazky[y]);

    free(n->prekazky);
    n->prekazky = NULL;
    n->mod_prekazok = 0;
}


int main(int argc, char **argv) {
    srand((unsigned)time(NULL));






    server_config cfg;

    if (config_parse(&cfg, argc, argv) != 0) {
        return 2;
    }

    config_print(&cfg);

    

    const char *port = PORT;
    int pripajanie = siet_pocuvaj_tcp(port, 8);

    if (pripajanie < 0) {
        fprintf(stderr, "chyba");
        return 3;
    }


    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = ukoncenie;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction");
        close(pripajanie);
        return 4;
    }

    sigset_t blocked;
    sigset_t empty;
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGINT);
    if (sigprocmask(SIG_BLOCK, &blocked, NULL) < 0) {
        perror("sigprocmask");
        close(pripajanie);
        return 5;
    }
    sigemptyset(&empty);

    printf("Cakam na pripojenie klienta na porte : %s\n", port);
    fflush(stdout);

    while (1) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(pripajanie, &rfds);

        int r = pselect(pripajanie + 1, &rfds, NULL, NULL, NULL, &empty);
        if (r < 0) {
            if (errno == EINTR) {
                break;
            }
            perror("pselect");
            break;
        }

        int prijaty = siet_prijmi_klienta(pripajanie);
        if (prijaty < 0) {
            continue;
        }

        zdielaneData data;
        pthread_mutex_init(&data.send_lock, NULL);
        pthread_mutex_init(&data.lock, NULL);
        data.fd = prijaty;
        data.running = 1;
        data.mode = 0;
        data.kroky = 0;
        data.x = 0;
        data.y = 0;
        data.cfg = &cfg;
        data.matica = NULL;



        pthread_mutex_lock(&data.lock);

        int pocet = cfg.sirka * cfg.vyska;
        data.matica = malloc(sizeof(policko_data) * pocet);
        if (!data.matica) {
            pthread_mutex_unlock(&data.lock);
            close(prijaty);
            continue;
        }

        pthread_mutex_unlock(&data.lock);


        sim_vypocitaj_maticu(&cfg, NULL, data.matica);


        pthread_t spracovanieSpravSKlientom;
        pthread_t krokySimulacie;
        pthread_create(&spracovanieSpravSKlientom, NULL, cmd_thread, &data);
        pthread_create(&krokySimulacie, NULL, sim_thread, &data);

        pthread_join(spracovanieSpravSKlientom, NULL);

     
        pthread_mutex_lock(&data.lock);
        data.running = 0;
        pthread_mutex_unlock(&data.lock);
        pthread_join(krokySimulacie, NULL);

        pthread_mutex_destroy(&data.send_lock);
        pthread_mutex_destroy(&data.lock);
        uvolni_prekazky(&data.prekazky);
        close(prijaty);
    }

    close(pripajanie);
    printf("server: Server ukonceny\n");
    return 0;
}


int nacitaj_prekazky(const char *subor, nacitavanie *n) {
    FILE *f = fopen(subor, "r");
    if (!f) return -1;

    fscanf(f, "%d %d", &n->sirka, &n->vyska);

    n->prekazky = malloc(n->vyska * sizeof(int *));
    for (int y = 0; y < n->vyska; y++) {
        n->prekazky[y] = malloc(n->sirka * sizeof(int));
        for (int x = 0; x < n->sirka; x++) {
            fscanf(f, "%d", &n->prekazky[y][x]);
        }
    }

    fclose(f);
    n->mod_prekazok = 1;
    return 0;
}

