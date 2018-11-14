#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

/*
nc 127.0.0.1 8080 nc ::1 8080
POST /cgi-bin/post.cgi HTTP/1.0
BODY
*/


#define DEFAULTPORT 8080
int build_ipv4_socket(u_short *port, const char *ip);
int build_ipv6_socket(u_short *port, const char *ip);
int is_valid_ipv4(const char *ipv4);
int is_valid_ipv6(const char *ipv6);
void handle_request();
int read_line(int socket, char *buf, int size);
int main(int argc, char *argv[]) {
	int listenedfd;
	int clientfd;
	u_short port = DEFAULTPORT;
//	struct sockaddr_in client;
	struct sockaddr_in6 client;
	int client_len = sizeof(client);
	/* listenedfd = build_ipv4_socket(&port,NULL); */
//  listenedfd = build_ipv4_socket(&port, "127.0.0.1");
//  listenedfd = build_ipv6_socket(&port, "::1");
  listenedfd = build_ipv6_socket(&port,NULL);
//	if (is_valid_ipv6("::1"))
//		printf("yes");
	while (1) {
		if((clientfd = accept(listenedfd, (struct sockaddr *)&client, (socklen_t *)&client_len)) == -1){
			perror("accept error");
			exit(EXIT_FAILURE);
		}
		handle_request(clientfd);
	}
}



int build_ipv4_socket(u_short *port, const char *ip){  //-p  -i
	int sockfd;
	struct sockaddr_in server_address;	
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		perror("ipv4 socket error");
		exit(EXIT_FAILURE);
		
	}
	int reuse = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
		perror("setsockopet");
	}
	memset(&server_address,0,sizeof(server_address)); 
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(*port);
	if (ip)
		server_address.sin_addr.s_addr = inet_addr(ip);
	else 
		server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if(bind(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1){
		perror("ipv4 binding error");
		exit(EXIT_FAILURE);
	}
	if(listen(sockfd,5) == -1){
		perror("ipv4 listen error");
		exit(EXIT_FAILURE);
	}
	return sockfd;
}
int build_ipv6_socket(u_short *port, const char *ip){
	int sockfd;
	struct sockaddr_in6 server_address;
	if ((sockfd = socket(AF_INET6, SOCK_STREAM, 0)) == -1) {
		perror("ipv6 socket error");
		exit(EXIT_FAILURE);
	}

	bzero(&server_address,sizeof(server_address));
	server_address.sin6_family = AF_INET6;
	server_address.sin6_port = htons(*port);
	if(ip)
		inet_pton(AF_INET6, ip, &server_address.sin6_addr);
	else 
		server_address.sin6_addr = in6addr_any;
	if((bind(sockfd, (struct sockaddr *)&server_address, sizeof(struct sockaddr_in6))) == -1 ){
		perror("ipv6 binding error");
		exit(EXIT_FAILURE);
	}
	if(listen(sockfd, 5) == -1){
		perror("ipv4 listen error");
		exit(EXIT_FAILURE);
	}

	return sockfd;
}


void handle_request(int clientfd){
	char buf[BUFSIZ];
	int len = 0;
	len = read_line(clientfd, buf, BUFSIZ);
	printf("request: %s",buf);
	len = read_line(clientfd, buf, BUFSIZ);
		printf("request: %s",buf);
	
}

int read_line(int socket, char *buf, int size){
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
int is_valid_ipv4(const char *ipv4){
	struct in_addr addr;
	if(ipv4 == NULL)
		return 0;
	if(inet_pton(AF_INET, ipv4, (void *)&addr) == 1)
		return 1;
	return 0;
}

int is_valid_ipv6(const char *ipv6){
	struct in6_addr addr6;
	if(ipv6 == NULL)
		return 0;
	if(inet_pton(AF_INET6, ipv6, (void *)&addr6) == 1)
		return 1;
	return 0;
}
