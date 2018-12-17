#include <sys/wait.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#define MAXTOKEN 100
#define DFTPERMISSION 0644
#define REDIRECTION_APPEND 1

typedef struct mycmd{
    char *cmd_token[MAXTOKEN];
    int tk_len;
}mycmd;

int last_cmd = 0;

int cd_cmd(char *tokens[],int x_flag);
int echo_cmd(int tokens_num, char *tokens[], int x_flag);
void exit_cmd(char *tokens[], int x_flag);
char *trim_string(const char *line);
char *replace (const char *src, const char *old, const char *new);
char *add_space(const char *line);
void pipe_exe(int tokens_num, char **tokens,int x_flag);
int parse_tokens(int tokens_num, char *tokens[], int flag);
void red_exe(char *cmd[], char *input, char *output, int flag, int background);
void sim_exe(char *cmd[], int background);

int main(int argc, char*argv[]){
    char buf[BUFSIZ];
    int x_flag = 0, c_flag = 0;
    char *query_string = NULL;
    char *line = NULL;
    line = (char *)malloc((size_t)BUFSIZ);
    size_t size;
    int len = 0;
    char opt;
    char *tokens[MAXTOKEN];
    int tokens_num = 0;
    signal(SIGINT, SIG_IGN);
    memset(buf,'\0',BUFSIZ);
    if(getcwd(buf, BUFSIZ) == NULL)
        fprintf(stderr, "getcwd err,errno: %d\n",errno);
    if(setenv("SHELL", buf, 1) == -1)
        fprintf(stderr, "setenv err,errno: %d\n",errno);
    while((opt = getopt(argc, argv, "xc:")) != -1){
        switch(opt){
        case 'x' :
            x_flag = 1;
            break;
        case 'c' :
            c_flag = 1;
            query_string = optarg;
            break;
        case '?':
            fprintf(stderr,"sish [ −x] [ −c command]\n");
            exit(EXIT_FAILURE);
        }
    }
    if(c_flag){
        query_string = replace(query_string, "<", " < ");
        query_string = replace(query_string, "&", " & ");
        query_string = replace(query_string, "|", " | ");
        query_string = replace(query_string, ">>", " >> ");
        query_string = add_space(query_string);
        query_string = trim_string(query_string);
        if(query_string == NULL){
            fprintf(stderr, "parse err\n");
            exit(EXIT_FAILURE);
        }
        if ((tokens[0] = strtok(query_string, " \t\n")) == NULL) {
            fprintf(stderr,"sish [ −x] [ −c command]\n");
            exit(EXIT_FAILURE);
        }
        tokens_num++;
        while ((tokens[tokens_num] = strtok(NULL, " \t\n")) != NULL)
            tokens_num++;
        if(parse_tokens(tokens_num,tokens,x_flag) < 0)
            fprintf(stderr, "parse_token err\n");
    }else{
        while (1) {
            printf("sish$ ");
            tokens_num = 0;
            line = NULL;
            if ((len = getline(&line, &size, stdin)) == -1){
                fprintf(stderr, "getline err\n");
                continue;
            }
            line[len - 1] = '\0';
            line = trim_string(line);
            if(strlen(line) == 0)
                continue;
            if (strcmp(line, "exit") == 0){
                if (x_flag)
                    fprintf(stderr,"+ exit\n");
                if (last_cmd)
                    exit(127);
                exit(EXIT_SUCCESS);
            }
            line = replace(line, "<", " < ");
            line = replace(line, "&", " & ");
            line = replace(line, "|", " | ");
            line = replace(line, ">>", " >> ");
            line = add_space(line);
            line = trim_string(line);
            if (line == NULL) {
                last_cmd = 1;
                fprintf(stderr, "read error");
                continue;
            }
            if ((tokens[0] = strtok(line, " \t")) == NULL) {
                fprintf(stderr,"sish: No tokens\n");
                continue;
            }
            tokens_num++;
            while ((tokens[tokens_num] = strtok(NULL, " \t")) != NULL)
                tokens_num++;
            parse_tokens(tokens_num,tokens,x_flag);
        }
    }
}
void exit_cmd(char *tokens[], int x_flag){
    if (x_flag) {
        int i = 0;
        fprintf(stderr, "+ ");
        while (tokens[i] != NULL) {
            fprintf(stderr, "%s ",tokens[i]);
            i++;
        }
        fprintf(stderr, "\n");
    }
    exit(EXIT_SUCCESS);
}
int cd_cmd(char *tokens[],int x_flag){
    if (tokens[1] == NULL){
        if(chdir(getenv("HOME")) == -1){
            last_cmd = 1;
            fprintf(stderr, "getenv error,errno: %d\n",errno);
            return  -1;
        }
        if (x_flag)
            fprintf(stderr,"+ cd\n");
        return 0;
    }
    if (chdir(tokens[1]) < 0) {
        if (x_flag) {
            int i = 0;
            fprintf(stderr,"+ ");
            while (tokens[i] != NULL){
                fprintf(stderr,"%s ",tokens[i]);
                i++;
            }
            fprintf(stderr,"\n");
        }
        fprintf(stderr,"cd: %s: No such file or directory\n",tokens[1]);
        last_cmd = 1;
        return -1;
    }
    return 0;
}
int echo_cmd(int tokens_num, char *tokens[], int x_flag){ 
    int i = 0;
    int fd;
    int std_out;
    if((std_out = dup(STDOUT_FILENO)) == -1){
        last_cmd = 1;
        close(std_out);
        fprintf(stderr, "dup err,errno: %d\n",errno);
        return  -1;
    }
    if (x_flag) {
        fprintf(stderr,"+ ");
        for (int j = 0; j < tokens_num; j++)
            fprintf(stderr,"%s ",tokens[j]);
        fprintf(stderr,"\n");
    }
    while (tokens[i] != NULL) {
        if (strcmp(">", tokens[i]) == 0){
            if (tokens[i + 1] == NULL) {
                fprintf(stderr,"syntax error near unexpected token `newline'\n");
                last_cmd = 1;
                return -1;
            }
            if((fd = open(tokens[i + 1], O_CREAT | O_TRUNC | O_WRONLY)) == -1){
                fprintf(stderr,"sish: %s: Create file error\n",tokens[i + 1]);
                last_cmd = 1;
                return -1;
            }
            if (dup2(fd, STDOUT_FILENO) == -1){
                last_cmd = 1;
                close(fd);
                fprintf(stderr, "dup2 err,errno: %d\n",errno);
                return  -1;
            }
            close(fd);
            break;
        }
        if (strcmp(">>", tokens[i]) == 0){
            if (tokens[i + 1] == NULL) {
                fprintf(stderr,"syntax error near unexpected token `newline'\n");
                last_cmd = 1;
                return -1;
            }
            if((fd = open(tokens[i + 1], O_CREAT | O_APPEND | O_WRONLY, DFTPERMISSION)) == -1){
                fprintf(stderr,"sish: %s: Create file error\n",tokens[i + 1]);
                last_cmd = 1;
                return -1;
            }
            if(dup2(fd, STDOUT_FILENO) == -1){
                last_cmd = 1;
                close(fd);
                fprintf(stderr, "dup2 err,errno: %d\n",errno);
                return  -1;
            }
            close(fd);
            break;
        }
        i++;
    }
    for (int j = 1; j < i; j++) {
        if(strcmp("$$", tokens[j]) == 0)
            printf("%d ",(int)getpid());
        else if (strcmp("$?", tokens[j]) == 0) 
            printf("%d ",last_cmd);
        else
            printf("%s ",tokens[j]);
    }
    printf("\n");
    last_cmd = 0;
    if(dup2(std_out, STDOUT_FILENO) == -1){
        last_cmd = 1;
        close(std_out);
        fprintf(stderr, "dup2 err,errno: %d\n",errno);
        return  -1;
    }
    return 0;
}
char *replace (const char *src, const char *old, const char *new) {
    char *result = NULL;
    int i, cnt = 0;
    int new_len = strlen(new);
    int old_len = strlen(old);
    for (i = 0; src[i] != '\0'; i++) {
        if (strstr(&src[i], old) == &src[i]) {
            cnt++;
            i += old_len - 1;
        }
    }
    result = (char *)malloc(i + cnt * (new_len - old_len) + 1);
    i = 0;
    while (*src) {
        if (strstr(src, old) == src) {
            strcpy(&result[i], new);
            i += new_len;
            src += old_len;
        }
        else
            result[i++] = *src++;
    }
    result[i] = '\0';
    return result;
}
/* use strtok to solve > and >> */
char* add_space(const char *line){
    char *result;
    result = (char*)malloc((size_t)BUFSIZ);
    int i = 0, j = 0;
    while(i < BUFSIZ - 1 && line[i] != '\0' && j < BUFSIZ - 1){
        if (isspace(line[i]) !=0 || isspace(line[i + 1]) != 0 || line[i + 1] != '>') {
            result[j++] = line[i++];
            continue;
        }
        if(line[i] == '>'){
            result[j++] = line[i++];
            result[j++] = line[i++];
            continue;
        }
        result[j++] = line[i++];
        result[j] = ' ';
        j++;
    }
    result[BUFSIZ - 1] = '\0';
    return result;
}
char* trim_string(const char *line){
    char *result = NULL;
    result = (char *)malloc((size_t)BUFSIZ);
    strcpy(result,line);
    char *end = NULL;
    while (isspace(*result))
        result++;
    if (*result == 0)
        return  result;
    end = result + strlen(result) - 1;
    while (end > result && isspace(*end))
        end--;
    end[1] = '\0';
    return result;
}

