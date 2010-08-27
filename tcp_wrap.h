#ifndef TCP_WRAP
#define TCP_WRAP
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/un.h> // bzero
#include <netinet/tcp.h>
#include <sys/ioctl.h> // ioctl
#include <net/if.h> // ifreq ifr IFNAMSIZ
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#define MAX_SENDBUF_SIZE (256 * 1024 * 1024)
#define MAX_RECVBUF_SIZE (256 * 1024 * 1024)


int create_tcpsocket(void);

int aton(const char* ipaddress);

char* ntoa(int ip);


void bind_inaddr_any(const int socket,const unsigned short port) __attribute__((always_inline));

void set_nodelay(const int socket) __attribute__((always_inline));

int set_nonblock(const int socket) __attribute__((always_inline));

void set_reuse(const int socket) __attribute__((always_inline));

void set_keepalive(const int socket) __attribute__((always_inline));


void socket_maximize_sndbuf(const int socket) __attribute__((always_inline));
void socket_maximize_rcvbuf(const int socket) __attribute__((always_inline));

int connect_ip_port(const int socket,const int ip,const unsigned short port);
int get_myip(void);
int get_myip_interface(const char* const name);
unsigned int get_myip_interface2(const char* const name);
#endif
