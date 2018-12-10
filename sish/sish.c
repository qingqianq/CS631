#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
// open irony company mode first;
char current_dir[BUFSIZ];
pid_t pid;
int last_cmd = 0;

int cd_cmd(const char *path);
int echo_cmd(const char *path);
const char* query_parse(const char *query_string);
char* trim_string(const char *line);
char *replace (const char *src, const char *old, const char *new);
char *add_space(const char *line);
int main(int argc, char*argv[]){
    int x_flag = 0, c_flag = 0;
    const char * query_string = NULL;
    char *line = NULL;
    size_t size;
    int len = 0;
    char opt;
    char *tokens[];
    signal(SIGINT, SIG_IGN);
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
            if(strtok(line, " \r") == NULL)
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
char* add_space(char *line){
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