/*
    out  in  out  in  out  in   out     in   out
    cmd0     cmd1     cmd2     cmd3    ...  ...   cmdn
    p_out    p_in     p_out    p_in  ...
    n % 2 == 0 -> p_in to in p_out to out
*/
void pipe_exe(int tokens_num, char *tokens[], int x_flag){
    int pipe_num = 0;
    int temp, status;
    int fd_out[2];
    int fd_in[2];
    pid_t pid;
    int std_out;
    std_out = dup(STDOUT_FILENO);
    for (int i = 0; i < tokens_num; i++) {
        if (strcmp("|", tokens[i]) == 0){
            if ((i - 1 == -1) || (tokens[i - 1] == NULL)) {
                fprintf(stderr, "syntax error near unexpected token `|'\n");
                last_cmd = 1;
                return;
            }
            if (tokens[i + 1] == NULL) {
                fprintf(stderr, "syntax error near unexpected token `|'\n");
                last_cmd = 1;
                return;
            }
            pipe_num++;
        }
    }
    mycmd cmd[pipe_num + 1];
    memset(cmd, 0, sizeof(cmd));
    for (int i = 0, j = 0; i < pipe_num + 1; i++) {
        temp = 0;
        if (x_flag)
            fprintf(stderr, "+ ");
        for (; tokens[j] != NULL; j++) {
            if(strcmp("|", tokens[j]) == 0){
                j++;
                break;
            }
            cmd[i].cmd_token[temp] = tokens[j];
            if (x_flag)
                fprintf(stderr, "%s ",tokens[j]);
            temp++;
            cmd[i].tk_len++;
        }
        if (x_flag)
            fprintf(stderr, "\n");
    }
    for (int i = 0; i < pipe_num + 1; i++) {
        if(i % 2 == 0)
            pipe(fd_out);
        else
            pipe(fd_in);
        if((pid = fork()) == -1){
            last_cmd = 1;
            if(i != pipe_num){
                (i % 2 == 0) ? close(fd_out[1]) : close(fd_in[1]);
            }
            fprintf(stderr, "pipe fork error\n");
            return;
        }else if (pid > 0) {
            signal(SIGCHLD, SIG_IGN);
            if (i == 0)
                close(fd_out[1]);
            else if(i == pipe_num)
                (i % 2 == 0) ? close(fd_out[0]) : close(fd_in[0]);
            else {
                if (i % 2 == 0) {
                    close(fd_out[0]);
                    close(fd_in[1]);
                }else {
                    close(fd_out[1]);
                    close(fd_in[0]);
                }
            }
            waitpid(pid, &status, 0);
            if ( i == pipe_num ) {
                if (status)
                    last_cmd = 127;
            }
        }else if (pid == 0){
            signal(SIGINT, SIG_DFL);
            if(i == 0){
                if (dup2(fd_out[1], STDOUT_FILENO) == -1) {
                    close(fd_out[1]);
                    fprintf(stderr, "pipe dup2 err\n");
                    kill(getpid(),SIGTERM);
                }
            }else if (i == pipe_num) {
                if ((pipe_num + 1) % 2 == 0) {
                    if (dup2(fd_out[0], STDIN_FILENO) == -1){
                        close(fd_out[0]);
                        fprintf(stderr, "pipe dup2 err\n");
                        kill(getpid(),SIGTERM);
                    }
                }else {
                    if(dup2(fd_in[0], STDIN_FILENO) == -1){
                        close(fd_in[0]);
                        fprintf(stderr, "pipe dup2 err\n");
                        kill(getpid(),SIGTERM);
                    }
                }
            }else if (i % 2 == 0) {
                if(dup2(fd_out[1], STDOUT_FILENO) == -1){
                    close(fd_out[1]);
                    fprintf(stderr, "pipe dup2 err\n");
                    kill(getpid(),SIGTERM);
                }
                if(dup2(fd_in[0], STDIN_FILENO) == -1){
                    close(fd_in[0]);
                    fprintf(stderr, "pipe dup2 err\n");
                    kill(getpid(),SIGTERM);
                }
            }else {
                if(dup2(fd_out[0], STDIN_FILENO) == -1){
                    close(fd_out[0]);
                    fprintf(stderr, "pipe dup2 err\n");
                    kill(getpid(),SIGTERM);
                }
                if(dup2(fd_in[1], STDOUT_FILENO) == -1){
                    close(fd_in[1]);
                    fprintf(stderr, "pipe dup2 err\n");
                    kill(getpid(),SIGTERM);
                }
            }
            if (execvp(cmd[i].cmd_token[0], cmd[i].cmd_token) == -1) {
                fprintf(stderr,"%s: command not found\n",cmd[i].cmd_token[0]);
                kill(getpid(), SIGTERM);
            }
        }
    }
}

