#include <sys/wait.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
// open irony company mode first;

#define MAXTOKEN 100
#define REDIRECTION_APPEND 1

typedef struct mycmd{
    char *cmd_token[MAXTOKEN];
    int infd;
    int outfd;
    int tk_len;
}mycmd;

int last_cmd = 0;

int cd_cmd(char *tokens[],int x_flag);
int echo_cmd(int tokens_num, char *tokens[], int x_flag);
char *trim_string(const char *line);
char *replace (const char *src, const char *old, const char *new);
char *add_space(const char *line);
int pipe_exe(int tokens_num, char **tokens,int x_flag);
int parse_tokens(int tokens_num, char *tokens[], int flag);
void red_exe(char *cmd[], char *input, char *output, int flag, int background);
void sim_exe(char *cmd[], int background);
int main(int argc, char*argv[]){
    char buf[BUFSIZ];
    int x_flag = 0, c_flag = 0;
    char *query_string = NULL;
    char *line = NULL;
    size_t size;
    int len = 0;
    char opt;
    char *tokens[MAXTOKEN];
    int tokens_num = 0;
    signal(SIGINT, SIG_IGN);
    memset(buf,'\0',BUFSIZ);
    getcwd(buf, BUFSIZ);
    setenv("SHELL", buf, 1);
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
            perror("getopt error");
            exit(EXIT_FAILURE);
        }
    }
