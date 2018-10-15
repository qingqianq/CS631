#include <stdio.h>
#include "mystruct.h"
extern int r_flg;

int cmpname (const void *s1, const void *s2);
int cmpatime(const void *s1, const void *s2);
int cmpmtime(const void *s1, const void *s2);
int cmpctime(const void *s1, const void *s2);
int cmpsize(const void *s1, const void *s2);
int sortDirs(mydir *dp,int (*cmp)(const void *,const void *));

int cmpname (const void *s1, const void *s2){
	return r_flg * strcmp(((myfile*)s1)->name, ((myfile*)s2)->name);
}
int cmpatime(const void *s1, const void *s2){
	return r_flg * ((myfile*)s2)->a_time - ((myfile*)s1)->a_time;
}
int cmpctime(const void *s1, const void *s2){
	return r_flg * (((myfile *)s2)->c_time - ((myfile *)s1)->c_time);
}
int cmpmtime(const void *s1, const void *s2){
	return r_flg * ((myfile*)s2)->m_time - ((myfile*)s1)->m_time;
}
int cmpsize(const void *s1, const void *s2){
	return r_flg * ((myfile*)s2)->blocks - ((myfile*)s1)->blocks;
}

/* Recursive sort my dir */
int sortDirs(mydir *dp,int (*cmp)(const void *,const void *)){
	int i;
	if (dp == NULL || dp->fp == NULL)
		return -1;
	for(i = 0; i< dp->fno; i++){
		if ((dp->fp[i].dp) != NULL) {
			sortDirs(dp->fp[i].dp, cmp);
		}
	}
	qsort(dp->fp, dp->fno, sizeof(myfile), cmp);
	return 0;
}