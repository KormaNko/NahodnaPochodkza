#include "net.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>



static void ukoncenie(int sig) {
    (void)sig;
} 

int main(int argc, char **argv) {
    
    if(argc != 2) {
        fprintf(stderr,"chyba pozui spravne parametre");
        return 2;
    }

   

    const char * port = argv[1];

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
    

    while(1){
    char riadok[256];
    int riad = siet_precitaj_riadok(prijaty,riadok,sizeof(riadok));
    if(riad > 0) {
        printf("Prisiel riadok %s ",riadok);
        if(strcmp(riadok,"HELLO\n") == 0) {
           const char * dorucenaNaServer = "CAU\n";
           (void)siet_posli_vsetko(prijaty,dorucenaNaServer,strlen(dorucenaNaServer));

        }else if(strcmp(riadok,"QUIT\n") == 0) {
            const char * dorucenaNaServer = "BYE\n";
           (void)siet_posli_vsetko(prijaty,dorucenaNaServer,strlen(dorucenaNaServer));
           break;
        }
        else {
            const char * err= "neznami prikaz\n";
           (void)siet_posli_vsetko(prijaty,err,strlen(err));
        }
        
    } else if (riad == 0) {
    fprintf(stderr, "Klient zavrel spojenie bez spravy\n");
    break;
    } else {
    fprintf(stderr, "Chyba pri citani\n");
    break;
    }   
    
    }
    close(prijaty);    
    }
   

    close(pripajanie);
    printf("Serve rukonceny");
    return 0;
}
