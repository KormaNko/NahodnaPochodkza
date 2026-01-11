#define _POSIX_C_SOURCE 200809L

#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/select.h>
#include "net.h"
#include "config.h"
#include "protokol.h"
#include "simulation.h"

typedef struct {
    int fd;
    int running;
    int mode;
    int stop_requested;
    int matica_ready;
    pthread_mutex_t lock;
    pthread_mutex_t send_lock;
    const server_config *cfg;
    policko_data *matica;
    int *prekazky;
    simulation_context sim_ctx;
    size_t maxRozmer;
} zdielaneData;


static int simulation_should_stop(void *userdata) {
    zdielaneData *data = userdata;
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
    zdielaneData *data = userdata;
    pthread_mutex_lock(&data->lock);
    int active = (data->mode == 1 && data->running);
    pthread_mutex_unlock(&data->lock);

    if (active)
        send_line(data, line);
}

static void generuj_prekazky(int *mapa, int sirka, int vyska) { // celu tuto metodu som skopiroval od cheta podla toho aj vyzera jej kvalita :D
    int pocet = (sirka * vyska) / 10; 

    memset(mapa, 0, sizeof(int) * sirka * vyska);

    int i = 0;
    while (i < pocet) {
        int x = rand() % sirka;
        int y = rand() % vyska;

        if (x == 0 && y == 0)
            continue;

        int idx = y * sirka + x;
        if (!mapa[idx]) {
            mapa[idx] = 1;
            i++;
        }
    }
}

static void *matrix_thread(void *arg) {
    zdielaneData *data = arg;
    pthread_mutex_lock(&data->lock);
    int max = data->maxRozmer;
    pthread_mutex_unlock(&data->lock);
    if(max <= 900){
    sim_vypocitaj_maticu(&data->sim_ctx, data->matica);
    pthread_mutex_lock(&data->lock);
    data->matica_ready = 1;
    pthread_mutex_unlock(&data->lock);
    }
    return NULL;
}

static void *sim_thread(void *arg) {// tu mi to debugovalo ai
    zdielaneData *data = arg;
    while (1) {
        pthread_mutex_lock(&data->lock);
        int run = data->running;
        int mode = data->mode;
        pthread_mutex_unlock(&data->lock);
        if (!run)
            break;
        if (mode == 1) {
            sim_interactive(
                &data->sim_ctx,
                simulation_update_writer,
                simulation_should_stop,
                data
            );
            pthread_mutex_lock(&data->lock);
            data->running = 0;
            data->mode = 0;
            pthread_mutex_unlock(&data->lock);
        }
        struct timespec ts = {0, 200 * 1000 * 1000};
        nanosleep(&ts, NULL);
    }
    return NULL;
}

static void *cmd_thread(void *arg) {
    zdielaneData *data = arg;
    char riadok[256];
    const char *odpoved;

    while (1) {
        pthread_mutex_lock(&data->lock);
        int run = data->running;
        pthread_mutex_unlock(&data->lock);
        if (!run)
            break;
        fd_set rfds;            // tieto riadky tiez robilo AI
        struct timeval tv = {0, 200000};
        FD_ZERO(&rfds);
        FD_SET(data->fd, &rfds);
        int r = select(data->fd + 1, &rfds, NULL, NULL, &tv);
        if (r < 0) {
            if (errno == EINTR) continue;
            break;
        }
        // az po tialto
        int info = siet_precitaj_riadok(data->fd, riadok, sizeof(riadok));
        if (info <= 0)
            break;
        switch (protocol_parse_line(riadok)) {
        case PROTO_CMD_HELLO:
            odpoved = "CAU\n";
            break;
        case PROTO_CMD_GET_STATE:
            pthread_mutex_lock(&data->lock);
            int mode = data->mode;
            pthread_mutex_unlock(&data->lock);
            odpoved = (mode == 1)
                ? "MODE INTERACTIVE\n"
                : "MODE SUMMARY\n";
            break;
        case PROTO_CMD_MODE_INTERACTIVE:
            pthread_mutex_lock(&data->lock);
            data->mode = 1;
            pthread_mutex_unlock(&data->lock);
            odpoved = "OK MODE INTERACTIVE\n";
            break;
        case PROTO_CMD_MODE_SUMMARY:
            pthread_mutex_lock(&data->lock);
            data->mode = 0;
            int ready = data->matica_ready;
            pthread_mutex_unlock(&data->lock);
            if (!ready)
                odpoved = "SUMMARY: MATICA SA ESTE POCITA\n";
            else if (data->maxRozmer > 900)
                odpoved = "SUMMARY: MATICA PRILIS VELKA\n";
            else {
                send_line(data, "OK MODE SUMMARY\n");
                send_line(data, "=== PRAVDEPODOBNOST ===\n");
                char *p = sim_matica_string(data->cfg, data->matica, 0);
                send_line(data, p);
                free(p);
                send_line(data, "=== PRIEMER ===\n");
                char *a = sim_matica_string(data->cfg, data->matica, 1);
                send_line(data, a);
                free(a);
                continue;
            }
            break;
        case PROTO_CMD_STOP:
            pthread_mutex_lock(&data->lock);
            data->stop_requested = 1;
            data->running = 0;
            pthread_mutex_unlock(&data->lock);
            odpoved = "SERVER ZASTAVENY\n";
            break;
        case PROTO_CMD_QUIT:
            pthread_mutex_lock(&data->lock);
            data->running = 0;
            pthread_mutex_unlock(&data->lock);
            odpoved = "ODPOJENY\n";
            break;
        default:
            odpoved = "NEZNAMY PRIKAZ\n";
        }
        send_line(data, odpoved);
    }
    return NULL;
}

