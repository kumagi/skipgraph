#include "tcp_wrap.h"
static int OK = 1;

int create_tcpsocket(void){
	int fd = socket(AF_INET,SOCK_STREAM, 0);
	if(fd < 0){
		perror("scocket");
	}
	return fd;
}

int aton(const char* ipaddress){
	struct in_addr tmp_inaddr;
	int ip = 0;
	if(inet_aton(ipaddress,&tmp_inaddr)){
		ip = tmp_inaddr.s_addr;
	}else {
		printf("aton:address invalid\n");
	}
	return ip;
}

char* ntoa(int ip){
	struct sockaddr_in tmpAddr;
	tmpAddr.sin_addr.s_addr = ip;
	return inet_ntoa(tmpAddr.sin_addr);
}


void bind_inaddr_any(const int socket,const unsigned short port){
	struct sockaddr_in myaddr;
	bzero((char*)&myaddr,sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port=htons(port);
	if(bind(socket,(struct sockaddr*)&myaddr, sizeof(myaddr)) < 0){
		perror("bind");
		close(socket);
		exit (0);
	}
}

void set_nodelay(const int socket){

	int result = setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (void *)&OK, sizeof(OK));
	if(result < 0){
		perror("set nodelay ");
	}
}

int set_nonblock(const int socket) {
	return fcntl(socket, F_SETFL, O_NONBLOCK);
}

void set_reuse(const int socket){
	int result = setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (void *)&OK, sizeof(OK));
	if(result < 0){
		perror("set reuse ");
	}
}
void set_keepalive(const int socket){
	int result = setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, (void *)&OK, sizeof(OK));
	if(result < 0){
		perror("set keepalive ");
	}
}


void socket_maximize_sndbuf(const int socket){
    socklen_t intsize = sizeof(int);
    int last_good = 0;
    int min, max, avg;
    int old_size;

	//*  minimize!
	avg = intsize;
	setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (void *)&avg, intsize);
	return;
	//*
	
    if (getsockopt(socket, SOL_SOCKET, SO_SNDBUF, &old_size, &intsize) != 0) {
        return;
    }

    min = old_size;
    max = MAX_SENDBUF_SIZE;

    while (min <= max) {
        avg = ((unsigned int)(min + max)) / 2;
        if (setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (void *)&avg, intsize) == 0) {
            last_good = avg;
            min = avg + 1;
        } else {
            max = avg - 1;
        }
    }
}
void socket_maximize_rcvbuf(const int socket){
    socklen_t intsize = sizeof(int);
    int last_good = 0;
    int min, max, avg;
    int old_size;

    if (getsockopt(socket, SOL_SOCKET, SO_RCVBUF, &old_size, &intsize) != 0) {
        return;
    }

    min = old_size;
    max = MAX_RECVBUF_SIZE;

    while (min <= max) {
        avg = ((unsigned int)(min + max)) / 2;
        if (setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (void *)&avg, intsize) == 0) {
            last_good = avg;
            min = avg + 1;
        } else {
            max = avg - 1;
        }
    }
}

int connect_ip_port(const int socket,const int ip,const unsigned short port){
	//return 0 if succeed
	
	struct sockaddr_in addr;
	bzero((char*)&addr,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ip;
	addr.sin_port=htons(port);
	
	if(connect(socket,(struct sockaddr*)&addr,sizeof(addr))){
		perror("connect");
		fprintf(stderr,"target:[%s:%d]\n",ntoa(ip),port);
		//exit(1);
		assert(!"connect failed");
		return 1;
	}
    set_nonblock(socket);
	return 0;
}
int get_myip(void){
	struct ifreq ifr;
	int fd=socket(AF_INET, SOCK_DGRAM,0);
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name,"eth0",IFNAMSIZ-1);
	ioctl(fd,SIOCGIFADDR,&ifr);
	close(fd);
	return (int)((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr.s_addr;
}

int get_myip_interface(const char* const name){
	struct ifreq ifr;
	int fd=socket(AF_INET, SOCK_STREAM,0);
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name,name,IFNAMSIZ-1);
	if(ioctl(fd,SIOCGIFADDR,&ifr) < 0){
		perror("ioctl(SIOCGIFADDR)");
		exit(1);
	}
	close(fd);
	return (int)((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr.s_addr;
}


unsigned int get_myip_interface2(const char* const name){
	struct ifreq ifr;
	struct sockaddr_in sin;
	int fd=socket(AF_INET, SOCK_STREAM,0);
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_addr.sa_family = AF_INET;
	
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, name);
	if(ioctl(fd, SIOCGIFADDR, &ifr) < 0){
		char buff[250];
		sprintf(buff,"cannot use <%s> SIOCGIFADDR",ifr.ifr_name);
		perror(buff);exit(1);
	}
	if(ioctl(fd, SIOCGIFADDR, &ifr) < 0){
		perror("ioctl(SIOCGIFADDR)");exit(1);
	}
	if(ifr.ifr_addr.sa_family == AF_INET){
		memcpy(&sin, &(ifr.ifr_addr), sizeof(struct sockaddr_in));
	}else{
		exit(1);
	}
	close(fd);
	return sin.sin_addr.s_addr;
}
