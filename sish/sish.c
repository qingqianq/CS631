#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
// open irony company mode first;

#define MAXTOKEN 100

typedef struct mycmd{
    char *cmd_token[MAXTOKEN];
    int infd;
    int outfd;
    int tk_len;
}mycmd;

char current_dir[BUFSIZ];
pid_t pid;
int last_cmd = 0;

int cd_cmd(const char *path);
int echo_cmd(const char *path);
const char* query_parse(const char *query_string);
char* trim_string(const char *line);
char *replace (const char *src, const char *old, const char *new);
char *add_space(const char *line);
int handle_tokens(int tokens_num, char **tokens);
int main(int argc, char*argv[]){
    char buf[BUFSIZ];
    int x_flag = 0, c_flag = 0;
    const char * query_string = NULL;
    char *line = NULL;
    size_t size;
    int len = 0;
    char opt;
    char *tokens[MAXTOKEN];
    int tokens_num;
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
        }
    }
//    printf("%s/n",query_string);
    if(x_flag){
        printf("x_flag\n");
    }else if(c_flag){
        printf("c_flag\n%s\n",query_string);
    }else{
         while (1) {
            printf("sish$ ");
            tokens_num = 0;
            len = getline(&line, &size, stdin); 
            line[len - 1] = '\0';
            line = trim_string(line);
            if(strlen(line) == 0)
                continue;
            if (strcmp(line, "exit") == 0)
                exit(EXIT_SUCCESS);
            /* to use strtok add space before and after characters*/
            line = replace(line, "<", " < ");
            line = replace(line, "&", " & ");
            line = replace(line, "|", " | ");
            line = replace(line, ">>", " >> ");
            line = add_space(line);
            printf("%s\n",line);
            if ((tokens[0] = strtok(line, " \t")) == NULL) {
                printf("need not handle");
                continue;
            }
            tokens_num++;
            while ((tokens[tokens_num] = strtok(NULL, " \t")) != NULL)
                tokens_num++;
            handle_tokens(tokens_num,tokens);
        }
    }
}
int cd_cmd(const char *path){
    return 0;
}
int echo_cmd(const char *path){
    return 0;
}
const char* query_parse(const char *query_string){
    return NULL;
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
int handle_tokens(int tokens_num, char *tokens[]){
    int pipe_num = 0;
    for (int i = 0; i < tokens_num; i++) {
        if (strcmp("|", tokens[i]) == 0) 
            pipe_num++;
    }
    printf("pipe_num:%d\n",pipe_num);
    if (pipe_num == 0) {
        printf(" no pipe");
    }else {
        mycmd cmd[pipe_num + 1];
        memset(cmd, 0, sizeof(cmd));
        for (int i = 0, j = 0; i < pipe_num + 1; i++) {
            cmd[i].infd = STDIN_FILENO;
            cmd[i].outfd = STDOUT_FILENO;
            int temp = 0;
            for (; tokens[j] != NULL; j++) {
                if(strcmp("|", tokens[j]) == 0){
                    j++;
                    break;
                }
                cmd[i].cmd_token[temp] = tokens[j];
                printf("cmd[%d].cmd_tok[%d]:%s\n",i,temp,cmd[i].cmd_token[temp]);
                temp++;
                cmd[i].tk_len++;
            }
        }
    }
    return 0;
}