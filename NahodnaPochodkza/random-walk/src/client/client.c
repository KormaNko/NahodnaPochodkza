

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "net.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Pouzitie: %s <host> <port>\nPriklad: %s 127.0.0.1 5555\n",
                argv[0], argv[0]);
        return 2;
    }

    const char *host = argv[1];
    const char *port = argv[2];

    int fd = siet_pripoj_sa_tcp(host, port);
    if (fd < 0) {
        fprintf(stderr, "Nepodarilo sa pripojit na %s:%s\n", host, port);
        return 1;
    }

    const char *msg = "HELLO\n";
    if (siet_posli_vsetko(fd, msg, strlen(msg)) < 0) {
        close(fd);
        return 1;
    }

    char riadok[256];
    int n = siet_precitaj_riadok(fd, riadok, sizeof(riadok));
    if (n > 0) {
        printf("%s", riadok); // ocakavas "OK\n" alebo "ERR ...\n"
    } else if (n == 0) {
        fprintf(stderr, "Server zatvoril spojenie bez odpovede\n");
    } else {
        // siet_precitaj_riadok uz vypise perror("read")
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}