static int vytvor_server(const server_config *cfg, int fd) {
    srand(time(NULL));
    config_print(cfg);
    size_t pocet = cfg->sirka * cfg->vyska;
    policko_data *matica = malloc(sizeof(policko_data) * pocet);
    int *prekazky = calloc(pocet, sizeof(int));
    if (!matica || !prekazky) {
    perror("malloc/calloc");
    close(fd);
    free(matica);
    free(prekazky);
    return 1;
    }
    if (cfg->prekazky){
        generuj_prekazky(prekazky, cfg->sirka, cfg->vyska);
        printf("MAPA PREKAZOK:\n");
        for (int y = 0; y < cfg->vyska; y++) {
            for (int x = 0; x < cfg->sirka; x++) {
                int idx = y * cfg->sirka + x;
                printf("%c ", prekazky[idx] ? '#' : '.');
            }
            printf("\n");
        }
        fflush(stdout);
}
    zdielaneData data = {     // to mi tiez refactoroval ai mal som to skoro rovnako iba som to nastavoval postupne v podstate je to jedno
        .fd = fd,
        .running = 1,
        .mode = 0,
        .stop_requested = 0,
        .matica_ready = 0,
        .cfg = cfg,
        .matica = matica,
        .prekazky = prekazky,
        .maxRozmer = pocet,
        .sim_ctx = { cfg, cfg->prekazky ? prekazky : NULL }
    };

    pthread_mutex_init(&data.lock, NULL);
    pthread_mutex_init(&data.send_lock, NULL);
    pthread_t t_cmd, t_sim, t_mat;
    pthread_create(&t_mat, NULL, matrix_thread, &data);
    pthread_create(&t_cmd, NULL, cmd_thread, &data);
    pthread_create(&t_sim, NULL, sim_thread, &data);
    pthread_join(t_sim, NULL);
    pthread_join(t_cmd, NULL);
    pthread_join(t_mat, NULL);
    close(fd);
    free(matica);
    free(prekazky);
    pthread_mutex_destroy(&data.lock);
    pthread_mutex_destroy(&data.send_lock);
    return 0;
}

int main(void) {
    int listen_fd = siet_pocuvaj_tcp("64321", 1);
    int fd = siet_prijmi_klienta(listen_fd);
    close(listen_fd);
    char line[512];
    if (siet_precitaj_riadok(fd, line, sizeof(line)) <= 0)
        return 1;
    if (strncmp(line, "CONFIG ", 7) != 0)
        return 1;
    server_config cfg;
    if (config_parse_string(&cfg, line + 7) != 0)
        return 1;
    config_save_to_file(&cfg);
    vytvor_server(&cfg, fd);
    free((void *)cfg.suborVystup);
    return 0;
}