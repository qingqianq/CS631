#include <sys/types.h>
#include <sys/stat.h>

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>
#include "cmp.h"



/* Read dir*/
mydir * readFile(const char *pathname);
int copyStat(myfile *fp, const struct stat *sp, const char *pathname);
int copyLinkName(myfile *fp, const char *filename);
int freeAllDir(mydir *dp);
int digitlen(int n);


int modeToStr(int mode, char *buf);
int printType(int mode);
int sizeChange(int size);

int rowPrint(mydir *dp);
int lprint(mydir *dp);
void printTime(int64_t t);
void q_copy(char *a, char *b);



static int A_flg, a_flg, C_flg, c_flg, d_flg, F_flg, f_flg, h_flg,
        i_flg,k_flg,l_flg, n_flg, q_flg, R_flg, S_flg, 
        s_flg, t_flg, u_flg, w_flg, x_flg;
int r_flg = 0;
static int(*Cmpfunc)(const void *, const void *);
static double blockSize;
static double blktimes;

        
int main(int argc, char *argv[]){
    register mydir *dp;
    int c;
    if (argc < 1) 
        fprintf(stderr, "Usage : ls [-Option] [dir] \n");
    Cmpfunc = cmpname;
    while ((c = getopt(argc, argv, "AaCcdFfhiklnqRrSstuwx"))!= -1){
        switch (c) {
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
            u_flg = 0;
            Cmpfunc = cmpctime;
            break;
        case 'd':
            d_flg = 1;
            R_flg = 0;
            break;
        case 'F':
            F_flg = 1;
            break;
        case 'f':
            f_flg = 1;
            break;
        case 'h':
            h_flg = 1;
            k_flg = 0;
            break;
        case 'i':
            i_flg = 1;
            break;
        case 'k':
            k_flg = 1;
            h_flg = 0;
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
            r_flg = -1;
            break;
        case 'S':
            S_flg = 1;
            Cmpfunc = cmpsize;
            break;
        case 's':
            s_flg = 1;
            break;
        case 't':
            t_flg = 1;
            Cmpfunc = cmpmtime;
            break;
        case 'u':
            u_flg = 1;
            c_flg = 0;
            Cmpfunc = cmpatime;
            break;
        case 'w':
            w_flg = 1;
            break;
        case 'x':
            x_flg = 1;
            break;
        default:
            printf("illegal option \n");
            exit(EXIT_FAILURE);
        }
    }
    r_flg = r_flg == 0 ? 1 : -1;
    blktimes = 1;
    if(k_flg && s_flg){
        if(getenv("BLOCKSIZE") != NULL){
            blockSize = atof(getenv("BLOCKSIZE"));
            if(blockSize < 0)
                blktimes = (double)DEFAUTBLOCKSIZE / (double)ONE_KB ;
            else 
                blktimes = blockSize / ONE_KB;
        }else {
            blockSize = ONE_KB;
            blktimes = (double)DEFAUTBLOCKSIZE / (double)ONE_KB;
        }
        setenv("BLOCKSIZE","1024",1);
    }
    if(argc == 1){
        if((dp = readFile(".")) != NULL){
            if(Cmpfunc)
                sortDirs(dp, Cmpfunc);
            rowPrint(dp);
            freeAllDir(dp);
            exit(EXIT_SUCCESS);
        }else 
            printf("ls '.' error.");
        exit(EXIT_FAILURE);
    }
    if (argc == 2) {
        if(argv[1][0] == '-'){
            dp = readFile(".");
            if(Cmpfunc)
                sortDirs(dp, Cmpfunc);
            if(l_flg || n_flg)
                lprint(dp);
            else 
                rowPrint(dp);
            freeAllDir(dp);
            exit(EXIT_SUCCESS);
        }else {
//            printf("argv1 %s \n",argv[1]);
            if((dp = readFile(argv[1])) != NULL){
                if(Cmpfunc)
                    sortDirs(dp, Cmpfunc);
                rowPrint(dp);
                freeAllDir(dp);
            }else 
                printf("Error: cannot ls '%s' \n",argv[1]);
            exit(EXIT_FAILURE);
        }
    }  
    if(argc > 2){
        if((dp = readFile(argv[2])) != NULL){
            if(Cmpfunc)
                sortDirs(dp, Cmpfunc);
            if(l_flg || n_flg){
                lprint(dp);
            }else 
                rowPrint(dp);
            freeAllDir(dp);
            exit(EXIT_SUCCESS);
        }else {
            if (R_flg)
                printf("-R maybe too many files.");
                printf("Error: cannot ls '%s' \n",argv[2]);
            exit(EXIT_FAILURE);
        }
    }    
}
/* Read a path and store it in mydir  
  
dir  
 | - file1 (dp not NULL )
 |     |--subfile1 (dp not NULL)
 |           |-- subfile1 (dp not NULL)
 |                  |-- sunsubfile (dp NULL)
 |   file2 (dp NULL)
 |   fileN...
*/

