#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define FINGER_PORT 79
#define bzero(ptr, size) memset (ptr, 0, size)

int tcpconnect(char *host, int port){
	struct hostent *h;
	struct sockaddr_in sa;
	int s;

	h = gethostbyname(host);
	if (!h || h->h_length != sizeof(struct in_addr)){
		fprintf(stderr, "%s: no such host\n", host);
		return -1;
	}

	s = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&sa, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(0);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) < 0){
		perror("bind");
		close(s);
		return -1;
	}

	sa.sin_port = htons(port);
	sa.sin_addr = *(struct in_addr *) h->h_addr;

	if(connect(s, (struct sockaddr *) &sa, sizeof(sa)) < 0){
		perror(host);
		close(s);
		return -1;
	}

	return s;
}

int main(int argc, char **argv){
	char *user;
	char *host;
	int s;
	int nread;
	char buf[1024];

	if(argc == 2){
		user = malloc(1+strlen(argv[1]));
		if(!user){
			fprintf( stderr, "out of memory\n");
			return -1;
		}
		strcpy(user, argv[1]);
		host = strrchr(user, '@');
	}
	else
		user = host = NULL;
	if(!host){
		fprintf(stderr, "userage: %s user@host\n", argv[0]);
		exit(1);
	}
	*host++ = '\0';

	s = tcpconnect(host, FINGER_PORT);
	if(s < 0)
		exit(1);

	if(write(s, user, strlen(user)) < 0 || write(s,"\r\n",2) < 0){
		perror(host);
		exit(1);
	}
	while((nread = read (s, buf, sizeof(buf))) > 0)
		write(1, buf, nread);

	exit(0);
}