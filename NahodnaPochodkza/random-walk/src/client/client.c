#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "net.h"
#include <pthread.h>
#include <stdlib.h>
#include "config.h"



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
    

   
    const char * host = "127.0.0.1";
    const char * port = 12345;
    int pripojenie = siet_pripoj_sa_tcp(host,port);
    if(pripojenie < 0) {
        printf("Nepodarilo sa pripojit\n");
        return 1;
    }
     hlavne_menu(pripojenie);
    pthread_t komunikaciaServer;
    spolocneData sp;
    sp.fd = pripojenie;

    if(pthread_create(&komunikaciaServer,NULL,prijmac_sprav_od_servera,&sp) != 0) {
        close(pripojenie);
        return 5;
    }

  
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

void strip_newline(char *buf) {
    buf[strcspn(buf, "\n")] = 0;
}

void spusti_novu_simulaciu(int socket_fd) {
    if (socket_fd < 0) {
        printf("Neplatne spojenie so serverom.\n");
        return;
    }

    char subor[128];
    int W, H, K, R;
    double pU, pD, pL, pR;
    char line[256];

    printf("Zadaj subor na vystup (napr. data.txt): ");
    fgets(subor, sizeof(subor), stdin);
    strip_newline(subor);

    printf("Zadaj sirku a vysku (W H): ");
    fgets(line, sizeof(line), stdin);
    sscanf(line, "%d %d", &W, &H);

    printf("Zadaj K (max. krokov): ");
    fgets(line, sizeof(line), stdin);
    K = atoi(line);

    printf("Zadaj R (pocet replikacii): ");
    fgets(line, sizeof(line), stdin);
    R = atoi(line);

    printf("Zadaj pravdepodobnosti (pU pD pL pR): ");
    fgets(line, sizeof(line), stdin);
    sscanf(line, "%lf %lf %lf %lf", &pU, &pD, &pL, &pR);

    
    char config_msg[512];
    snprintf(config_msg, sizeof(config_msg),
             "CONFIG %s %d %d %d %d %.6f %.6f %.6f %.6f\n",
             subor, W, H, K, R, pU, pD, pL, pR);

    printf(">> POSIELAM: %s", config_msg);

    if (siet_posli_vsetko(socket_fd, config_msg, strlen(config_msg)) < 0) {
        printf("Chyba pri odosielani CONFIG spravy\n");
    }
}


void hlavne_menu(int socket_fd) {
    while (1) {
        printf("\n===== HLAVNÉ MENU =====\n");
        printf("1) Nová simulácia\n");
        printf("0) Koniec\n");
        printf("Vyber možnosť: ");

        char buf[32];
        if (!fgets(buf, sizeof(buf), stdin)) break;
        int volba = atoi(buf);

        switch (volba) {
            case 1:
                printf("1) Bez prekazok\n");
                printf("2) S prekazkami\n");
                 char line[256];
                int cislo;
                 fgets(line, sizeof(line), stdin);
                    cislo = atoi(line);
                if(cislo == 2) {
                    char subor[128];
                    printf("Zadaj subor s prekazkami: ");
                    fgets(subor, sizeof(subor), stdin);
                    strip_newline(subor);

                    char msg[256];
                    snprintf(msg, sizeof(msg),
                            "MODE OBSTACLES %s\n", subor);

                    siet_posli_vsetko(socket_fd, msg, strlen(msg));
                }


                spusti_novu_simulaciu(socket_fd);
                break;
            case 0:
                printf("Koniec aplikácie.\n");
                return;
            default:
                printf("Neznáma voľba\n");
        }
    }
}