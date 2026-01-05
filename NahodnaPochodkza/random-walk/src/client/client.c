#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "net.h"
#include <pthread.h>
#include <stdlib.h>

typedef struct  {
    int fd;
}spolocneData;

void * prijmac_sprav_od_servera(void * arg) {
    spolocneData * sp = (spolocneData*)arg;
    int cisloSocket = sp->fd;
    char riadok[512];
    while(1) {
        int info = siet_precitaj_riadok(cisloSocket,riadok,sizeof(riadok));
        if(info > 0){
            printf("%s", riadok);
            fflush(stdout);
        }else if(info == 0) {
            printf("Server ukoncil komunikaciu\n");
            break;
        }else {
            printf("chyba\n");
            break;
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("chyba\n");
        return 2;
    }

    const char * host = argv[1];
    const char * port = argv[2];
    int pripojenie = siet_pripoj_sa_tcp(host,port);
    if(pripojenie < 0) {
        printf("Nepodarilo sa pripojit\n");
        return 1;
    }
    
    pthread_t komunikaciaServer;
    spolocneData sp;
    sp.fd = pripojenie;

    if(pthread_create(&komunikaciaServer,NULL,prijmac_sprav_od_servera,&sp) != 0) {
        close(pripojenie);
        return 5;
    }

    // Handshake
    const char * handshake = "HELLO\n";
    int skusamPripojenie = siet_posli_vsetko(pripojenie,handshake,strlen(handshake));
    if(skusamPripojenie < 0) {
        pthread_join(komunikaciaServer,NULL);
        close(pripojenie);
        return 6;
    }

    char sprava[256];
    while(1) {
        printf("\nMenu: \n"); 
        printf("1) MODE INTERACTIVE\n");
        printf("2) MODE SUMMARY\n");
        printf("3) GET STATE\n");
        printf("0) QUIT\n");
        fflush(stdout);

        if(fgets(sprava, sizeof(sprava), stdin) == NULL) {
            break;
        }
        int cisloVmenu = atoi(sprava);
        const char * spravaMenu = NULL;

        switch (cisloVmenu)
        {
        case 0:
            spravaMenu= "QUIT\n";
            break;
        case 1:
            spravaMenu= "MODE INTERACTIVE\n";
            break;
        case 2:
            spravaMenu= "MODE SUMMARY\n";
            break;
        case 3:
            spravaMenu= "GET STATE\n";
            break;
        default:
            printf("Takyto prikaz nepoznam\n");
            continue;
        }

        int sent = siet_posli_vsetko(pripojenie,spravaMenu,strlen(spravaMenu));
        if (sent < 0) {
            break;
        }

        if(cisloVmenu == 2){
            
            char odpoved[16];
            if(fgets(odpoved, sizeof(odpoved), stdin) == NULL)
                break;
           
            siet_posli_vsetko(pripojenie, odpoved, strlen(odpoved));
        }

        if(strcmp(spravaMenu,"QUIT\n") == 0) {
            break;
        }
    }

    pthread_join(komunikaciaServer,NULL);
    close(pripojenie);
    return 0;
}
