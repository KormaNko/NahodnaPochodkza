#include "net.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include "config.h"


static void ukoncenie(int sig) {
    (void)sig;
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
    
    int mode = 0;
    int x = 0,y = 0;
    unsigned long kroky = 0;
    unsigned long tick_ms = 200;
    unsigned long ms_since_summary = 0;

    while(1){
        
    char riadok[256];
    int riad = siet_precitaj_riadok(prijaty,riadok,sizeof(riadok));
    if(riad > 0) {
       

        if(strcmp(riadok,"HELLO\n") == 0) {
           const char * spravaPreClienta = "server: CAU\n";
           (void)siet_posli_vsetko(prijaty,spravaPreClienta,strlen(spravaPreClienta));

        }else if(strcmp(riadok,"QUIT\n") == 0) {
            const char * spravaPreClienta = "server: BYE\n";
           (void)siet_posli_vsetko(prijaty,spravaPreClienta,strlen(spravaPreClienta));
           break;
        }else if(strcmp(riadok,"MODE INTERACTIVE\n") == 0)
         {
            mode = 1;
            const char * spravaPreClienta = "server: Zapol si si MODE INTERACTIVE\n";
           (void)siet_posli_vsetko(prijaty,spravaPreClienta,strlen(spravaPreClienta));
         }else if(strcmp(riadok,"MODE SUMMARY\n") == 0)
         {
            mode = 0;
            const char * spravaPreClienta = "server: Zapol si si MODE SUMMARY\n";
           (void)siet_posli_vsetko(prijaty,spravaPreClienta,strlen(spravaPreClienta));
         }else if(strcmp(riadok,"GET STATE\n") == 0)
         {
            const char * spravaPreClienta;
            if(mode == 0){
            spravaPreClienta = "server: Mas nastaveny MODE SUMMARY\n";
            }else{ 
            spravaPreClienta = "server: Mas nastaveny MODE INTERACTIVE\n";
            }

           (void)siet_posli_vsetko(prijaty,spravaPreClienta,strlen(spravaPreClienta));
         }
        else {
            const char * err= "server: neznami prikaz\n";
           (void)siet_posli_vsetko(prijaty,err,strlen(err));
        }
        
    } else if (riad == 0) {
    fprintf(stderr, "server: Klient zavrel spojenie bez spravy\n");
    break;
    } else {
    fprintf(stderr, "server: Chyba pri citani\n");
    break;
    }   
    
    }
    close(prijaty);    
    }
   

    close(pripajanie);
    printf("server: Server ukonceny");
    return 0;
}
