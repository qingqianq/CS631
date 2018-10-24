
#define BUFMODESIZE 10
#define DEFAUTBLOCKSIZE 512
#define ONE_KB 1024

typedef struct mydir mydir;
typedef struct myfile myfile;
//struct 
struct mydir{
	int fno;
	int iwid;
	int lwid;
	int uwid;
	int gwid;
	int dwid;
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
	int blocks;
	int64_t a_time;
	int64_t m_time;
	int64_t c_time;
	char name[MAXNAMLEN + 1];
	char linkName[MAXNAMLEN + 1];
};