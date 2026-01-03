#include "net.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t zastav = 0;

static void pri_sigint(int sig) {
    (void)sig;
    zastav = 1;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Pouzitie: %s <port>\nPriklad: %s 5555\n", argv[0], argv[0]);
        return 2;
    }

    signal(SIGINT, pri_sigint);

    const char *port = argv[1];

    int lfd = siet_pocuvaj_tcp(port, 8);
    if (lfd < 0) {
        fprintf(stderr, "Nepodarilo sa spustit server na porte %s\n", port);
        return 1;
    }

    printf("Server pocuva na porte %s\n", port);
    fflush(stdout);

    while (!zastav) {
        int cfd = siet_prijmi_klienta(lfd);
        if (cfd < 0) continue;

        char riadok[256];
        int n = siet_precitaj_riadok(cfd, riadok, sizeof(riadok));
        if (n > 0) {
            printf("Prislo: %s", riadok);

            if (strcmp(riadok, "HELLO\n") == 0) {
                const char *ok = "OK\n";
                (void)siet_posli_vsetko(cfd, ok, strlen(ok));
            } else {
                const char *err = "ERR neznamy prikaz\n";
                (void)siet_posli_vsetko(cfd, err, strlen(err));
            }
        }

        close(cfd);
    }

    close(lfd);
    printf("Server ukonceny\n");
    return 0;
}
