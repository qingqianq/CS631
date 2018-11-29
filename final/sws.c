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
#include <fcntl.h>
#include <dirent.h>
#include <magic.h>
/*
nc 127.0.0.1 8080 nc ::1 8080
GET /../one/ HTTP/1.0
abc def abd
If-Modified-Since: Sun, 30 Sep 2018 17:58:49 GMT


guangqiqing$ gcc  -Wall -g test.c -lmagic  -o mytest
*/

#define DEFAULTPORT 8080
#define METHODSIZ 10
#define HEAD 10
#define TIMESIZ BUFSIZ
#define SERVER_STRING "Server: gqq version 1.0\r\n"
#define UNIMPLEMENT "HTTP/1.0 500 Not Implement"
#define BAD_REQUEST "HTTP/1.0 400 Bad Request\r\n"
#define CONNECT_SUCCESS "HTTP/1.0 200 OK\r\n"
#define NOT_FOUND "HTTP/1.0 404 Not Found\r\n"
#define CONTENTBUF 20000
#define CONTENT "Content-Type: text/html\r\n"
#define NOT_MODIFIED "HTTP/1.0 304 Not Modified\r\n"


int build_ipv4_socket(u_short *port, const char *ip);
int build_ipv6_socket(u_short *port, const char *ip);
int is_valid_ipv4(const char *ipv4);
int is_valid_ipv6(const char *ipv6);
void handle_request(int clientfd);
void handle_head(int clientfd, const char *url);
void handle_get(int clientfd, const char *path, const char *modify);
int read_line(int socket, char *buf, int size);
int is_cgi(const char *url);
int send_date(int clientfd);
int send_modify(int clientfd, const char *path);
void send_content(int clientfd, const char *path);


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
	printf("sws_dir : %s\n",sws);
	if (sws == NULL) {
		perror("NO DIR\nsws[−dh] [−c dir] [−i address] [−l file] [−p port] dir\n\n");
		exit(EXIT_FAILURE);
	}
	if (l_flag == 1) {
		
	}
	if (i_flag == 1) {
		if (is_valid_ipv4(ip)) {
			listenedfd = build_ipv4_socket(&port, ip);
		}
		if (is_valid_ipv6(ip)) {
			listenedfd = build_ipv6_socket(&port, ip);
		}
	}else {
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
	close(listenedfd);
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
	char *path = NULL;
	char temp[BUFSIZ];
	char modify[BUFSIZ];
	char http_version[BUFSIZ];
	char *query_string = NULL;
	int i = 0, j = 0, cgi = 0;
	int n = read_line(clientfd, buf, BUFSIZ);
	struct stat st;
	while(!isspace(buf[i]) && i < BUFSIZ - 1){
		method[i] = buf[i];
		i++;
	}
	method[i] = '\0';
	printf("method: %s\n",method);
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
	if ((stat(sws, &st)) == -1) {
		perror("stat error\n");
		exit(EXIT_FAILURE);
	}
	if (!S_ISDIR(st.st_mode)) {
		perror("not a dir\n");
		exit(EXIT_FAILURE);
	}
	if (chdir(sws) == -1) {
		perror("change dir error");
		exit(EXIT_FAILURE);
	}
	path = (char *)malloc((size_t)BUFSIZ);
	if(getcwd(path, BUFSIZ + 1) == NULL){
		perror("get path error\n");
		exit(EXIT_FAILURE);
	}
	if (query_string[0] == '~') {
		query_string++;
		strcat(path,query_string);
	}else
		strcat(path,url);
	printf("path: %s\n",path);
	if (is_cgi(query_string)) 
		cgi = 1;
	printf("cgi:%d\n",cgi);
	while (isspace(buf[i]))
		i++;
	j = 0;
	while (!isspace(buf[i]) && j < BUFSIZ - 1) {
		http_version[j] = buf[i];
		j++;
		i++;
	}
	http_version[j] = '\0';
	printf("version:%s\n",http_version);
	if (strcmp(method, "GET") && strcmp(method, "HEAD")) {	
		send(clientfd, UNIMPLEMENT, strlen(UNIMPLEMENT), 0);
		send(clientfd,SERVER_STRING,strlen(SERVER_STRING),0);
//		send(clientfd,CONTENT,strlen(CONTENT),0);
//		send(clientfd, "Content-Length: 0\n", strlen("Content-Length: 0\n"), 0);
		send_date(clientfd);
		close(clientfd);
		return;
	}
//	if ((n = strcmp(http_version, "HTTP/1.1")) == 0){
//		close(clientfd);
//		return;
//	}
	if ((n = strcmp(http_version, "HTTP/0.9")) == 0){
		close(clientfd);
		return;
	}
//	if ((n = strcmp(http_version, "HTTP/1.1")) !=0) { /*test for browser send http 1.1*/
	if ((n = strcmp(http_version, "HTTP/1.0")) !=0) {
		send(clientfd, BAD_REQUEST, strlen(BAD_REQUEST), 0);
		send(clientfd,SERVER_STRING,strlen(SERVER_STRING),0);
		send(clientfd,CONTENT,strlen(CONTENT),0);
		send(clientfd, "Content-Length: 0\r\n", strlen("Content-Length: 0\r\n"), 0);
		send_date(clientfd);
		close(clientfd);
		return;
	}
	
	/* read if-modify-scince */
	while ((n = strcmp(buf, "\n")) != 0 ) {
		i = 0;
		j = 0;
		read_line(clientfd, buf, sizeof(buf));
		while (isspace(buf[i]))
			i++;
		while (!isspace(buf[i]) && j < BUFSIZ - 1) {
			temp[j] = buf[i];
			i++;
			j++;
		}
		temp[j] = '\0';
		printf("%s\n",temp);
		if ((n = strcmp(temp, "If-Modified-Since:")) == 0)
			strcpy(modify,buf);
	}
	if ((n = strcmp(method, "HEAD")) == 0)
		handle_head(clientfd,path);
	if((n = strcmp(method, "GET")) == 0){
//		if(cgi)
//			handle_cgi()
		handle_get(clientfd, path, modify);
		bzero(modify, sizeof(modify));
	}
	close(clientfd);
}

