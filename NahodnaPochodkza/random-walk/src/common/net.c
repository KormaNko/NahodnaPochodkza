#include "net.h"
#include <sys/socket.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

static int znovuPouzitie(int identifikator) {
    int pravda = 1;
    return setsockopt(identifikator,SOL_SOCKET,SO_REUSEADDR,&pravda,sizeof(pravda));
}

int siet_posli_vsetko(int identifikator,const char * buf,size_t dlzka) {
    size_t kdeSom = 0;
    while(kdeSom < dlzka) {
        ssize_t kolkoSaMiPodariloPoslat = write(identifikator,buf + kdeSom,dlzka - kdeSom);

        if(kolkoSaMiPodariloPoslat < 0 ) {
            if(errno == EINTR) {
                continue;
            }
            perror("write");
            return -1;
        }
        kdeSom += (size_t)kolkoSaMiPodariloPoslat;
    }
    return 0;
}


int siet_precitaj_riadok(int identifikator, char *buf, size_t buf_sz) {
    if(buf_sz == 0){
        return -1;
    }

    size_t kdeSom = 0;
    while(kdeSom + 1 < buf_sz) {
        char ch;
        ssize_t navratova = read(identifikator,&ch,1);
        if(navratova == 0) {
            buf[kdeSom] = '\0';
            return kdeSom;
        }else if(navratova < 0) {
                if(errno == EINTR) {
                    continue;
                }
                perror("read");
                return -1;
        }else if(navratova  == 1) {
            buf[kdeSom] = ch;
            kdeSom++;
            if(ch == '\n') {
                break;
            }
        }
        
    }
    buf[kdeSom] = '\0';
    return (int)kdeSom;
}

int siet_pripoj_sa_tcp(const char *host, const char *port) {
    struct addrinfo pomoc;
    memset(&pomoc,0,sizeof(pomoc));
    pomoc.ai_socktype = SOCK_STREAM;
    pomoc.ai_family = AF_UNSPEC;

    struct addrinfo * ukazovatelNaZaciatokZoznamuAdries;
    int info = getaddrinfo(host,port,&pomoc,&ukazovatelNaZaciatokZoznamuAdries);
    if(info != 0) return -1;

    struct addrinfo *kurzorAdries;
    int fd;
    kurzorAdries = ukazovatelNaZaciatokZoznamuAdries;
    while(kurzorAdries != NULL) {
    fd = socket(kurzorAdries->ai_family,kurzorAdries->ai_socktype,kurzorAdries->ai_protocol);
    if(fd < 0 ){
        kurzorAdries = kurzorAdries->ai_next;
        continue;
    }

    int neuspesne = connect(fd,kurzorAdries->ai_addr,kurzorAdries->ai_addrlen);
    if(neuspesne == 0) {
        freeaddrinfo(ukazovatelNaZaciatokZoznamuAdries);
        return fd;
    }else {
        close(fd);
    }
    kurzorAdries = kurzorAdries->ai_next;
    }
    freeaddrinfo(ukazovatelNaZaciatokZoznamuAdries);
    return -1;
}

int siet_pocuvaj_tcp(const char *host, const char *port, int backlog) {
    
}