

#include "../config.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char *argv[])
{
	int ifd,ofd;
	char *name,*p;
	char buf[200];
	int len;

	if (argc<2) return -1;	
	name=malloc(strlen(argv[1])+10);
	sprintf(name,"%s",argv[1]);
	p=strstr(name,".raw");
	if (p!=NULL){
		sprintf(p,"%s",".wav\0");
	}else{
		sprintf(name,"%s%s",argv[1],".raw");
	}
	
	ifd=open(name,O_RDONLY);
	if (ifd<0) {
		perror("Could not open input file");
		return -1;
	}
	ofd=open(argv[1],O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP);
	if (ofd<0) {
		perror("Could not open output file");
		return -1;
	}
	len=read(ifd,buf,20);
	printf("len=%i\n",len);
	/* erase the wav header */
	if (len>0){
		memset(buf,0,20);
		write(ofd,buf,20);
	}else{
		printf("Error while processing %s: %s\n",argv[1],strerror(errno));
		return -1;
	};

	while ( (len=read(ifd,buf,200))>0){
		#ifdef WORDS_BIGENDIAN	
		for (i=0;i<len/2;i+=2){
			tmp=buf[i];
			buf[i]=buf[i+1];
			buf[i+1]=tmp;
		}
		#endif
		write(ofd,buf,len);
	}

	close(ifd);
	close(ofd);
	return 0;
}


