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

        fd_set rfds; // zas a znovu tieto ridaky su cisto od AI - >
        FD_ZERO(&rfds);
        pthread_mutex_lock(&sp->lock);
        int fd = sp->fd;
        pthread_mutex_unlock(&sp->lock);
        FD_SET(fd, &rfds);
       int r = select(sp->fd + 1, &rfds, NULL, NULL, NULL);
    if (r <= 0)
    continue;
            // po tialto povedalo ze je to bezna prax robit to takto 
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
    if (pid == 0) { // toto som si tiez pozerl cez AI lebo som si uz nepametal ako nato
        execl("./bin/server", "./bin/server", NULL);
        perror("exec");
        exit(1);
    }
    const char * host = "127.0.0.1";
    const char * port = "64321";
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
    server_config cfg;
    config_parse_string(&cfg, config_line);
    config_save_to_file(&cfg);
    free((void *)cfg.suborVystup);
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
        printf("4) STOP\n");
        printf("0) QUIT\n");
        fflush(stdout);

        if(fgets(sprava, sizeof(sprava), stdin) == NULL) {
            break;
        }
        char *end;
        long volba = strtol(sprava, &end, 10); // toto robilo AI tu ochranu pred zlym nacitanim
        if (end == sprava || *end != '\n') {
            printf("Zly vstup\n");
            continue;
        }

        int cisloVmenu = (int)volba;
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
        case 4:
            spravaMenu= "STOP\n";
            break;
        default:
            printf("Takyto prikaz nepoznam\n");
            continue;
        }
        int sent = siet_posli_vsetko(pripojenie,spravaMenu,strlen(spravaMenu));
        if (sent < 0) {
            break;
        }
        if(strcmp(spravaMenu,"QUIT\n") == 0) {
            break;
        }
    }
  pthread_mutex_lock(&sp.lock);
sp.running = 0;
pthread_mutex_unlock(&sp.lock);
shutdown(sp.fd, SHUT_RDWR); // toto shutdown mi tiez AI poradilo povedalo ze je to 100% legitimne tak dufam :)
pthread_join(komunikaciaServer, NULL);
close(sp.fd);
pthread_mutex_destroy(&sp.lock);
    return 0;
}