//    if(x_flag){
//        while (1) {
//            printf("sish$ ");
//            tokens_num = 0;
//            len = getline(&line, &size, stdin); 
//            line[len - 1] = '\0';
//            line = trim_string(line);
//            if(strlen(line) == 0)
//                continue;
//            if (strcmp(line, "exit") == 0){
//                printf("+ exit\n");
//                if (last_cmd)
//                    exit(127);
//                exit(EXIT_SUCCESS);
//            }                
//            /* to use strtok add space before and after characters*/
//            line = replace(line, "<", " < ");
//            line = replace(line, "&", " & ");
//            line = replace(line, "|", " | ");
//            line = replace(line, ">>", " >> ");
//            line = add_space(line);
//            line = trim_string(line);
//            if ((tokens[0] = strtok(line, " \t")) == NULL) {
//                printf("sish: No tokens\n");
//                continue;
//            }
//            tokens_num++;
//            while ((tokens[tokens_num] = strtok(NULL, " \t")) != NULL)
//                tokens_num++;
//            parse_tokens(tokens_num,tokens,x_flag);
//        }
//    }else 
    if(c_flag){
        query_string = replace(query_string, "<", " < ");
        query_string = replace(query_string, "&", " & ");
        query_string = replace(query_string, "|", " | ");
        query_string = replace(query_string, ">>", " >> ");
        query_string = add_space(query_string);
        query_string = trim_string(query_string);
        if ((tokens[0] = strtok(query_string, " \t\n")) == NULL) {
            printf("sish [ −x] [ −c command]\n");
            exit(EXIT_FAILURE);
        }
        tokens_num++;
        while ((tokens[tokens_num] = strtok(NULL, " \t")) != NULL)
            tokens_num++;
        parse_tokens(tokens_num,tokens,x_flag);
    }else{
         while (1) {
            printf("sish$ ");
            tokens_num = 0;
            len = getline(&line, &size, stdin); 
            line[len - 1] = '\0';
            line = trim_string(line);
            if(strlen(line) == 0)
                continue;
            if (strcmp(line, "exit") == 0){
                if (x_flag)
                    printf("+ exit\n");
                if (last_cmd)
                    exit(127);
                exit(EXIT_SUCCESS);
            }                
            /* to use strtok add space before and after characters*/
            line = replace(line, "<", " < ");
            line = replace(line, "&", " & ");
            line = replace(line, "|", " | ");
            line = replace(line, ">>", " >> ");
            line = add_space(line);
            line = trim_string(line);
            if ((tokens[0] = strtok(line, " \t")) == NULL) {
                printf("sish: No tokens\n");
                continue;
            }
            tokens_num++;
            while ((tokens[tokens_num] = strtok(NULL, " \t")) != NULL)
                tokens_num++;
            parse_tokens(tokens_num,tokens,x_flag);
        }
    }
}
int cd_cmd(char *tokens[],int x_flag){
    if (tokens[1] == NULL){
        chdir(getenv("HOME"));
        if (x_flag) {
            printf("+ cd\n");
        }  
        return 0;
    }
    if (chdir(tokens[1]) < 0) {
        if (x_flag) {
            int i = 0;
            printf("+ ");
            while (tokens[i] != NULL){
                printf("%s ",tokens[i]);
                i++;
            }
            printf("\n");
        }  
        printf("cd: %s: No such file or directory\n",tokens[1]);
        last_cmd = 1;
        return -1;
    }   
    return 0;
}
/* $? is zero to success 1 to fail */
int echo_cmd(int tokens_num, char *tokens[], int x_flag){ 
    int i = 0;
    int fd;
    int std_out = dup(STDOUT_FILENO);
    if (x_flag) {
        printf("+ ");
        for (int j = 0; j < tokens_num; j++)
            printf("%s ",tokens[j]);
        printf("\n");
    }
    while (tokens[i] != NULL) {
        if (strcmp(">", tokens[i]) == 0){
            if (tokens[i + 1] == NULL) {
                printf("Expect output argv after >\n");
                last_cmd = 1;
                return -1;
            }
            if((fd = open(tokens[i + 1], O_CREAT | O_TRUNC | O_WRONLY, 0600)) == -1){
                printf("sish: %s: Create file error\n",tokens[i + 1]);
                last_cmd = 1;
                return -1;
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            break;
        }
        if (strcmp(">>", tokens[i]) == 0){
            if (tokens[i + 1] == NULL) {
                printf("Expect output argv after >>\n");
                last_cmd = 1;
                return -1;
            }
            if((fd = open(tokens[i + 1], O_CREAT | O_APPEND | O_WRONLY, 0600)) == -1){
                printf("sish: %s: Create file error\n",tokens[i + 1]);
                last_cmd = 1;
                return -1;
            }
            dup2(fd, STDOUT_FILENO);            
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
    dup2(std_out, STDOUT_FILENO);

    return 0;
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
    if (*result == 0) /* end with '\0'*/
        return  result;
    end = result + strlen(result) - 1;
    while (end > result && isspace(*end))
        end--;
    end[1] = '\0';
    return result;
}
char *replace (const char *src, const char *old, const char *new) { 
    char *result; 
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
int pipe_exe(int tokens_num, char *tokens[], int x_flag){
    int pipe_num = 0;
    int temp;
    for (int i = 0; i < tokens_num; i++) {
        if (strcmp("|", tokens[i]) == 0) 
            pipe_num++;
    }
    printf("pipe_num:%d\n",pipe_num);
    mycmd cmd[pipe_num + 1];
    memset(cmd, 0, sizeof(cmd));
    for (int i = 0, j = 0; i < pipe_num + 1; i++) {
        cmd[i].infd = STDIN_FILENO;
        cmd[i].outfd = STDOUT_FILENO;
        temp = 0;
        for (; tokens[j] != NULL; j++) {
            if(strcmp("|", tokens[j]) == 0){
                j++;
                break;
            }
            cmd[i].cmd_token[temp] = tokens[j];
            temp++;
            cmd[i].tk_len++;
        }
    }
    return 0;
}

int parse_tokens(int tokens_num,char *tokens[], int x_flag){
    int i = 0;
    int temp;
    int background = 0;
    char *buf[MAXTOKEN];
    memset(buf,0,sizeof(buf));

    if(strcmp("cd", tokens[0]) == 0){
        cd_cmd(tokens,x_flag);
        return 0;
    }
    if (strcmp("echo", tokens[0]) == 0){
        echo_cmd(tokens_num, tokens, x_flag);
        return 0;
    }
/* handle commands not build in */  
    if (strcmp("&", tokens[tokens_num - 1]) == 0)
        background = 1;
    while (tokens[i] != NULL) {    // read the tokens before redirection
        if((strcmp(">", tokens[i]) == 0) || (strcmp(">>", tokens[i]) == 0) || 
        (strcmp("&", tokens[i]) == 0) || (strcmp("<", tokens[i]) == 0) || (strcmp("|", tokens[i])) ==0){
            break;
        }
        buf[i] = tokens[i];
        i++;
    }
    if (x_flag) {
        int i = 0;
        printf("+ ");
        while (buf[i] != NULL){
            printf("%s ",buf[i]);
            i++;
        }
        printf("\n");
    }  
    while (tokens[i] != NULL) {
        if(strcmp("&", tokens[i]) == 0)
            background = 1;
        if (strcmp("|", tokens[i]) == 0) {
            pipe_exe(tokens_num, tokens, x_flag);
            return 0;
        }
        if(strcmp("<", tokens[i]) == 0) {
            temp = i + 1;
            if (tokens[temp] == NULL) {
                printf("Expect input argv after <\n");
                last_cmd = 1;
                return -1;
            }
            /* aed -e <file >file.enc */
            if ((tokens[temp + 1] != NULL ) && (strcmp(">", tokens[temp + 1]) == 0)) {
                if (tokens[temp + 2] == NULL) {
                    printf("Expect output argv after >\n");
                    last_cmd = 1;
                    return -1;
                }
                red_exe(buf, tokens[temp], tokens[temp + 2], 0, background);
                return  0;
            }else if ((tokens[temp + 1] != NULL) && (strcmp(">>", tokens[temp + 1])) == 0){
                if (tokens[temp + 2] == NULL) {
                    printf("Expect output argv after >>\n");
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
                printf("Expect output argv after >\n");
                last_cmd = 1;
                return -1;
            }
            red_exe(buf, NULL, tokens[temp], 0, background);
            return 0;
        }
        if (strcmp(">>", tokens[i]) == 0) {
            temp = i + 1;
            if (tokens[temp] == NULL) {
                printf("Expect output argv after >>\n");
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
        printf("Redirection fork error\n");
        return;
    }
    if (pid == 0) {
//        if (background == 0)
            signal(SIGINT, SIG_DFL);
        if (input) {
            if((fd = open(input, O_RDONLY)) == -1){
                printf("sish: %s: No such file or directory\n",input);
                last_cmd = 1;
                return;
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        if (output) {
            if (append) {
                if((fd = open(output, O_CREAT | O_TRUNC | O_WRONLY, 0600)) == -1){
                    printf("sish: %s: Create file error\n",output);
                    last_cmd = 1;
                    return;
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            if((fd = open(output, O_CREAT | O_APPEND | O_WRONLY, 0600)) == -1){
                printf("sish: %s: Create file error\n",output);
                last_cmd = 1;
                return;
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        if (execvp(cmd[0], cmd) == -1) {
            printf("%s: command not found\n",cmd[0]);
            kill(getpid(), SIGTERM);
        }
    }else {
        if (background == 0) {
            waitpid(pid, &status, 0);
            if (status) 
                last_cmd = 1;
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
        printf("simple cmd fork error\n");
    }
    if (pid == 0) {
//        if (background == 0)
            signal(SIGINT, SIG_DFL);
        if (execvp(cmd[0], cmd) == -1) {
            printf("%s: command not found\n",cmd[0]);
            kill(getpid(), SIGTERM);
        }
    }else {
        if (background == 0) {            
            waitpid(pid,&status,0);
            if (status) 
                last_cmd = 1;
        }else {
            signal(SIGCHLD, SIG_IGN);
            printf("Pid: %d\n",pid);
        }
    }
}