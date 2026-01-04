

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "net.h"
#include <pthread.h>

typedef struct  {
    int fd;
}spolocneData;


void * prijmac_sprav_od_servera(void * arg) {
    spolocneData * sp = (spolocneData*)arg;
    int cisloSocket = sp->fd;

    char riadok[256];
    while(1) {
        int info = siet_precitaj_riadok(cisloSocket,riadok,sizeof(riadok));
        if(info > 0){
                    printf("Toto mi poslal server: %s",riadok);
        }else if(info == 0) {
            printf("Server ukoncil komunikaciu");
            break;
        }else {
            printf("chyba");
            break;
        }
    }
    return NULL;
}



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
    
    pthread_t komunikaciaServer;
    spolocneData sp;
    sp.fd = pripojenie;

    pthread_create(&komunikaciaServer,NULL,prijmac_sprav_od_servera,&sp);

    const char * spravaPreServer = "HELLO\n";

    int odosielam = siet_posli_vsetko(pripojenie,spravaPreServer,strlen(spravaPreServer));
    if (odosielam < 0) {
        close(pripojenie);
        return 1;
    }

    sleep(5);
spravaPreServer = "QUIT\n";

   odosielam = siet_posli_vsetko(pripojenie,spravaPreServer,strlen(spravaPreServer));
    if (odosielam < 0) {
        close(pripojenie);
        return 1;
    }

    pthread_join(komunikaciaServer,NULL);
    close(pripojenie);
    return 0;
}
    