int is_cgi(const char *url){
	int n;
	char buf[BUFSIZ];
	strncpy(buf,url,BUFSIZ);
	if (url[0] == '/') {
		if ((n = strncmp(buf, "/cgi-bin/",sizeof("/cgi-bin/") - 1)) == 0)
			return 1;
		return 0;
	}
	if (url[0] == '~') {
		if ((n = strncmp(buf, "~/cgi-bin/",sizeof("~/cgi-bin/") - 1)) == 0)
			return 1;
		return 0;
	}
	return 0;	
}

void handle_head(int clientfd, const char *path){
	int dp;	
	// here may be safe problem maybe ~/../../   maybe root
	if ((dp = open(path, O_RDONLY)) < 0){
		send(clientfd, NOT_FOUND, strlen(NOT_FOUND), 0);
		send_date(clientfd);
		send(clientfd, SERVER_STRING, strlen(SERVER_STRING), 0);
	}				
	else {		
		send(clientfd, CONNECT_SUCCESS, strlen(CONNECT_SUCCESS), 0);
		send_date(clientfd);
		send(clientfd, SERVER_STRING, strlen(SERVER_STRING), 0);
		send_modify(clientfd, path);
		send(clientfd, CONTENT, strlen(CONTENT), 0);
		send(clientfd, "Content-Length: 0\r\n", strlen("Content-Length: 0\r\n"), 0);
	}	
}
void handle_get(int clientfd, const char *path, const char *modify){
	int dp,n;	
	char buf[TIMESIZ];
	char conntent_buf[CONTENTBUF];
	char *home = NULL;
	char abs_path[BUFSIZ];
	struct stat st;
	struct tm ts;
	struct dirent **namelist;
	/* abs_path should be same with home */ 	
	home = (char *)malloc((size_t)BUFSIZ);
	realpath(path, abs_path);
//	send(clientfd, abs_path, strlen(abs_path), 0);
//	send(clientfd, "\r\n", 2, 0);
	home = getenv("HOME");
	// for safety not root
	if ((n = strncmp(abs_path, home, strlen(home))) != 0 ) {
		send(clientfd, NOT_FOUND, strlen(NOT_FOUND), 0);
		send_date(clientfd);
		send(clientfd, SERVER_STRING, strlen(SERVER_STRING), 0);
		return;
	}
	if ((dp = open(path, O_RDONLY)) < 0){
		send(clientfd, NOT_FOUND, strlen(NOT_FOUND), 0);
		send(clientfd, SERVER_STRING, strlen(SERVER_STRING), 0);
		return;
	}					
	if ((stat(path, &st)) == -1) {
		perror("stat error");
		return;
	}
	ts = *gmtime((time_t*)&st.st_mtimespec);
	strftime(buf, sizeof(buf), "If-Modified-Since: %a, %d %b %Y %H:%M:%S GMT\n", &ts);
	if ((n = strcmp(buf, modify)) == 0) {
		send(clientfd, NOT_MODIFIED, strlen(NOT_MODIFIED), 0);
		send_date(clientfd);
		send(clientfd, SERVER_STRING, strlen(SERVER_STRING), 0);
		send_modify(clientfd, path);
		send(clientfd, "Content-Length: 0\r\n", strlen("Content-Length: 0\r\n"), 0);
		return;
	}else {
		send(clientfd, CONNECT_SUCCESS, strlen(CONNECT_SUCCESS), 0);
		send_date(clientfd);
		send(clientfd, SERVER_STRING, strlen(SERVER_STRING), 0);
		send_modify(clientfd, path);
		send_content(clientfd, path);
	}
	if (S_ISDIR(st.st_mode)) {
		if ((n = scandir(path, &namelist, 0, alphasort)) < 0) {
			perror("sancdir error");
			return;
		}
		/* html test */
//		sprintf(buf, "<HTML><HEAD><TITLE>DIR\r\n");
//		send(clientfd, buf, strlen(buf), 0);
//		sprintf(buf, "</TITLE></HEAD>\r\n");
//		send(clientfd, buf, strlen(buf), 0);
//		sprintf(buf, "<BODY>\r\n");
//		send(clientfd, buf, strlen(buf), 0);
		conntent_buf[0] = '\0';
		for (int i = 0 ;i < n; i++) {
			if(namelist[i]->d_name[0] == '.')
				continue;
//			strcat(conntent_buf,"<P>");
			strcat(conntent_buf,namelist[i]->d_name);
			strcat(conntent_buf,"\r\n");
		}
		sprintf(buf,"Content-Length: %d\r\n",(int)strlen(conntent_buf));
		send(clientfd, buf, strlen(buf), 0);
		send(clientfd,"\r\n",strlen("\r\n"),0);
		send(clientfd, conntent_buf, strlen(conntent_buf), 0);
//		sprintf(buf, "</BODY></HTML>\r\n");
//		send(clientfd, buf, strlen(buf), 0);
	}else { /* file */
		sprintf(buf,"Content-Length: %d\r\n",(int)st.st_size);
		send(clientfd, buf, strlen(buf), 0);
		send(clientfd,"\r\n",strlen("\r\n"),0);
		while ((n = read(dp, buf, BUFSIZ)) > 0)
			send(clientfd,buf,strlen(buf),0);
		send(clientfd,"\r\n",strlen("\r\n"),0);
	}
}
int send_date(int clientfd){
	time_t     now;
	struct tm  ts;
	char       buf[TIMESIZ];
	time(&now);
	ts = *gmtime(&now);
	strftime(buf, sizeof(buf), "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", &ts);
	send(clientfd, buf, strlen(buf), 0);
	return 0;
}
int send_modify(int clientfd, const char *path){
	char buf[TIMESIZ];
	struct stat st;
	struct tm ts;
	if ((stat(path, &st)) == -1) {
		perror("stat error");
		close(clientfd);
		return EXIT_FAILURE;
	}
	ts = *gmtime((time_t*)&st.st_mtimespec);
	strftime(buf, sizeof(buf), "Last-Modified: %a, %d %b %Y %H:%M:%S GMT\r\n", &ts);
	send(clientfd, buf, strlen(buf), 0);
	return EXIT_SUCCESS;
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

int logging(){
	return 0;
}
/* magic 5 and use libmagic 
	compile with -lmagic
*/
void send_content(int clientfd, const char *path){
	char buf[BUFSIZ];
	const char *mime;
	magic_t magic;
	magic = magic_open(MAGIC_MIME_TYPE);
	magic_load(magic, NULL);
	mime = magic_file(magic,path);
	sprintf(buf, "Content-Type: %s\r\n",mime);
	send(clientfd, buf, strlen(buf) , 0);
	magic_close(magic);
}