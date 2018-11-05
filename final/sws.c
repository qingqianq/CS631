#include <sys/socket.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define DEFAULTPORT 8080
int buildSocket(u_short *port);
void handle_request();
int get_line(int socket, char *buf, int size);
int main(int argc, char *argv[]) {
	int listenedfd;
	int clientfd;
	u_short port = DEFAULTPORT;
	struct sockaddr_in client;
	int client_len = sizeof(client);
	listenedfd = buildSocket(&port);
	while (1) {
		clientfd = accept(listenedfd, (struct sockaddr *)&client, (socklen_t *)&client_len);
		handle_request(clientfd);
	}
}

int buildSocket(u_short *port){  //-p  -i
	int sockfd;
	struct sockaddr_in server_address;	
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		perror("socket error");
		exit(EXIT_FAILURE);
		
	}
	int reuse = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		perror("setsockopet");
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
void handle_request(int clientfd){
	char buf[BUFSIZ];
	int len = 0;
	len = get_line(clientfd, buf, BUFSIZ);
	printf("%s",buf);
	exit(1);
	
	
}

int get_line(int socket, char *buf, int size){
	int i = 0;
	int len;
	char c = '\0';
	while (i < size - 1 && (c !='\n')) {
		len = recv(socket, &c, 1, 0);
		if (len > 0) {
			if(c == '\r'){
				len = recv(socket, &c, 1, MSG_PEEK);
				if((len > 0) && (c == '\n'))
					recv(socket, &c, 1, 0);
				else 
					c = '\n';
			}
			buf[i] = c;
			++i;
		}else {
			c = '\n';
		}
	}
	buf[i] = '\0';
	return len;
}