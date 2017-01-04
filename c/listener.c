#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>	
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT '4444'
#define BUFSIZE 8096
#define ERROR 42
#define INVALID 43
#define LOG 44


void log(int type, char *s1, char *s2, int num){
	int fd;
	char logbuffer[BUFSIZE*2];

	switch(type){
	case ERROR: (void)sprintf(logbuffer, "ERROR: %s:%s Errno=%d exiting pid=%d", s1, s2, errno, getpid()); break;
	case INVALID:
		(void)sprintf(logbuffer, "INVALID %s:%s<\r\n", s1,s2);
		(void)write(num,logbuffer,strlen(logbuffer));
		(void)sprintf(logbuffer,"INVALID: %s:%s",s1,s2);
		break;
	case LOG: (void)sprintf(logbuffer,"INFO: %s:%s:%d", s1, s2, num); break;
	}

	if((fd = open("server.log", O_CREAT | O_WRONLY | O_APPEND, 0644)) >= 0){
	(void)write(fd,logbuffer,strlen(logbuffer));
	(void)write(fd,"\n",1);
	(void)close(fd);
	}
	if(type == ERROR || type == INVALID){
		exit(3);
	}
}

void listen(int fd, int hit){
	int j, file_fd, buflen, len;
	long i, ret;
	char * fstr;
	static char buffer[BUFSIZE+1];

	ret = read(fd,buffer,BUFSIZE);
	if(ret == 0|| ret == -1){
		log(INVALID,"failed to read request","",fd);
	}
	if(ret > 0 && ret < BUFSIZE){
		buffer[ret]=0;
	}
	else
		buffer[0]=0;

/*	for(i=0;i<ret;i++){
		if(buffer[i] == ' '){
			buffer[i] = 0;
			break;
		}
	}*/
	log(LOG,"SEND", &buffer, hit);

	(void)write(fd,buffer,strlen(buffer));

#ifdef LINUX
	sleep(1);
#endif
	exit(1);
}

int main(int argc, char **argv){
	int i, port, pid, listenfd, socketfd, hit;
	size_t length;
	static struct sockaddr_in cli_addr;
	static struct sockaddr_in serv_addr;

	if( argc < 3 || argc > 3|| !strcmp(argv[1], "-?")){
		(void)printf("usage: listener [port] [server directory] &"
		"\t Example: listner 80 ./ &\n\n");
		(void)printf("\n\tNot Supported: directories / /etc /bin /lib /tmp /usr /dev /sbin \n");
		exit(0);
	}
	if( !strncmp(argv[2],"/",2) || !strncmp(argv[2],"/etc",5) || !strncmp(argv[2],"/bin",5) || !strncmp(argv[2],"/lib",5) || !strncmp(argv[2],"/tmp",5) || !strncmp(argv[2],"/usr",5) || !strncmp(argv[2],"/dev",5) || !strncmp(argv[2],"/sbin",6)){
		(void)printf("ERROR: Bad top directory %s, see server -?\n", argv[2]);
		exit(3);
	}
	if(chdir(argv[2]) == -1){
		(void)printf("ERROR: Can't change to directory %s\n", argv[2]);
		exit(4);
	}

	if(fork() != 0)
		return 0;
	(void)signal(SIGCLD, SIG_IGN);
	(void)signal(SIGHUP, SIG_IGN);
	for(i=0;i<32;i++)
		(void)close(i);
	(void)setpgrp();

	log(LOG, "server starting", argv[1], getpid());

	if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		log(ERROR, "system call","socket",0);
	port = atoi(argv[1]);
	if(port < 0 || port >60000)
		log(ERROR, "Invalid port number try [1,60000]", argv[1],0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	if(bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		log(ERROR,"system call","bind",0);
	if(listen(listenfd,64) < 0)
		log(ERROR,"system call","listen",0);

	for(hit=1;;hit++){
		length = sizeof(cli_addr);
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0)
			log(ERROR,"system call","accept",0);
		if((pid = fork()) < 0){
			log(ERROR,"system call","fork",0);
		}
		else{
			if(pid == 0){
				(void)close(listenfd);
				listen(socketfd, hit);
			}
			else{
				(void)close(socketfd);
			}
		}
	}
}