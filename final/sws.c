#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define DEFAULTPORT 8080
int buildSocket(u_short *port);
int main(int argc, char *argv[]) {
	int listenfd;
	u_short port = DEFAULTPORT;
	listenfd = buildSocket(&port);
	printf("%d",listenfd);
}

int buildSocket(u_short *port){  //-p  -i
	int sockfd;
	struct sockaddr_in server_address;	
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		perror("socket error");
		exit(EXIT_FAILURE);
	}
	memset(&server_address,0,sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(*port);
	server_address.sin_addr.s_addr= htonl(INADDR_ANY);
	if(bind(sockfd, (struct sockaddr *)&server_address, sizeof(server_address))<0){
		perror("binding error");
		exit(EXIT_FAILURE);
	}
	if(listen(sockfd,5)<0){
		perror("listen error");
		exit(EXIT_FAILURE);
	}
	return sockfd;
}