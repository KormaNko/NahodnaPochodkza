

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "net.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("chyba");
        return 2;
    }

    const char * host = argv[1];
    const char * port = argv[2];

    int pripojenie = siet_pripoj_sa_tcp(host,port);
    if(pripojenie < 0) {
        printf("Nepodarilo sa pripojit");
        return 1;
    }

    const char * spravaPreServer = "HELLO\n";

    int odosielam = siet_posli_vsetko(pripojenie,spravaPreServer,strlen(spravaPreServer));
    if (odosielam < 0) {
        close(pripojenie);
        return 1;
    }

    char riadok[256];
    int n = siet_precitaj_riadok(pripojenie,riadok,sizeof(riadok));

    if(n > 0) {
        printf("Toto mi poslal server: %s",riadok);
    }else {
        printf("koniec");
    }
    close(pripojenie);
    return 0;
}
    


