#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <time.h>
/*
nc 127.0.0.1 8080 nc ::1 8080
POST /cgi-bin/post.cgi HTTP/1.0
BODY
*/


#define DEFAULTPORT 8080
#define METHODSIZ 10
//#define PROSIZ 100
#define SERVER_STRING "Server: http1.0\r\n"
#define BAD_REQUEST "HTTP/1.0 400 Bad Request\r\n"
#define CONNECT_SUCCESS "HTTP/1.0 200 OK\r\n"
#define NOT_FOUND "HTTP/1.0 404 Not Found\r\n"
int build_ipv4_socket(u_short *port, const char *ip);
int build_ipv6_socket(u_short *port, const char *ip);
int is_valid_ipv4(const char *ipv4);
int is_valid_ipv6(const char *ipv6);
void handle_request();
void handle_head(int clientfd, const char *url);
int read_line(int socket, char *buf, int size);
int is_cgi(char *url);
//char sws[BUFSIZ] = "/sws";
int c_flag = 0,d_flag = 0,h_flag = 0,i_flag = 0,l_flag = 0;
u_short port = DEFAULTPORT;
int logfd;
char *cgi, *sws, *ip, *log_file = NULL;
int main(int argc, char *argv[]) {
	int listenedfd;
	int clientfd;
	struct sockaddr_in6 client;
	char opt;
	while ((opt = getopt(argc, argv, "c:dhi:l:p:")) != -1) {
		switch(opt){
			case 'c':
				c_flag = 1;
				cgi = optarg;
				break;
			case 'd':
				d_flag = 1;
				break;
			case 'h':
				h_flag = 1;
				break;
			case 'i':
				i_flag = 1;
				ip = optarg;
				break;
			case 'l':
				l_flag = 1;
				log_file = optarg;
				break;
			case 'p':
				port = atoi(optarg);
				break;
		}
	}
	if (h_flag == 1) {
		printf("sws[−dh] [−c dir] [−i address] [−l file] [−p port] dir\n");
		exit(EXIT_SUCCESS);
	}
	if (d_flag == 0) {

	}
	sws = argv[optind];
	printf("%s\n",sws);
	if (sws == NULL) {
		perror("NO DIR\nsws[−dh] [−c dir] [−i address] [−l file] [−p port] dir\n\n");
		exit(EXIT_FAILURE);
	}
	if (l_flag == 1) {
		
	}
	if (i_flag == 1) {
		if (is_valid_ipv4(ip)) {
			printf("ipv4");
			listenedfd = build_ipv4_socket(&port, ip);
		}
		if (is_valid_ipv6(ip)) {
			printf("ipv6");
			listenedfd = build_ipv6_socket(&port, ip);
		}
	}else {
		printf("else");
		listenedfd = build_ipv6_socket(&port, ip);
	}
	int client_len = sizeof(client);
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
	int reuse = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
		perror("setsockopet");
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
	char method[BUFSIZ];
	char url[BUFSIZ];
	char *path;
	char http_version[BUFSIZ];
	char *query_string = NULL;
	int i = 0, j = 0, cgi = 0;
	int n = read_line(clientfd, buf, BUFSIZ);
	while(!isspace(buf[i]) && i < METHODSIZ - 1){
		method[i] = buf[i];
		i++;
	}
	method[i] = '\0';
	printf("method : %s\n",method);
//	i++;
	while (isspace(buf[i])) {
		i++;
	}
	while (!isspace(buf[i]) && j < BUFSIZ - 1) {
		url[j] = buf[i];
		j++;
		i++;
	}
	url[j] = '\0';
	printf("url :%s\n",url);
	query_string = url;
	/* handle_url */
	if (query_string[0] == '~') {
		path = getenv("HOME");
		strcat(path,sws);
		query_string++;
		strcat(path,query_string);
	}else 
		path = url;	
	printf("path:%s\n",path);
	
	if (is_cgi(query_string)) 
		cgi = 1;
	while (isspace(buf[i])) {
		i++;
	}
	j = 0;
	while (!isspace(buf[i]) && j < BUFSIZ - 1) {
		http_version[j] = buf[i];
		j++;
		i++;
	}
	http_version[j] = '\0';
	printf("version:%s\n",http_version);
	if (strcmp(method, "GET") && strcmp(method, "HEAD")) {
		send(clientfd, BAD_REQUEST, sizeof(BAD_REQUEST), 0);
		close(clientfd);
		return;
	}
	if ((n = strcmp(http_version, "HTTP/1.1")) == 0){
		close(clientfd);
		return;
	}
	if ((n = strcmp(http_version, "HTTP/0.9")) == 0){
		close(clientfd);
		return;
	}
	if ((n = strcmp(http_version, "HTTP/1.0")) !=0) {
		send(clientfd, BAD_REQUEST, sizeof(BAD_REQUEST), 0);
		close(clientfd);
		return;
	}
	if ((n = strcmp(method, "HEAD")) == 0) {
		handle_head(clientfd,path);
	}
}

int is_cgi(char *url){
	int n;
	char buf[11];
	strncpy(buf,url,10);
	if (url[0] == '/') {
		buf[9] = '\0';
		if ((n = strcmp(buf, "/cgi-bin/")) == 0)
			return 1;
		return 0;
	}
	if (url[0] == '~') {
		buf[10] = '\0';
		if ((n = strcmp(buf, "~/cgi-bin/")) == 0)
			return 1;
		return 0;
	}
	return 0;	
}

void handle_head(int clientfd, const char *path){
	time_t now;
	struct tm *tm_now;
	time(&now);
	tm_now = localtime(&now);
	send(clientfd, SERVER_STRING, sizeof(SERVER_STRING), 0);
	if (fopen(path, "r")) {
		
	}
	send(clientfd, CONNECT_SUCCESS, sizeof(CONNECT_SUCCESS), 0);
//	 printf("now datetime: %d-%d-%d %d:%d:%d\n",
//	tm_now->tm_year+1900, tm_now->tm_mon+1, tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec) ;
}
int read_line(int socket, char *buf, int size){
	int i = 0;
	int len;
	char c = '\0';
	while (i < size - 1 && (c !='\n')) {
		len = recv(socket, &c, 1, 0);
		if (len > 0) {
			if(c == '\r'){
				len = recv(socket, &c, 1, MSG_PEEK); // MSG_PEEK test next \n without change the next recv
				if((len > 0) && (c == '\n'))
					recv(socket, &c, 1, 0);
				else 
					c = '\n';
			}
			buf[i] = c;
			++i;
		}else 
			c = '\n';
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

int is_modify(const char *modify){
	return 0;
}

int logging(){
	return 0;
}