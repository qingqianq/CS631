#include <sys/types.h>
#include <sys/stat.h>

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct mydir mydir;
typedef struct myfile myfile;
mydir * readFile(const char *pathname);
void copyStat(myfile *fp, const struct stat *sp, const char *pathname);
int lprint(mydir * dp);
        
struct mydir{
    int fno;
    int ino;
    int lno;
    int uno;
    int gno;
    int dno;
    myfile *fp;
};
/* dp is NULL to file and not NULL to point a mydir*/
struct myfile{
    mydir *dp; /* Null to file, current dir to dir */
    int nameLen;
    int uid;
    int gid;
    int dev;
    int mode;
    int links;
    int ino;
    int size;
//    int a_time;
//    int m_time;
//    int c_time;
    char name[MAXNAMLEN + 1];
};

static int A_flg, a_flg, C_flg, c_flg, d_flg, F_flg, f_flg, h_flg,
        i_flg,k_flg,l_flg, n_flg, q_flg, R_flg, r_flg, S_flg, 
        s_flg, t_flg, u_flg, w_flg, x_flg;
        
int main(int argc, char *argv[]){
    mydir *dp;
    if (argc < 1) 
        fprintf(stderr, "Usage : ls [-Option] [dir] \n");
    if(argc == 1){
        //basicprint();
        exit(EXIT_SUCCESS);
    }
    if(argv[2][0] == '-'){ /*load option*/
        char *p = argv[2];
        p++;
        for (int i = 0;i < strlen(p) ;i++) {
            char ch = *(p + i);
            switch (ch) {
            case '1':
                n_flg = 1;
                break;
            case 'A':
                A_flg = 1;
                break;
            case 'a':
                a_flg = 1;
                break;
            case 'C':
                C_flg = 1;    
                break;
            case 'c':
                c_flg = 1;
                break;
            case 'd':
                d_flg = 1;
                break;
            case 'F':
                F_flg = 1;
                break;
            case 'f':
                f_flg = 1;
                break;
            case 'h':
                h_flg = 1;
                break;
            case 'i':
                i_flg = 1;
                break;
            case 'k':
                k_flg = 1;
                break;
            case 'l':
                l_flg = 1;
                break;
            case 'n':
                n_flg = 1;
                break;
            case 'q':
                q_flg = 1;
                break;
            case 'R':
                R_flg = 1;
                break;
            case 'r':
                r_flg = 1;
                break;
            case 'S':
                S_flg = 1;
                break;
            case 's':
                s_flg = 1;
            case 't':
                t_flg = 1;
            case 'u':
                u_flg = 1;
            case 'w':
                w_flg = 1;
            case 'x':
                x_flg = 1;
            default:
                printf("illegal option -%c \n",ch);
            }
        }
        if (argc == 2) {
            // use current dir
            dp = readFile(".");
            if(l_flg)
                lprint(dp);
            
        }else if (argc == 3) { /*3 statements*/ 
            if(argv[2][0] != '-')
                printf("illegal option %s",argv[2]);
            dp = readFile(argv[3]);
            if(l_flg)
                lprint(dp);
        }else { 
            /* four argc error*/
            printf("more than 3 argv");
            exit(EXIT_FAILURE);
        }
    }else {
        // there is no option
        printf("no argv");
//        basicprintf()
    }
}
/* Read a path and store it in mydir  */
mydir * readFile(const char *pathname){
    int no;
    DIR *dp;
    mydir *dir;
    myfile *temp;
    struct stat *sp;
    struct dirent *direp;
    char home[MAXNAMLEN + 1];
    if(getcwd(home, MAXNAMLEN + 1) == NULL)    //current dir
        return NULL;
    if ((stat(pathname, sp)) == -1) 
        return  NULL;
    if((dir = calloc(1,sizeof(mydir))) == NULL)
        return NULL;
    if ((dir->fp = malloc(sizeof(myfile))) == NULL) {
        free(dir);
        return NULL;
    }
    if (!S_ISDIR(sp->st_mode)) {
        dir->fno = 1;
        copyStat(dir->fp, sp, pathname);
        return dir;
    }
    if (chdir(pathname) == -1) {
        free(dir->fp);
        free(dir);
        return NULL;
    }
    if((dp = opendir(".")) == NULL){
        free(dir->fp);
        free(dir);
        chdir(home);
        return NULL;
    }
    for(dir->fno = 0; direp = readdir(dp); dir->fno++ ){ 
        if (direp->d_name[0] == '.' && A_flg && a_flg) {
            dir->fno--;
            continue;
        }
        if ((strcmp(direp->d_name, ".")) || strcmp(direp->d_name, "..")) {
            dir->fno--;
            continue;
        }
        if ((temp = realloc(dir->fp, (dir->fno + 1) * sizeof(myfile))) == NULL) {
            free(dir);   
            chdir(home);
            closedir(dp);
            return  NULL;
        }
        dir->fp = temp;
        if (stat(direp->d_name, sp) == -1) {
            free(dir);
            chdir(home);
            closedir(dp);
            return  NULL;
        }
        copyStat(dir->fp + dir->fno, sp, direp->d_name);
        /* update argv in mydir */
//        if (i_flg && (w = digitlen(dir->fp[dir->fno].ino)) {
//            
//        }
//        if(l_flg){
//            
//        }
        //Rflg 
    }
    chdir(home);
    closedir(dp);
    return dir;
}

void copyStat(myfile *fp, const struct stat *sp, const char *pathname){
    fp->dp = NULL;
    fp->ino = sp->st_ino;
    fp->uid = sp->st_uid;
    fp->gid = sp->st_gid;
    fp->dev = sp->st_rdev;
    fp->mode = sp->st_mode;
    fp->size = sp->st_size;
    /*mode does not match*/
//    fp->a_time = sp->st_atime;
//    fp->m_time = sp->st_mtime;
//    fp->c_cime = sp->st_ctime;
    fp->links = sp->st_nlink;
    fp->nameLen = strlen(pathname);
    strcpy(fp->name, pathname);
}
int lprint(mydir * dp){
    int i;
    if(dp == NULL || dp->fp == NULL)
        return -1;
    if(dp->fno == 0)
        return 0;
    for (i = 0;i < dp->fno;i++) {
        if (d_flg) {
            printf("d");
        }
        if(i_flg)
            printf("i");
    }
      /* print mode*/
    return 0;
}
