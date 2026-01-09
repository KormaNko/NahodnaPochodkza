#define _POSIX_C_SOURCE 200809L
#include <time.h>
#include <sys/select.h>

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


static volatile sig_atomic_t server_running = 1;


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
} zdielaneData;


static void ukoncenie(int sig) {
    (void)sig;
    server_running = 0;
}


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
            /* 1️⃣ najprv skontroluj flagy */
            pthread_mutex_lock(&data->lock);
            int run = data->running;
            pthread_mutex_unlock(&data->lock);

            if (!run || !server_running)
                break;

            /* 2️⃣ čakaj na dáta s timeoutom */
            fd_set rfds;
            struct timeval tv;

            FD_ZERO(&rfds);
            FD_SET(data->fd, &rfds);

            tv.tv_sec = 0;
            tv.tv_usec = 200000;  // 200 ms

            int r = select(data->fd + 1, &rfds, NULL, NULL, &tv);
            if (r < 0) {
                /* chyba alebo signál */
                break;
            }
            if (r == 0) {
                /* timeout – skontroluj flagy znovu */
                continue;
            }

            /* 3️⃣ socket je ready → teraz čítaj */
            int info = siet_precitaj_riadok(data->fd, riadok, sizeof(riadok));
            if (info <= 0) {
                pthread_mutex_lock(&data->lock);
                data->running = 0;
                pthread_mutex_unlock(&data->lock);
                break;
            }
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
                        
                       
                        /* simulácia beží */
                sim_interactive(data->cfg, simulation_update_writer, data);
                /* simulácia SKONČILA */

                /* ukonči session */
                pthread_mutex_lock(&data->lock);
                data->running = 0;
                data->mode = 0;
                pthread_mutex_unlock(&data->lock);

                /* AKTÍVNE UKONČI SPOJENIE – zobudí cmd_thread */
                

                /* ukonči server */
                server_running = 0;
        }

        struct timespec ts;
ts.tv_sec = 0;
ts.tv_nsec = 200 * 1000 * 1000; 
nanosleep(&ts, NULL);

    }
    return NULL;
}


int main(int argc, char **argv) {
    srand((unsigned)time(NULL));

    server_config cfg;
    if (config_parse(&cfg, argc, argv) != 0) {
        return 2;
    }

    config_print(&cfg);

    int pocet = cfg.sirka * cfg.vyska;
    policko_data matica[pocet];
    sim_vypocitaj_maticu(&cfg, matica);

    const char *port = cfg.port;
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

    

    printf("Cakam na pripojenie klienta na porte : %s\n", port);
    fflush(stdout);

        while (server_running) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(pripajanie, &rfds);

        int r = pselect(pripajanie + 1, &rfds, NULL, NULL, NULL, NULL);
        if (r < 0) {
            if (errno == EINTR)
                continue;
            perror("pselect");
            break;
        }

        int prijaty = siet_prijmi_klienta(pripajanie);
        if (prijaty < 0) {
            continue;
        }

        printf("Uspesne som pripojil klienta\n");

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
        data.matica = matica;

        pthread_t spracovanieSpravSKlientom;
        pthread_t krokySimulacie;

        pthread_create(&spracovanieSpravSKlientom, NULL, cmd_thread, &data);
            pthread_create(&krokySimulacie, NULL, sim_thread, &data);


            pthread_join(krokySimulacie, NULL);


            close(prijaty);


            pthread_join(spracovanieSpravSKlientom, NULL);


            pthread_mutex_destroy(&data.send_lock);
            pthread_mutex_destroy(&data.lock);

        
    }


    close(pripajanie);
    printf("server: Server ukonceny\n");
    return 0;

}
