#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

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
#include <pthread.h>

#define LISTENSIZE 5
#define DEFAULTPORT 8080
#define METHODSIZ 10
#define HEAD 10
#define TIMESIZ BUFSIZ
#define SERVER_STRING "Server: gqq version 1.0\r\n"
#define SERVER_ERROR "HTTP/1.0 500 Internal Server Error\r\n"
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
void *handle_request(void* client);
void handle_head(int clientfd, const char *path);
void handle_get(int clientfd, const char *path, const char *modify);
int read_line(int socket, char *buf, int size);
int is_cgi(const char *url);
int send_date(int clientfd);
int send_modify(int clientfd, const char *path);
void send_content(int clientfd, const char *path);
void handle_cgi(int clientfd, const char *path);
void send_cgi_error(int clientfd);
char *replace (const char *url, const char *old_user, const char *new_user);
void log_ip(int clientfd);
void *accept_ipv6(void *);

int c_flag = 0,d_flag = 0,h_flag = 0,i_flag = 0,l_flag = 0;
u_short port = DEFAULTPORT;
int log_fd = 0;
const char *cgi_path, *sws, *ip, *log_file = NULL;
int main(int argc, char *argv[]) {
	int listenedfd,listenedfd_ipv6;
	int clientfd;
	struct sockaddr_in client;
	char opt;
	while ((opt = getopt(argc, argv, "c:dhi:l:p:")) != -1) {
		switch(opt){
			case 'c':
				c_flag = 1;
				cgi_path = optarg;
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
		/* mac dont support this function*/
		if (daemon(1, 0) == -1) {
			perror("daemon err\n");
		}
	}else {
		printf("Debuging mode\n");
	}
	sws = argv[optind];
	printf("sws_dir : %s\n",sws);
	if (sws == NULL) {
		perror("NO DIR\nsws[−dh] [−c dir] [−i address] [−l file] [−p port] dir\n\n");
		exit(EXIT_FAILURE);
	}
	if (l_flag == 1 && log_file != NULL) {
		if ((log_fd = open(log_file,O_CREAT|O_APPEND|O_RDWR,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) <= 0 ){
			perror("create log_file error");
			exit(EXIT_FAILURE);
		}
	}
	if (i_flag == 1) {
		if (is_valid_ipv4(ip)) {
			listenedfd = build_ipv4_socket(&port, ip);
		}
		if (is_valid_ipv6(ip)) {
			listenedfd = build_ipv6_socket(&port, ip);
		}
	}else {
		listenedfd = build_ipv4_socket(&port, ip);
		listenedfd_ipv6 = build_ipv6_socket(&port, ip);
	}		
	int client_len = sizeof(client);	
	/* use a new thread to listen ipv6 */
	pthread_t ipv6_thread, client_thread;
	if (pthread_create(&ipv6_thread, NULL, accept_ipv6, (void*)&listenedfd_ipv6) != 0)
		perror("create ipv6 error");	
	while (1) {
		if((clientfd = accept(listenedfd, (struct sockaddr *)&client, (socklen_t *)&client_len)) == -1){
			perror("accept error");
			exit(EXIT_FAILURE);
		}
		if (log_fd > 0 && l_flag == 1)
			log_ip(clientfd);
		if(d_flag)
			handle_request(&clientfd);
		else { /* create a thread to handle 2 request */
			if(pthread_create(&client_thread, NULL, handle_request, &clientfd) != 0)
				perror("create thread error");
		}
	}
	close(listenedfd);
	return  EXIT_SUCCESS;
}

void *accept_ipv6(void* listened){
	int listenedfd = *(int *)listened;
	struct sockaddr_in6 client;
	int client_len = sizeof(client);
	int clientfd;
	pthread_t client_thread;
	while (1) {
		if((clientfd = accept(listenedfd, (struct sockaddr *)&client, (socklen_t *)&client_len)) == -1){
			perror("accept error");
			exit(EXIT_FAILURE);
		}
		if (log_fd > 0 && l_flag == 1)
			log_ip(clientfd);
		if(d_flag)
			handle_request(&clientfd);
		else {
			if(pthread_create(&client_thread, NULL, handle_request, &clientfd) != 0)
				perror("create thread error");
		}
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
	if(listen(sockfd,LISTENSIZE) == -1){
		perror("ipv4 listen error");
		exit(EXIT_FAILURE);
	}
	return sockfd;
}
void log_ip(int clientfd){
	socklen_t len;
	struct sockaddr_storage addr;
	u_short port;
	char buf[BUFSIZ];
	
	if (getpeername(clientfd, (struct sockaddr*)&addr, &len) == -1) {
		printf("log error");
		return;
	}
	if (addr.ss_family == AF_INET) { //ipv4
		char ip_address[INET_ADDRSTRLEN];
		struct sockaddr_in *s = (struct sockaddr_in *)&addr;
		port = ntohs(s->sin_port);
		inet_ntop(AF_INET, &s->sin_addr,ip_address, (socklen_t)INET_ADDRSTRLEN);
		sprintf(buf,"%s:%d ",ip_address,port);
		write(log_fd, buf, strlen(buf));
	}else {
		char ip_address[INET6_ADDRSTRLEN];
		struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
		port = ntohs(s->sin6_port);
		inet_ntop(AF_INET6, &s->sin6_addr, ip_address, INET6_ADDRSTRLEN);
		sprintf(buf,"%s:%d ",ip_address,port);
		write(log_fd, buf, strlen(buf));
	}
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
	if(listen(sockfd, LISTENSIZE) == -1){
		perror("ipv4 listen error");
		exit(EXIT_FAILURE);
	}
	return sockfd;
}

void *handle_request(void* client){
	int clientfd = *(int *)client;
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
	if (log_fd > 0 && l_flag == 1){
		write(log_fd, buf, strlen(buf) - 1);
		write(log_fd, " ", strlen(" "));
	}
	struct stat st;
	while(!isspace(buf[i]) && i < BUFSIZ - 1){
		method[i] = buf[i];
		i++;
	}
	method[i] = '\0';
	printf("method: %s\n",method);
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
	
	/* url begin with /~ use replace to get new user */
	if (query_string[1] == '~') {
		char path_buf[BUFSIZ];
		char new_user[BUFSIZ];
		char *p = NULL;
		char *q = NULL;
		/*delete ~ */	
		query_string++;
		query_string++;
		memset(path_buf,0,sizeof(path_buf));
		strcpy(new_user,query_string);
		q = strstr(query_string, "/");
		p = strtok(new_user, "/");
		p = getenv("USER");
		if (p)
			path = replace(path, p, new_user);
		strcat(path,q);
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
	/* not GET and HEAD method */
	if (strcmp(method, "GET") && strcmp(method, "HEAD")) {	
		send(clientfd, BAD_REQUEST, strlen(BAD_REQUEST), 0);
		if (log_fd > 0 && l_flag == 1)
			write(log_fd, BAD_REQUEST, strlen(BAD_REQUEST) - 2);
		send_date(clientfd);
		send(clientfd,SERVER_STRING,strlen(SERVER_STRING),0);
		send(clientfd,CONTENT,strlen(CONTENT),0);
		send(clientfd, "Content-Length: 0\n", strlen("Content-Length: 0\n"), 0);
		if (log_fd > 0 && l_flag == 1)
			write(log_fd, " Content-Length: 0\n", strlen(" Content-Length: 0\n"));
		close(clientfd);
		return NULL;
	}
	if ((n = strcmp(http_version, "HTTP/1.1")) == 0){
		close(clientfd);
		return NULL;
	}
	if ((n = strcmp(http_version, "HTTP/0.9")) == 0){
		close(clientfd);
		return NULL;
	}
//	if ((n = strcmp(http_version, "HTTP/1.1")) !=0) { /*test for browser send http 1.1*/
	if ((n = strcmp(http_version, "HTTP/1.0")) !=0) {
		send(clientfd, BAD_REQUEST, strlen(BAD_REQUEST), 0);
		if (log_fd > 0 && l_flag == 1)
			write(log_fd, BAD_REQUEST, strlen(BAD_REQUEST) - 2);
		send(clientfd,SERVER_STRING,strlen(SERVER_STRING),0);
		send(clientfd,CONTENT,strlen(CONTENT),0);
		send(clientfd, "Content-Length: 0\r\n", strlen("Content-Length: 0\r\n"), 0);
		if (log_fd > 0 && l_flag == 1)
			write(log_fd, " Content-Length: 0\n", strlen(" Content-Length: 0\n"));
		send_date(clientfd);
		close(clientfd);
		return NULL;
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
		if ((n = strcmp(temp, "If-Modified-Since:")) == 0){
			strcpy(modify,buf);
			printf("%s\n",modify);
		}			
	}
	if ((n = strcmp(method, "HEAD")) == 0)
		handle_head(clientfd,path);
	if((n = strcmp(method, "GET")) == 0){
		if(c_flag != 0 && cgi_path != NULL && cgi != 0){
			handle_cgi(clientfd,path);
			close(clientfd);
			return NULL;
		}			
		handle_get(clientfd, path, modify);
		bzero(modify, sizeof(modify)); 
	}
	close(clientfd);
	return NULL;
}
/* 
		set path by ?
		set env by &
*/
void handle_cgi(int clientfd, const char *path){
	char buf[BUFSIZ];
	strcpy(buf,path);
	char abs_path[BUFSIZ];
	char query_string[BUFSIZ];
	char env[BUFSIZ];
	char *p = NULL;
	char *q = NULL;
	struct stat st;
	if ((q = strchr(buf, '?')) != NULL) {
		p = strtok(buf, "?");
		realpath(cgi_path, abs_path);		/* cgi-path */
		p = strtok(NULL, "?");
		if (p)
			strcpy(query_string,p);		
		if ((q = strchr(query_string, '&')) != NULL) {
			p = strtok(query_string, "&");
			strcpy(env,p);
			if(strchr(env,'='))
				putenv(env);
			while ((p = strtok(NULL, "&")) != NULL) {
				strcpy(env,p);
				if(strchr(env, '='))
					putenv(env);
			}
		}else {
			if(strchr(query_string, '='))
				putenv(query_string);
		}
	}else 
		realpath(cgi_path, abs_path);
	/* chage /cgi-bin/ with cgi_path*/
	p = strstr(buf, "cgi-bin");
	for (int i = 0;i < (int)strlen("cgi-bin"); i++,p++){}
	strcat(abs_path,p);
	/*execute cgi*/
	if (stat(abs_path, &st) == 0 && st.st_mode & S_IXUSR){	/* executable */
		send(clientfd, CONNECT_SUCCESS, strlen(CONNECT_SUCCESS), 0);
		if (log_fd > 0 && l_flag == 1)
			write(log_fd, CONNECT_SUCCESS, strlen(CONNECT_SUCCESS) - 2);
		int cgi_output[2];
		int cgi_input[2];
		pid_t pid;
		if (pipe(cgi_output) < 0){
			send_cgi_error(clientfd);
			return;
		}			
		if (pipe(cgi_input) < 0) {
			send_cgi_error(clientfd);
			return;
		}			
		if ((pid = fork()) < 0) {
			send_cgi_error(clientfd);
			return;
		}
		if(pid == 0){ //child
			close(cgi_output[0]);
			close(cgi_input[1]);
			dup2(cgi_output[1],STDOUT_FILENO);
			dup2(cgi_input[0], STDIN_FILENO);
			execl(abs_path, abs_path, NULL);
			exit(EXIT_SUCCESS);
		}else { //parent
			int size = 0, n = 0;
			char content_buf[CONTENTBUF];
			content_buf[0] = '\0';
			close(cgi_output[1]);
			close(cgi_input[0]);
			send_date(clientfd);
			send(clientfd,SERVER_STRING,strlen(SERVER_STRING),0);
			send_modify(clientfd, abs_path);
			send_content(clientfd, abs_path);
			memset(content_buf,0,sizeof(content_buf));
			memset(buf,0,sizeof(buf));
			while ((n = read(cgi_output[0], buf, sizeof(buf))) > 0){
				size += n;
				strcat(content_buf,buf);
			}			
			sprintf(buf,"Content-Length: %d\r\n",size);
			send(clientfd, buf, strlen(buf), 0);
			if (log_fd > 0 && l_flag == 1)
				write(log_fd, buf, strlen(buf));
			send(clientfd,"\r\n",2,0);
			send(clientfd,content_buf,size,0);
			close(cgi_output[0]);
			close(cgi_input[1]);
			waitpid(pid, NULL, 0);
		}
	}else{
		send(clientfd, NOT_FOUND, strlen(NOT_FOUND), 0);
		if (log_fd > 0 && l_flag == 1)
			write(log_fd, NOT_FOUND, strlen(NOT_FOUND) - 2);
		send_date(clientfd);
		send(clientfd, SERVER_STRING, strlen(SERVER_STRING), 0);
	}
}

void send_cgi_error(int clientfd){
	send(clientfd, SERVER_ERROR, strlen(SERVER_ERROR), 0);
	if (log_fd > 0 && l_flag == 1)
		write(log_fd, SERVER_ERROR, strlen(SERVER_ERROR) - 2);
	send_date(clientfd);
	send(clientfd,SERVER_STRING,strlen(SERVER_STRING),0);
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
	return 0;
}

void handle_head(int clientfd, const char *path){
	int dp;	
	if ((dp = open(path, O_RDONLY)) < 0){
		send(clientfd, NOT_FOUND, strlen(NOT_FOUND), 0);
		if (log_fd > 0 && l_flag == 1)
			write(log_fd, NOT_FOUND, strlen(NOT_FOUND) - 2);
		send_date(clientfd);
		send(clientfd, SERVER_STRING, strlen(SERVER_STRING), 0);
	}				
	else {		
		send(clientfd, CONNECT_SUCCESS, strlen(CONNECT_SUCCESS), 0);
		if (log_fd > 0 && l_flag == 1)
			write(log_fd, CONNECT_SUCCESS, strlen(CONNECT_SUCCESS) - 2);
		send_date(clientfd);
		send(clientfd, SERVER_STRING, strlen(SERVER_STRING), 0);
		send_modify(clientfd, path);
		send_content(clientfd, path);
		send(clientfd, "Content-Length: 0\r\n", strlen("Content-Length: 0\r\n"), 0);
		if (log_fd > 0 && l_flag == 1)
			write(log_fd, " Content-Length: 0\n", strlen(" Content-Length: 0\n"));
	}	
}
void handle_get(int clientfd, const char *path, const char *modify){
	int dp, n, c;	
	char buf[TIMESIZ];
	char content_buf[CONTENTBUF];
	char *home = NULL;
	char abs_path[BUFSIZ];
	struct stat st;
	struct stat sp;
	struct tm ts;
	struct dirent **namelist;
	/* abs_path should not out of with home */ 	
	home = (char *)malloc((size_t)BUFSIZ);
	realpath(path, abs_path);
	home = "/home";	
	/* can not get out of /home or /Users */
	if ((n = strncmp(abs_path, home, strlen(home))) != 0 ) {
		send(clientfd, NOT_FOUND, strlen(NOT_FOUND), 0);
		if (log_fd > 0 && l_flag == 1)
			write(log_fd, NOT_FOUND, strlen(NOT_FOUND) - 2);
		send_date(clientfd);
		send(clientfd, SERVER_STRING, strlen(SERVER_STRING), 0);
		return; 
	}
	if ((dp = open(path, O_RDONLY)) < 0){
		send(clientfd, NOT_FOUND, strlen(NOT_FOUND), 0);
		if (log_fd > 0 && l_flag == 1)
			write(log_fd, NOT_FOUND, strlen(NOT_FOUND) - 2);
		send_date(clientfd);
		send(clientfd, SERVER_STRING, strlen(SERVER_STRING), 0);
		close(dp);
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
		if (log_fd > 0 && l_flag == 1)
			write(log_fd, NOT_MODIFIED, strlen(NOT_MODIFIED) - 2);
		send_date(clientfd);
		send(clientfd, SERVER_STRING, strlen(SERVER_STRING), 0);
		send_modify(clientfd, path);
		send(clientfd, "Content-Length: 0\r\n", strlen("Content-Length: 0\r\n"), 0);
		if (log_fd > 0 && l_flag == 1)
			write(log_fd, " Content-Length: 0\n", strlen(" Content-Length: 0\n"));
		return;
	}else {
		send(clientfd, CONNECT_SUCCESS, strlen(CONNECT_SUCCESS), 0);
		if (log_fd > 0 && l_flag == 1)
			write(log_fd, CONNECT_SUCCESS, strlen(CONNECT_SUCCESS) - 2);
		send_date(clientfd);
		send(clientfd, SERVER_STRING, strlen(SERVER_STRING), 0);
		send_modify(clientfd, path);
		send_content(clientfd, path);
	}
	if (S_ISDIR(st.st_mode)) { /* dir use alphasort to sort*/
		/* check index.html exist or not */
		char temp[BUFSIZ];
		strcpy(temp,path);
		c = (int)strlen(temp) - 1;
		temp[c] == '/' ? strcat(temp, "index.html") : strcat(temp, "/index.html");
		if (stat(temp, &sp) == 0) {
			sprintf(buf,"Content-Length: %d\r\n",(int)sp.st_size);
			send(clientfd, buf, strlen(buf), 0);
			if (log_fd > 0 && l_flag == 1)
				write(log_fd,buf, strlen(buf));
			send(clientfd,"\r\n",strlen("\r\n"),0);
			close(dp);
			dp = open(temp, O_RDONLY);
			memset(buf,0,sizeof(buf));
			while ((n = read(dp, buf, BUFSIZ)) > 0)
				send(clientfd, buf,strlen(buf),0);
			send(clientfd,"\r\n",strlen("\r\n"),0);
			close(dp);
			return;
		}
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
		content_buf[0] = '\0';
		for (int i = 0 ;i < n; i++) {
			if(namelist[i]->d_name[0] == '.')
				continue;
//			strcat(content_buf,"<P>"); 
			strcat(content_buf,namelist[i]->d_name);
			strcat(content_buf,"\r\n");
		}
		sprintf(buf,"Content-Length: %d\r\n",(int)strlen(content_buf));
		send(clientfd, buf, strlen(buf), 0);
		if (log_fd > 0 && l_flag == 1)
			write(log_fd, buf, strlen(buf));
		send(clientfd,"\r\n",strlen("\r\n"),0);
		send(clientfd, content_buf, strlen(content_buf), 0);
//		sprintf(buf, "</BODY></HTML>\r\n");
//		send(clientfd, buf, strlen(buf), 0);
	}else { /* file */
		sprintf(buf,"Content-Length: %d\r\n",(int)st.st_size);
		send(clientfd, buf, strlen(buf), 0);
		if (log_fd > 0 && l_flag == 1)
			write(log_fd, buf, strlen(buf));
		send(clientfd,"\r\n",strlen("\r\n"),0);
		memset(buf,0,sizeof(buf));
		while ((n = read(dp, buf, BUFSIZ)) > 0)
			send(clientfd, buf,strlen(buf),0);
		send(clientfd,"\r\n",strlen("\r\n"),0);
		close(dp);
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
	if (log_fd > 0 && l_flag == 1){
		strftime(buf, sizeof(buf), " Date: %a, %d %b %Y %H:%M:%S GMT ", &ts);
		write(log_fd, buf, strlen(buf));
	}		
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
/* 
	magic 5 and use libmagic package which mac donot have
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
/* for /~user/... to replace all old user to new user */
char *replace (const char *url, const char *old_user, const char *new_user) { 
	char *result; 
	int i, cnt = 0; 
	int new_len = strlen(new_user); 
	int old_len = strlen(old_user); 
	for (i = 0; url[i] != '\0'; i++) { 
		if (strstr(&url[i], old_user) == &url[i]) { 
			cnt++; 
			i += old_len - 1; 
		} 
	}
	result = (char *)malloc(i + cnt * (new_len - old_len) + 1);
	i = 0; 
	while (*url) { 
		if (strstr(url, old_user) == url) { 
			strcpy(&result[i], new_user); 
			i += new_len; 
			url += old_len; 
		} 
		else
			result[i++] = *url++; 
	}   
	result[i] = '\0'; 
	return result; 
}