mydir * readFile(const char *pathname){
    int width;
    DIR *dp;
    mydir *dir;
    myfile *temp;
    struct stat  st;
    struct dirent *direp;
    char home[MAXNAMLEN + 1];
   
    if(getcwd(home, MAXNAMLEN + 1) == NULL)    //current dir
        return NULL;
    if ((stat(pathname, &st)) == -1) 
        return  NULL;
    if((dir = calloc(1,sizeof(mydir))) == NULL)
        return NULL;
    if ((dir->fp = malloc(sizeof(myfile))) == NULL) {
        free(dir);
        return NULL;
    }
    /* update once when ld or nd */
    if(d_flg ){
        if(l_flg || n_flg){
            copyStat(dir->fp, &st, pathname);
            dir->fno = 1;
            lprint(dir);
        }else {
            printf(".\n");
        }        
        exit(EXIT_SUCCESS);
    }
    if (!S_ISDIR(st.st_mode)) {
        dir->fno = 1;
        copyStat(dir->fp, &st, pathname);
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
    /* create myfile to store info */
    for(dir->fno = 0; (direp = readdir(dp)) != NULL; dir->fno++ ){ 
        if (direp->d_name[0] == '.' && A_flg == 0 && a_flg == 0) {
            dir->fno--;
            continue;
        }
        if ((strcmp(direp->d_name, ".") == 0 || strcmp(direp->d_name, "..") == 0) && a_flg == 0) {
            dir->fno--;
            continue;
        }
        if ((temp = realloc(dir->fp, (dir->fno + 1) * sizeof(myfile))) == NULL) {
            freeAllDir(dir);   
            chdir(home);
            closedir(dp);
            return  NULL;
        }
        dir->fp = temp; /* relloc may change the pointer if there is no enough space */
        if (stat(direp->d_name, &st) == -1) {
            printf("illegal name");
            continue;
        }
        if(q_flg){
            q_copy(direp->d_name, direp->d_name);
        }
        copyStat(dir->fp + dir->fno, &st, direp->d_name);
        copyLinkName(dir->fp + dir->fno,direp->d_name);
        /* update argv in mydir try to calculate width  try to implement format */
        if (i_flg && (width = digitlen(dir->fp[dir->fno].ino)) > dir->iwid) {
            dir->iwid = width;
        }
        if(l_flg || n_flg){
            if ((width = digitlen(dir->fp[dir->fno].links)) > dir->lwid) {
                dir->lwid = width;
            }
            if ((width = digitlen(dir->fp[dir->fno].uid)) > dir->lwid) {
                dir->lwid = width;
            }
            if ((width = digitlen(dir->fp[dir->fno].links)) > dir->lwid) {
                dir->lwid = width;
            }
        }
        if(R_flg){
            if(strcmp(direp->d_name, ".") == 0 || strcmp(direp->d_name, "..") == 0 )
                continue;
            char *tmp = &(dir->fp + dir->fno)->linkName;
            if (strcmp(tmp, ".") == 0) {
                printf( "Error : loop dir\n");
                return NULL;
            }
            if(S_ISDIR(st.st_mode) && ((dir->fp[dir->fno].dp = readFile(direp->d_name)) == NULL)){
                freeAllDir(dir);
                chdir(home);
                closedir(dp);
                return NULL;
            }
        }
    }
    chdir(home);
    closedir(dp);
    return dir;
}
/* from stat mode */
int modeToStr(int mode, char *buf){
    strcpy(buf,"----------");
    if(S_ISDIR(mode))
        buf[0] = 'd';
    if(S_ISCHR(mode))
        buf[0] = 'c';
    if(S_ISBLK(mode))
        buf[0] = 'b';
    if(S_ISLNK(mode))
        buf[0] = 'l';
    if(S_ISFIFO(mode))
        buf[0] = 'p';
    if(S_ISSOCK(mode))
        buf[0] = 's';
    if(mode & S_IRUSR)
        buf[1] = 'r';
    if(mode & S_IWUSR)
        buf[2] = 'w';
    if(mode & S_IXUSR)
        buf[3] = 'x';
    if(mode & S_ISUID)
        buf[3] = 's';
    if(mode & S_IRGRP)
        buf[4] = 'r';
    if(mode & S_IWGRP)
        buf[5] = 'w';
    if(mode & S_IXGRP)
        buf[6] = 'x';
    if(mode & S_ISGID)
        buf[6] = 's';
    if(mode & S_IROTH)
        buf[7] = 'r';
    if(mode & S_IWOTH)
        buf[8] = 'w';
    if(mode & S_IXOTH)
        buf[9] = 'x';
    if(mode & S_ISVTX)
        buf[9] = 't';
    return 0;
}


/* readlink(2) find linkName*/
int copyLinkName(myfile *fp, const char *filename){
    char pathName[MAXNAMLEN + 1];
    int linklen;
    fp->linkName[0] = '\0';
    if((linklen = readlink(filename, pathName, sizeof(pathName) - 1)) != -1){
        pathName[linklen] = '\0';
        strcpy(fp->linkName, &pathName[0]);
    }
    return 0;
}

int copyStat(myfile *fp, const struct stat *sp, const char *pathname){
    fp->dp = NULL;
    fp->ino = sp->st_ino;
    fp->uid = sp->st_uid;
    fp->gid = sp->st_gid;
    fp->dev = sp->st_rdev;
    fp->mode = sp->st_mode;
    fp->size = sp->st_size;
    fp->a_time = sp->st_atime;
    fp->m_time = sp->st_mtime;
    fp->c_time = sp->st_ctime;
    fp->links = sp->st_nlink;
    fp->nameLen = strlen(pathname);
    fp->blocks = sp->st_blocks;
    strcpy(fp->name, pathname);
    return 0;
}

/*recursive free the mydir struct*/
int freeAllDir(mydir *dp){
    if (dp == NULL || dp->fp == NULL) 
        return -1;
    for (int i = 0;i < dp->fno ;i++) {
        if (dp->fp[i].dp) 
            freeAllDir(dp->fp[i].dp);
    }
    free(dp->fp);
    free(dp);
    return 0;
}
/* return digit of n  (-C) not implement */
int digitlen(int n){
    char buf[BUFSIZ];
    snprintf(buf, BUFSIZ,"%d",n);
    return strlen(buf);
}

void printTime(int64_t t){
    char buf[BUFSIZ];
    struct tm *tp;
    if ((tp = localtime((time_t*)&t)) == NULL) {
        snprintf(buf, BUFSIZ, "worng time");
    }else {
        snprintf(buf, BUFSIZ, "%d.%d.%d %d:%d:%d",tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday, tp->tm_hour,tp->tm_min, tp->tm_sec);
    }
    printf("%-18s ",buf);
}


int lprint(mydir *dp){
    char buf[BUFMODESIZE + 1];
    struct passwd *pw;
    struct group *gp;
    int i;
    if(dp == NULL || dp->fp == NULL)
        return -1;
    if(dp->fno == 0){
        putchar('\n');
        return 0;
    }
    for (i = 0; i < dp->fno; i++) {
        if(i_flg)
            printf("%-d ", dp->fp[i].ino);
        if(s_flg){
            printf("%-d ",(int)((dp->fp[i].blocks) * blktimes));
            putchar(' ');
        }
        modeToStr(dp->fp[i].mode, buf);
        printf("%s",buf);
        putchar(' ');
        printf("%d ",dp->fp[i].links);        
        pw = getpwuid(dp->fp[i].uid);
        gp = getgrgid(dp->fp[i].gid);
        if (l_flg) {
            if(pw != NULL)
            printf("%s ",pw->pw_name);
            printf("%s ",gp->gr_name);
        }
        if(n_flg){
            printf("%d %d ",dp->fp[i].uid,dp->fp[i].gid);
        }
        if (h_flg) {
            sizeChange(dp->fp[i].size);
            putchar(' ');
        }else {
            printf("%-d ",dp->fp[i].size);
        }
        if (c_flg) {
            printTime(dp->fp[i].c_time);
        }else if(u_flg) {
            printTime(dp->fp[i].a_time);
        }else
            printTime(dp->fp[i].m_time);
        if(d_flg){
            printf(".\n");
            return 0;
        }
        printf("%s",dp->fp[i].name);
        if(F_flg){
            printType(dp->fp[i].mode);
        }
        if(dp->fp[i].linkName[0] != '\0')
            printf("->%s",dp->fp[i].linkName);
        putchar('\n');
    }
    if(R_flg){
        for (i = 0;i < dp->fno; i++) {
            if(strcmp(dp->fp[i].name, ".") == 0 || strcmp(dp->fp[i].name, "..") == 0)
                continue;
            if(S_ISDIR(dp->fp[i].mode)){
                printf("\n%s:\n", dp->fp[i].name);
                lprint(dp->fp[i].dp);
            }
        }
    }
    return 0;
}
/* default print */
int rowPrint(mydir *dp){
    int i;
    if(dp == NULL || dp->fp == NULL)
        return -1;
    if(dp->fno == 0){
        putchar('\n');
        return 0;
    }
    for(i = 0; i < dp->fno; i++){
        if(i_flg)
            printf("%-d ", dp->fp[i].ino);
        if(s_flg){
            if(getenv("BLOCKSIZE") != NULL){
                blockSize = atof(getenv("BLOCKSIZE"));
            }else {
                blockSize = DEFAUTBLOCKSIZE;
            }
            printf("%-d  ",(int)((dp->fp[i].blocks) * (DEFAUTBLOCKSIZE/blockSize)));
        }
        printf("%s", dp->fp[i].name);
        if(F_flg)
            printType(dp->fp[i].mode);
        putchar('\n');
    }
    if(R_flg == 0)
        return 0;
    for(i = 0; i < dp->fno; i++){ 
        if(strcmp(dp->fp[i].name, ".") == 0 || strcmp(dp->fp[i].name, "..") == 0)
            continue;
        if(S_ISDIR(dp->fp[i].mode)){
            printf("%s:\n", dp->fp[i].name);
            rowPrint(dp->fp[i].dp);
        }
    }
        return 0;
}
/*change Byte to K M G */
int sizeChange(int size){
    int n = 0;
    int temp = size;
    while (temp > ONE_KB) {
        temp = temp / ONE_KB;
        n++;
    }
    printf("%d",temp);
    switch (n) {
    case 0:
        putchar('B');
        break;
    case 1:
        putchar('K');
        break;
    case 2:
        putchar('M');
        break;
    case 3:
        putchar('G');
        break;
    case 4:
        putchar('T');
        break;
    case 5:
        putchar('P');
        break;
    case 6:
        putchar('E');
        break;
    case 7:
        putchar('Z');
        break;
    default:
        printf("size Error");
        return -1;
    }
    return 0;
}

int printType(int mode){
    switch(mode & S_IFMT){
    case S_IFDIR:
        putchar('/');
        break;
    case S_IFLNK:
        printf("@");
        break;
    case S_IFSOCK:
        putchar('=');
        break;
    case S_IFIFO:
        putchar('|');
        break;
    default:
        if(mode & (S_IXUSR | S_IXGRP | S_IXOTH))
            putchar('*');
            return 1;
        break;
    }
    putchar(' ');
    return 0;
}
/* printable use ? instead other */
void q_copy(char *a, char *b){
    register char *p, *q;
    for(p = a, q = b; *p; p++, q++){
        if(isprint(*p))
            *q = *p;
        else
            *q = '?';
    }
}