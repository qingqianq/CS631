#include <sys/wait.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
int command(const char *string, char *outbuf, int outlen, char *errbuf, int errlen);
int main(int argc, char *argv[]){
  
    int i;
    char cmd[BUFSIZ];
    for (i = 1; i < argc; i++) {		/* read command from terminal */
	strcat(cmd, argv[i]);
	strcat(cmd," ");
    }
  
    char out[BUFSIZ], err[BUFSIZ];
    if (command(cmd, out, BUFSIZ, err, BUFSIZ) == -1) {
	perror("command");
	exit(EXIT_FAILURE);
    }
    printf("stdout:\n%s\n", out);
    printf("stderr:\n%s\n", err);
    return EXIT_SUCCESS;
}

int command(const char *string, char *outbuf, int outlen, char *errbuf, int errlen){
    int outfd[2];
    int errfd[2];
    int pid,lenth;
    if(string == NULL || strlen(string) == 0){
	perror("null command");
	return -1;
    }
    if(pipe(errfd) < 0){
	perror("create errfd error");
	return -1;
    }
    if(pipe(outfd) < 0){
	perror("create outfd error");
	return -1;
    }
    if((pid = fork()) <0){
	perror("fork error");
	exit(127);
    }
    if(pid > 0){ /* father write ls to pipe */
	close(outfd[0]);
	/* make out to outfd1 */
	if(dup2(outfd[1],STDOUT_FILENO) != STDOUT_FILENO){
	    perror("dup2 error");
	    return -1;
	}
	close(errfd[0]);
	/*error to errfd */
	if (dup2(errfd[1], STDERR_FILENO) != STDERR_FILENO) {
	    perror("dup2 error");
	    return -1;
	}
	execlp("/bin/sh","sh","-c",string, NULL);
	close(outfd[1]);
	close(errfd[1]);
	if (waitpid(pid, NULL, 0) < 0) {
	    perror("waitpid error");
	    return -1;
	}
    }
    if(pid == 0) { /* chilid read from the pipe */
	close(outfd[1]);
	close(errfd[1]);
    
	if((lenth = read(outfd[0], outbuf, outlen)) == -1){
	    perror("read pipe error");
	    return -1;
	}
	if((lenth = read(errfd[0], errbuf, errlen)) == -1){
	    perror("read pipe error");
	    return -1;
	}
	outbuf[BUFSIZ - 1] = '\0'; /* reduce mojibake */
	errbuf[BUFSIZ - 1] = '\0';
	close(outfd[0]);
	close(errfd[0]);
    }
  
    return EXIT_SUCCESS;
}
