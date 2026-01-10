#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "net.h"
#include <pthread.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "config.h"
#include <errno.h> 
#include <sys/select.h>
typedef struct  {
    int fd;
    int running;
    pthread_mutex_t lock;
}spolocneData;

void * prijmac_sprav_od_servera(void * arg) {
    spolocneData * sp = (spolocneData*)arg;
    char riadok[512];
    while(1) {
        pthread_mutex_lock(&sp->lock);
        int run = sp->running;
        pthread_mutex_unlock(&sp->lock);

        if (!run)
            break;

        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(sp->fd, &rfds);

       int r = select(sp->fd + 1, &rfds, NULL, NULL, NULL);
    if (r <= 0)
    continue;

        int info = siet_precitaj_riadok(sp->fd,riadok,sizeof(riadok));
        if(info > 0){
            printf("%s", riadok);
            fflush(stdout);
        }else if(info == 0) {
            printf("Server ukoncil komunikaciu\n");
            break;
        }else {
            if (errno == EBADF || errno == EINTR) {
                break;   
            }
            perror("read");
            break;
        }
    }
     
    return NULL;
}






int main(void) {
   

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {

        execl("./bin/server", "./bin/server", NULL);
        perror("exec");
        exit(1);
    }
    const char * host = "127.0.0.1";
    const char * port = "77777";

    sleep(1); 

    
    int pripojenie = -1;

for (int i = 0; i < 50; i++) {         
    pripojenie = siet_pripoj_sa_tcp(host, port);
    if (pripojenie >= 0)
        break;
    sleep(1);                   
}

if (pripojenie < 0) {
    fprintf(stderr, "Nepodarilo sa pripojit k serveru\n");
    return 1;
}
    

    char config_line[256];
    NacitajConfig(config_line, sizeof(config_line));


    char msg[300];
    snprintf(msg, sizeof(msg), "CONFIG %s\n", config_line);
    siet_posli_vsetko(pripojenie, msg, strlen(msg));

    pthread_t komunikaciaServer;
    spolocneData sp;
    
    sp.fd = pripojenie;
    sp.running = 1;
    pthread_mutex_init(&sp.lock, NULL);
    
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
    
  pthread_mutex_lock(&sp.lock);
sp.running = 0;
pthread_mutex_unlock(&sp.lock);

shutdown(sp.fd, SHUT_RDWR);
pthread_join(komunikaciaServer, NULL);
close(sp.fd);
pthread_mutex_destroy(&sp.lock);
    return 0;
}
