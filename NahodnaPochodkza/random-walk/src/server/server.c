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
#include <time.h>

typedef struct {

int fd;
int running;
int mode;
int x,y;
unsigned long kroky;
pthread_mutex_t lock;
pthread_mutex_t send_lock;
const server_config *cfg;

}zdielaneData;

static void ukoncenie(int sig) {
    (void)sig;
} 

static void send_line(zdielaneData *ctx, const char *msg) {
    pthread_mutex_lock(&ctx->send_lock);
    siet_posli_vsetko(ctx->fd, msg, strlen(msg));
    pthread_mutex_unlock(&ctx->send_lock);
}



void *cmd_thread(void *arg) {
    zdielaneData * data = (zdielaneData*)arg;
    char riadok[256];
    const char * spravaPreClienta;

    while(1) {
        int info = siet_precitaj_riadok(data->fd,riadok,sizeof(riadok));
        if(info > 0){
        switch (protocol_parse_line(riadok))
        {
        case PROTO_CMD_HELLO:
            spravaPreClienta = "CAU\n";
            break;
        case PROTO_CMD_GET_STATE:
            if(data->mode == 1){
            spravaPreClienta = "JE NASTAVENY MODE INTERACTIVE\n";
            }else {
            spravaPreClienta = "JE NASTAVENY MODE SUMMARY\n";
            }
            break;
        case PROTO_CMD_MODE_INTERACTIVE:
            spravaPreClienta = "OK MODE INTERACTIVE\n";
            pthread_mutex_lock(&data->lock);
            data->mode = 1;
            pthread_mutex_unlock(&data->lock);
            break;  
        case PROTO_CMD_MODE_SUMMARY:
            spravaPreClienta = "OK MODE SUMMARY\n";
             pthread_mutex_lock(&data->lock);
            data->mode = 0;
             pthread_mutex_unlock(&data->lock);
            break;   
        case PROTO_CMD_QUIT:
            spravaPreClienta = "DOBRE MAJ SA\n";
            pthread_mutex_lock(&data->lock);
            data->running = 0;
            pthread_mutex_unlock(&data->lock);
            return NULL;
              
        default:
            spravaPreClienta = "TAKUTO SPRAVU NEPOZNAM\n";
            break; 
        }
        send_line(data,spravaPreClienta);        
        }else if(info <= 0) {
            data->running = 0;
            return NULL;
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    
    server_config cfg;
    if (config_parse(&cfg, argc, argv) != 0) {
        return 2;
    }
    config_print(&cfg);
    const char * port = cfg.port;

    int pripajanie = siet_pocuvaj_tcp(port,8);

    

    if(pripajanie < 0 ) {
        fprintf(stderr,"chyba");
        return 3;
    }

    struct sigaction sa;    // tu v tomto odseku som si pomahal s copilotom
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = ukoncenie;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if(sigaction(SIGINT,&sa,NULL) < 0) {
        perror("sigaction");
        close(pripajanie);
        return 4;
    }  
    
    sigset_t blocked;                   
    sigset_t empty;                      
    sigemptyset(&blocked);              
    sigaddset(&blocked, SIGINT);         
    if (sigprocmask(SIG_BLOCK, &blocked, NULL) < 0) { 
        perror("sigprocmask");           
        close(pripajanie);               
        return 5;                        
    }                                    
    sigemptyset(&empty);    
        // tu ta cast konci

    printf("Cakam na pripojenie klienta na porte : %s\n",port);
    fflush(stdout);

    while(1) {

        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(pripajanie,&rfds);

        int r = pselect(pripajanie + 1, &rfds, NULL, NULL, NULL, &empty); 
        if (r < 0) {                    
            if (errno == EINTR) {      
                break;                   
            }                            
            perror("pselect");           
            break;                      
        }                                


           int prijaty = siet_prijmi_klienta(pripajanie); 
            if(prijaty < 0) {
                continue;
            }
    
    
        zdielaneData data;
        pthread_mutex_init(&data.send_lock, NULL);
        pthread_mutex_init(&data.lock, NULL);   
        data.fd = prijaty;
        data.running = 1;
        data.mode = 0;
        data.kroky = 0;
        data.x = 0;
        data.y = 0;
        data.cfg = &cfg;
        pthread_t spracovanieSpravSKlientom;
        pthread_create(&spracovanieSpravSKlientom,NULL,cmd_thread,&data);

        pthread_join(spracovanieSpravSKlientom,NULL);
        pthread_mutex_destroy(&data.send_lock);
    pthread_mutex_destroy(&data.lock);
     close(prijaty);  
    }
     
  
   
    
    close(pripajanie);
    printf("server: Server ukonceny");
    return 0;
}
