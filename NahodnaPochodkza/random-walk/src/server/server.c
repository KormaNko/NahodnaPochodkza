#include "net.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t zastavit = 0;

static void siginit(int sig) {
    (void)sig;
    zastavit = 1;
} 

int main(int argc, char **argv) {
    
    if(argc != 2) {
        fprintf(stderr,"chyba pozui spravne parametre");
        return 2;
    }

    signal(SIGINT,siginit);

    const char * port = argv[1];

    int pripajanie = siet_pocuvaj_tcp(port,8);

    if(pripajanie < 0 ) {
        fprintf(stderr,"chyba");
        return 3;
    }

    printf("Cakam na pripojenie klienta na porte : %s\n",port);
    fflush(stdout);

    while(zastavit == 0) {
           int prijaty = siet_prijmi_klienta(pripajanie); 
            if(prijaty < 0) {
                continue;
            }
    

    char riadok[256];

    int riad = siet_precitaj_riadok(prijaty,riadok,sizeof(riadok));
    if(riad > 0) {
        printf("Prisiel riadok %s \n",riadok);
        if(strcmp(riadok,"HELLO\n") == 0) {
           const char * dorucenaNaServer = "CAU\n";
           (void)siet_posli_vsetko(prijaty,dorucenaNaServer,strlen(dorucenaNaServer));

        }else {
            const char * err= "necznami prikaz\n";
           (void)siet_posli_vsetko(prijaty,err,strlen(err));
        }
        
    } else if (riad == 0) {
    fprintf(stderr, "Klient zavrel spojenie bez spravy\n");
    } else {
    fprintf(stderr, "Chyba pri citani\n");
    }   
    close(prijaty);
    }
    close(pripajanie);
    printf("Serve rukonceny");
    return 0;
}
