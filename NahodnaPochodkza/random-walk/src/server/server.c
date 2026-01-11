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



static int vytvor_server(const server_config *cfg, int fd);

static volatile sig_atomic_t server_running = 1;


typedef struct {
    int fd;
    int running;
    int mode;
    int x, y;
    int stop_requested;
    int matica_ready;
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
static int simulation_should_stop(void *userdata) {
    zdielaneData *data = (zdielaneData *)userdata;

    pthread_mutex_lock(&data->lock);
    int stop = data->stop_requested;
    pthread_mutex_unlock(&data->lock);

    return stop;
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

void *matrix_thread(void *arg) {
    zdielaneData *data = arg;

    sim_vypocitaj_maticu(data->cfg, data->matica);

    pthread_mutex_lock(&data->lock);
    data->matica_ready = 1;
    pthread_mutex_unlock(&data->lock);

    return NULL;
}

void *cmd_thread(void *arg) {
    zdielaneData *data = (zdielaneData*)arg;
    char riadok[256];
    const char *spravaPreClienta;

            while (1) {
            
            pthread_mutex_lock(&data->lock);
            int run = data->running;
            pthread_mutex_unlock(&data->lock);
            //|| !server_running
            if (!run )
                break;

           
            fd_set rfds;
            struct timeval tv;

            FD_ZERO(&rfds);
            FD_SET(data->fd, &rfds);

            tv.tv_sec = 0;
            tv.tv_usec = 200000; 

            int r = select(data->fd + 1, &rfds, NULL, NULL, &tv);
            if (r < 0) {
            if (errno == EINTR)
                continue;
            perror("select");
            break;
        }
            if (r == 0) {
                
                continue;
            }

           
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
                pthread_mutex_lock(&data->lock);
                int ready = data->matica_ready;
                data->mode = 0;
                pthread_mutex_unlock(&data->lock);

                if (!ready) {
                    spravaPreClienta = "SUMMARY: MATICA SA ESTE POCITA\n";
                    break;
                }

                spravaPreClienta = "OK MODE SUMMARY\n";
                send_line(data, spravaPreClienta);

                
                send_line(data, "=== PRAVDEPODOBNOST ===\n");
                char *p = sim_matica_string(data->cfg, data->matica, 0);
                send_line(data, p);
                free(p);

               
                send_line(data, "=== PRIEMERNY POCET KROKOV ===\n");
                char *a = sim_matica_string(data->cfg, data->matica, 1);
                send_line(data, a);
                free(a);

                break;
            case PROTO_CMD_QUIT:
                pthread_mutex_lock(&data->lock);
                data->running = 0;
                pthread_mutex_unlock(&data->lock);
                spravaPreClienta = "DOBRE MAJ SA\n";
                break;
            case PROTO_CMD_STOP:
                pthread_mutex_lock(&data->lock);
                data->stop_requested = 1;
                data->running = 0;
                pthread_mutex_unlock(&data->lock);

                spravaPreClienta = "SERVER ZASTAVENY\n";
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
                sim_interactive(
    data->cfg,
    simulation_update_writer,
    simulation_should_stop,
    data
);
                pthread_mutex_lock(&data->lock);
                data->running = 0;
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



int main(void) {
    int listen_fd = siet_pocuvaj_tcp("54321", 1);
    int fd = siet_prijmi_klienta(listen_fd);

    char line[512];
    if (siet_precitaj_riadok(fd, line, sizeof(line)) <= 0)
        return 1;

    if (strncmp(line, "CONFIG ", 7) != 0)
        return 1;

    server_config cfg;
    if (config_parse_string(&cfg, line + 7) != 0)
        return 1;

    vytvor_server(&cfg, fd);
    free((void *)cfg.suborVystup);
    //close(listen_fd);
    return 0;
}





 static int vytvor_server(const server_config *cfg, int prijaty) {
    srand((unsigned)time(NULL));

    config_print(cfg);

    int pocet = cfg->sirka * cfg->vyska;

        policko_data *matica = malloc(sizeof(policko_data) * pocet);
        if (!matica) {
            perror("malloc matica");
            exit(1);
        }

    
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = ukoncenie;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    zdielaneData data;
    pthread_mutex_init(&data.send_lock, NULL);
    pthread_mutex_init(&data.lock, NULL);

    data.fd = prijaty;
    data.running = 1;
    data.mode = 0;
    data.kroky = 0;
    data.x = 0;
    data.y = 0;
    data.cfg = cfg;
    data.matica = matica;
    data.stop_requested = 0;
    data.matica_ready = 0;
    pthread_t matrix_tid;

    pthread_t spracovanieSpravSKlientom;
    pthread_t krokySimulacie;
    pthread_create(&matrix_tid, NULL, matrix_thread, &data);
    
    pthread_create(&spracovanieSpravSKlientom, NULL, cmd_thread, &data);
    pthread_create(&krokySimulacie, NULL, sim_thread, &data);

    pthread_join(krokySimulacie, NULL);

    close(prijaty);

    pthread_join(spracovanieSpravSKlientom, NULL);

    pthread_mutex_destroy(&data.send_lock);
    pthread_mutex_destroy(&data.lock);

    printf("server: Server ukonceny\n");
    pthread_join(matrix_tid, NULL);
    free(matica);
    return 0;
}

