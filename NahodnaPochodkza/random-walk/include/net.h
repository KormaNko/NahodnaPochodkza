#ifndef NET_H
#define NET_H

#include <stddef.h>

int siet_pocuvaj_tcp(const char *port, int backlog);
int siet_prijmi_klienta(int listen_fd);
int siet_pripoj_sa_tcp(const char *host, const char *port);
int siet_precitaj_riadok(int identifikator, char *buf, size_t buf_sz);
int siet_posli_vsetko(int identifikator, const char *buf, size_t len);

#endif

