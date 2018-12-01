#include <stdio.h>
#include <stdlib.h>
int main(int argc, char *argv[]) {
	char *envir = NULL;
	envir = getenv("HELLO");
//	env = getenv("HOME");
	if (envir) 
		printf("WORLD \n");
}