int parse_tokens(int tokens_num,char *tokens[], int x_flag){
    int i = 0,j = 0;
    int temp;
    int background = 0;
    char *buf[MAXTOKEN];
    memset(buf,0,sizeof(buf));
    if (strcmp("exit", tokens[0]) == 0)
        exit_cmd(tokens, x_flag);
    if(strcmp("cd", tokens[0]) == 0)
        return cd_cmd(tokens,x_flag);
    if (strcmp("echo", tokens[0]) == 0)
        return echo_cmd(tokens_num, tokens, x_flag);
    while (tokens[j] != NULL) {
        if (strcmp("|", tokens[j]) == 0){
            pipe_exe(tokens_num, tokens, x_flag);
            return 0;
        }
        j++;
    }
    if (strcmp("&", tokens[tokens_num - 1]) == 0)
        background = 1;
    while (tokens[i] != NULL) {
        if((strcmp(">", tokens[i]) == 0) || (strcmp(">>", tokens[i]) == 0) || 
             (strcmp("&", tokens[i]) == 0) || (strcmp("<", tokens[i]) == 0))
            break;
        buf[i] = tokens[i];
        i++;
    }
    if (x_flag) {
        int i = 0;
        fprintf(stderr, "+ ");
        while (tokens[i] != NULL){
            fprintf(stderr,"%s ",tokens[i]);
            i++;
        }
        fprintf(stderr,"\n");
    }
    while (tokens[i] != NULL) {
        if(strcmp("&", tokens[i]) == 0)
            background = 1;
        if(strcmp("<", tokens[i]) == 0) {
            temp = i + 1;
            if (tokens[temp] == NULL) {
                fprintf(stderr,"Expect input argv after <\n");
                last_cmd = 1;
                return -1;
            }
            if ((tokens[temp + 1] != NULL ) && (strcmp(">", tokens[temp + 1]) == 0)) {
                if (tokens[temp + 2] == NULL) {
                    fprintf(stderr,"Expect output argv after >\n");
                    last_cmd = 1;
                    return -1;
                }
                red_exe(buf, tokens[temp], tokens[temp + 2], 0, background);
                return  0;
            }else if ((tokens[temp + 1] != NULL) && (strcmp(">>", tokens[temp + 1])) == 0){
                if (tokens[temp + 2] == NULL) {
                    fprintf(stderr,"syntax error near unexpected token `newline'\n");
                    last_cmd = 1;
                    return -1;
                }
                red_exe(buf, tokens[temp], tokens[temp + 2], REDIRECTION_APPEND, background);
                return 0;
            }else{
                red_exe(buf, tokens[temp], NULL, 0, background);
                return  0;
            }
        }
        if (strcmp(">", tokens[i]) == 0) {
            temp = i + 1;
            if (tokens[temp] == NULL) {
                fprintf(stderr,"syntax error near unexpected token `newline'\n");
                last_cmd = 1;
                return -1;
            }
            red_exe(buf, NULL, tokens[temp], 0, background);
            return 0;
        }
        if (strcmp(">>", tokens[i]) == 0) {
            temp = i + 1;
            if (tokens[temp] == NULL) {
                fprintf(stderr,"syntax error near unexpected token `newline'\n");
                last_cmd = 1;
                return -1;
            }
            red_exe(buf, NULL, tokens[temp], REDIRECTION_APPEND, background);
            return  0;
        }
        i++;
    }
    sim_exe(buf,background);
    return 0;
}
void red_exe(char *cmd[], char *input, char *output, int append, int background){
    int fd,status;
    pid_t pid;
    if ((pid = fork()) == -1) {
        last_cmd = 1;
        fprintf(stderr,"Redirection fork error\n");
        return;
    }
    if (pid == 0) {
        //        if (background == 0)
        signal(SIGINT, SIG_DFL);
        if (input) {
            if((fd = open(input, O_RDONLY)) == -1){
                fprintf(stderr,"sish: %s: No such file or directory\n",input);
                close(fd);
                kill(getpid(), SIGTERM);
            }
            if(dup2(fd, STDIN_FILENO) == -1){
                fprintf(stderr, "dup2 err,errno: %d\n",errno);
                close(fd);
                kill(getpid(), SIGTERM);
            }
            close(fd);
        }
        if (output) {
            if (append) {
                if((fd = open(output, O_CREAT | O_TRUNC | O_WRONLY, DFTPERMISSION)) == -1){
                    fprintf(stderr,"sish: %s: Create file error\n",output);
                    close(fd);
                    kill(getpid(), SIGTERM);
                }
                if(dup2(fd, STDOUT_FILENO) == -1){
                    fprintf(stderr, "dup2 err,errno: %d\n",errno);
                    close(fd);
                    kill(getpid(), SIGTERM);
                }
                close(fd);
            }
            if((fd = open(output, O_CREAT | O_APPEND | O_WRONLY, DFTPERMISSION)) == -1){
                fprintf(stderr,"sish: %s: Create file error\n",output);
                close(fd);
                kill(getpid(), SIGTERM);
            }
            if(dup2(fd, STDOUT_FILENO) == -1){
                fprintf(stderr, "dup2 err,errno: %d\n",errno);
                close(fd);
                kill(getpid(), SIGTERM);
            }
            close(fd);
        }
        if (execvp(cmd[0], cmd) == -1) {
            fprintf(stderr,"%s: Command not found\n",cmd[0]);
            kill(getpid(), SIGTERM);
        }
    }else {
        if (background == 0) {
            waitpid(pid, &status, 0);
            if (status)
                last_cmd = 127;
        }else {
            signal(SIGCHLD, SIG_IGN);
            printf("Pid: %d\n",pid);
        }
    }
}
/* simple cmd without redirection */
void sim_exe(char *cmd[], int background){
    pid_t pid;
    int status;
    if ((pid = fork()) < 0) {
        last_cmd = 1;
        fprintf(stderr,"simple cmd fork error\n");
        return;
    }
    if (pid == 0) {
        //        if (background == 0)
        signal(SIGINT, SIG_DFL);
        if (execvp(cmd[0], cmd) == -1) {
            fprintf(stderr,"%s: command not found\n",cmd[0]);
            kill(getpid(), SIGTERM);
        }
    }else {
        if (background == 0) {
            waitpid(pid,&status,0);
            if (status)
                last_cmd = 127;
        }else {
            signal(SIGCHLD, SIG_IGN);
            printf("Pid: %d\n",pid);
        }
    }
}
