#ifndef _SISH_
#define _SISH_
#define MAXTOKEN 100
#define DFTPERMISSION 0644
#define REDIRECTION_APPEND 1

typedef struct mycmd{
	char *cmd_token[MAXTOKEN];
	int tk_len;
}mycmd;

int cd_cmd(char *tokens[],int x_flag);
int echo_cmd(int tokens_num, char *tokens[], int x_flag);
void exit_cmd(char *tokens[], int x_flag);
char *trim_string(const char *line);
/* to use strtok to parse string, use " > /dev/null" insetead ">/dev/null*/
char *replace (const char *src, const char *old, const char *new);
/* seperate > and // before use strtok*/
char *add_space(const char *line);
void pipe_exe(int tokens_num, char **tokens,int x_flag);
int parse_tokens(int tokens_num, char *tokens[], int flag);
void red_exe(char *cmd[], char *input, char *output, int flag, int background);
/* simple cmd withou redirection and pipe*/
void sim_exe(char *cmd[], int background);
#endif